#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include "scheduler.h"

#include <assert.h>
#include <curses.h>
#include <ucontext.h>

#include "util.h"

// This is an upper limit on the number of tasks we can create.
#define MAX_TASKS 128

// This is the size of each task's stack memory
#define STACK_SIZE 65536

#define STATE_RUN 1
#define STATE_DONE 2
#define STATE_READ 3
#define STATE_WAIT 4
#define STATE_SLEEP 5

// This struct will hold the all the necessary information for each task
typedef struct task_info {
  // This field stores all the state required to switch back to this task
  ucontext_t context;
  // This field stores another context. This one is only used when the task
  // is exiting.
  ucontext_t exit_context;
  // Keep track of this task's state.
  int state;
  // If the task is sleeping, when should it wake up?
  size_t sleep;
  // If the task is waiting for another task, which task is it waiting for?
  task_t next;
  // Was the task blocked waiting for user input? Once you successfully
  // read input, you will need to save it here so it can be returned.
  int read;
} task_info_t;

int current_task = 0; //< The handle of the currently-executing task
int num_tasks = 1;    //< The number of tasks created so far
task_info_t tasks[MAX_TASKS]; // Information for every task


/**
 * This function builds up the scheduler that swap tasks based on the state of
 * the current task.
 */
void scheduler_next_task() {
  // i is the current index of the tasks array
  int i = current_task;
  // loop through the list of tasks until we find one that we can run
  // then break out and swap to it
  while(1) {
    i++;
    // if we are past num_tasks, go back to 0
    if (i >= num_tasks) {
      i = 0;
    }
    // determine if next task is good to run
    if (tasks[i].state == STATE_RUN) {
      // run STATE_RUN tasks always
      break;
    } else if (tasks[i].state == STATE_WAIT) {
      // tasks[i] is waiting on tasks[wait]
      task_t wait = tasks[i].next;
      if (tasks[wait].state == STATE_DONE) {
        // if the waited task is done
        // run tasks[i]
        tasks[i].state = STATE_RUN;
        break;
      }
    } else if (tasks[i].state == STATE_SLEEP) {
      // get current time and see if it is past the tasks wakeup time
      size_t now = time_ms();
      if (now >= tasks[i].sleep) {
        // wake up the task and run it
        tasks[i].state = STATE_RUN;
        break;
      }
    } else if (tasks[i].state == STATE_READ) {
      // test if we have read input yet
      int c = getch();
      if (c != ERR) {
        // set the tasks read value to the input
        tasks[i].read = c;
        // then run it
        tasks[i].state = STATE_RUN;
        break;
      }
    }
  }
  // outside the loop, i = the next task to swap to
  if (current_task == i) {
    return; // task and i are the same so we don't do anything
  } else {
    int old_current = current_task;
    current_task = i;
    swapcontext(&(tasks[old_current].context),&(tasks[i].context));
  }
}
/**
 * Initialize the scheduler. Programs should call this before calling any other
 * functiosn in this file.
 */
void scheduler_init() {
  // Initialize the state of the scheduler
  tasks[current_task].state = STATE_RUN;
}


/**
 * This function will execute when a task's function returns. This allows you
 * to update scheduler states and start another task. This function is run
 * because of how the contexts are set up in the task_create function.
 */
void task_exit() {
  // Handle the end of a task's execution here
  tasks[current_task].state = STATE_DONE;
  //swapcontext(&task.context,&task.exit_context);
  scheduler_next_task();
}

/**
 * Create a new task and add it to the scheduler.
 *
 * \param handle  The handle for this task will be written to this location.
 * \param fn      The new task will run this function.
 */
void task_create(task_t* handle, task_fn_t fn) {
  // Claim an index for the new task
  int index = num_tasks;
  num_tasks++;
  
  // Set the task handle to this index, since task_t is just an int
  *handle = index;
 
  // We're going to make two contexts: one to run the task, and one that runs at the end of the task so we can clean up. Start with the second
  
  // First, duplicate the current context as a starting point
  getcontext(&tasks[index].exit_context);
  
  // Set up a stack for the exit context
  tasks[index].exit_context.uc_stack.ss_sp = malloc(STACK_SIZE);
  tasks[index].exit_context.uc_stack.ss_size = STACK_SIZE;
  
  // Set up a context to run when the task function returns. This should call task_exit.
  makecontext(&tasks[index].exit_context, task_exit, 0);
  
  // Now we start with the task's actual running context
  getcontext(&tasks[index].context);
  
  // Allocate a stack for the new task and add it to the context
  tasks[index].context.uc_stack.ss_sp = malloc(STACK_SIZE);
  tasks[index].context.uc_stack.ss_size = STACK_SIZE;
  
  // Now set the uc_link field, which sets things up so our task will go to the exit context when the task function finishes
  tasks[index].context.uc_link = &tasks[index].exit_context;
  
  // And finally, set up the context to execute the task function
  makecontext(&tasks[index].context, fn, 0);
  // set the new tasks state to run
  tasks[index].state = STATE_RUN;
}

/**
 * Wait for a task to finish. If the task has not yet finished, the scheduler should
 * suspend this task and wake it up later when the task specified by handle has exited.
 *
 * \param handle  This is the handle produced by task_create
 */
void task_wait(task_t handle) {
  // Block this task until the specified task has exited.
  tasks[current_task].state = STATE_WAIT;
  tasks[current_task].next = handle;
  scheduler_next_task();
}

/**
 * The currently-executing task should sleep for a specified time. If that time is larger
 * than zero, the scheduler should suspend this task and run a different task until at least
 * ms milliseconds have elapsed.
 * 
 * \param ms  The number of milliseconds the task should sleep.
 */
void task_sleep(size_t ms) {
  // Block this task until the requested time has elapsed.
  size_t now = time_ms();
  tasks[current_task].state = STATE_SLEEP;
  tasks[current_task].sleep = now + ms;
  scheduler_next_task();
}

/**
 * Read a character from user input. If no input is available, the task should
 * block until input becomes available. The scheduler should run a different
 * task while this task is blocked.
 *
 * \returns The read character code
 */
int task_readchar() {
  // Block this task until there is input available.
  tasks[current_task].state = STATE_READ;
  scheduler_next_task();
  if (tasks[current_task].read != ERR) {
    return tasks[current_task].read;
  } else {
    return ERR;
  }
}
