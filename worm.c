#include <curses.h>


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "util.h"
#include "list.h"
#include "interface.h"


char board[BOARD_HEIGHT][BOARD_WIDTH];
char on_screen[ROW_NUM][WORD_LEN];
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // board, on_screen

FILE* stream;

char input[WORD_LEN];
pthread_mutex_t m_input = PTHREAD_MUTEX_INITIALIZER; // input

bool check_compare[ROW_NUM];
pthread_mutex_t m_compare = PTHREAD_MUTEX_INITIALIZER; // check_compare

int score = 0;
pthread_mutex_t m_score = PTHREAD_MUTEX_INITIALIZER; // score

bool running = true;
pthread_mutex_t m_running = PTHREAD_MUTEX_INITIALIZER; // running

int kill_thread = 0;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock_cv = PTHREAD_MUTEX_INITIALIZER;

size_t interval[ROW_NUM] = {0, 1000, 2000, 5000, 9000, 4000, 7000, 3000, 8000, 6000};

linkedlist_t* list = NULL;


// Entry point: Set up the game, create jobs, then run the scheduler
int main(void) {
  user_menu();
  read_user_command();
  list = listInit();

  if ((stream = fopen("input.txt", "r")) == NULL) {
    fprintf(stderr, "Error opening input file");
    exit(2);
  } // Check error
  //Initialize the ncurses window
  WINDOW* mainwin = initscr();
  if(mainwin == NULL) {
    fprintf(stderr, "Error initializing ncurses.\n");
    exit(2);
  }
  
  // Seed random number generator with the time in milliseconds
  srand(time_ms());
  
  noecho();               // Don't print keys when pressed
  //keypad(mainwin, true);  // Support arrow keys
  //nodelay(mainwin, true); // Non-blocking keyboard access
  cbreak();
  // Initialize the game display
  init_display();
  
  // Zero out the board contents
  //memset(board, ' ', BOARD_WIDTH*BOARD_HEIGHT*sizeof(char));

  for(int i = 0; i < BOARD_HEIGHT; i++) {
    for(int j = 0; j < BOARD_WIDTH; j++) {
      board[i][j] = ' ';
    }
  }

  // clean at first
  for(int i = 0; i < WORD_LEN; i++) {
    input[i] = ' ';
  }

  for (int i = 0; i < ROW_NUM; i++) {
    for(int j = 0; j < WORD_LEN; j++) {
      on_screen[i][j] = ' ';
    }
  }
  
  pthread_t check;
  if(pthread_create(&check, NULL, check_thread, NULL)) {
    perror("pthread_creates failed\n");
    exit(2);
  }
  
  
  pthread_t threads[ROW_NUM];
  args_thread_t args[ROW_NUM];
  for(int i = 0; i < ROW_NUM; i++) {
    args[i].row = i;
    if(pthread_create(&threads[i], NULL, run_game, &args[i])) {
      perror("pthread_creates failed\n");
      exit(2);
    } else {
      // store corresponding row number and boolean
      check_compare[i] = true;
      addNode(list, threads[i]);
    }
  }


  /*
  for(int i = 0; i < ROW_NUM; i++) {
    if(pthread_join(threads[i], NULL)) {
      perror("pthread_join main failed\n");
      exit(2);
    }
  }
  
  if(pthread_join(check, NULL)) {
      perror("pthread_join main failed\n");
      exit(2);
   }
  */

  //use conditional variable
  pthread_mutex_lock(&lock_cv);
  while(kill_thread != 50) {
    pthread_cond_wait(&cv, &lock_cv);
  }
  pthread_mutex_unlock(&lock_cv);

  
  

  
  


  // Display the end of game message and wait for user input
  end_game();
  
  // Clean up window
  delwin(mainwin);
  endwin();

  destroyList(list);

  fclose(stream);
  return 0;
} // main


/* In main, we create 10 threads, each thread responsible for one row of words.
 * Each thread will generate word if a line is empty, move and compare word simultaneously
 * Struct to keep the state of each row, if empty, generate word, if not move and compare
 */

