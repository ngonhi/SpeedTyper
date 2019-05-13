#define _GNU_SOURCE

#include <curses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "util.h"
#include "interface.h"
#include "list.h"

/**
 * Sleep for a given number of milliseconds
 * \param   ms  The number of milliseconds to sleep for
 */
void sleep_ms(size_t ms) {
  struct timespec ts;
  size_t rem = ms % 1000;
  ts.tv_sec = (ms - rem)/1000;
  ts.tv_nsec = rem * 1000000;
  
  // Sleep repeatedly as long as nanosleep is interrupted
  while(nanosleep(&ts, &ts) != 0) {}
}

/**
 * Get the time in milliseconds since UNIX epoch
 */
size_t time_ms() {
  struct timeval tv;
  if(gettimeofday(&tv, NULL) == -1) {
    perror("gettimeofday");
    exit(2);
  }
  
  // Convert timeval values to milliseconds
  return tv.tv_sec*1000 + tv.tv_usec/1000;
}

// Check if row is empty (no word)
bool is_empty(int row) {
  for (int col = 0; col < BOARD_WIDTH; col++) {
    if (board[row][col] != ' ') {
      return false;
    }
  }

  return true;
}


// Write a helper function check if running is true or false
// lock check unlock return
bool check_running() {
  pthread_mutex_lock(&m_running);
  if (running) {
    pthread_mutex_unlock(&m_running);
    return true;
  } else {
    pthread_mutex_unlock(&m_running);
    return false;
  }
}

void* generate_word(void* p) {
  args_thread_t*args = p;
  int row = args->row;

  while(check_running()) {
    pthread_mutex_lock(&m);
    if(is_empty(row)) {
      // Seek to the end of the file so we can get its size
      if(fseek(stream, 0, SEEK_END) != 0) {
        perror("Unable to seek to end of file");
        exit(2);
      }

      // Get the size of the file
      size_t size = ftell(stream);

      // Seek back to the beginning of the file
      if(fseek(stream, 0, SEEK_SET) != 0) {
        perror("Unable to seek to beginning of file");
        exit(2);
      }
  
      int offset = rand() % size; //check rand
      while(offset > size-8) {
        offset = rand() % size;
        printf("offset = %d\n", offset);
      }

      
      if(fseek(stream, offset, SEEK_SET) != 0) {
        perror("Unable to seek to offset");
        exit(2);
      }
      
      char c;
      if((c = fgetc(stream)) == EOF) {
        if (ferror(stream) == 0) {
          perror("Error read char from input file 1");
          exit(2);
        }
      }
      while(c != '\n' && c != EOF) {
        offset++;
        if((c = fgetc(stream)) == EOF) {
          if (ferror(stream) == 0) {
            perror("Error read char from input file 2");
            exit(2);
          }
        }
      }

      offset += 1; // Move to new word

      // reach the point to start reading
      if(fseek(stream, offset, SEEK_SET) != 0) {
        perror("Unable to seek to offset");
        exit(2);
      }

      char* string = NULL;
      size_t len = 0;
      ssize_t nread = getline(&string, &len, stream);
      if(nread == -1) {
        perror("cannot getline");
        exit(2);
      } else {
        for (int i = 0; i < nread-1; i++) {
          on_screen[row][i] = *(string+i);
          board[row][i] = *(string+i);
        }
      }

      on_screen[row][nread-1] = '\0';
      free(string);
    }
    pthread_mutex_unlock(&m);
  } // while running
  return NULL;
}

// helper to read user input
void read_input() {
  char ch;
  if((ch = getch()) == ERR) {
    fprintf(stderr, "getch fails\n");
    exit(2);
  } 
  int i = 0;
  while(ch != '\n') {
    
    input[i] = ch;
    //pthread_mutex_lock(&m);
    mvaddch(screen_row(BOARD_HEIGHT/2), screen_col(BOARD_WIDTH + 3 + i), ch);
    // pthread_mutex_unlock(&m);
    i++;
    if((ch = getch()) == ERR) {
      fprintf(stderr, "getch fails\n");
      exit(2);
    } 
  }
  input[i] = '\0';
}

// Add a null terminator
// put input word as a global
void* compare_word(void* p) {
  args_thread_t* arg = p;
  int row = arg->row;
  
  while(check_running()) {
    //bool check = true; // might be a prob
   
    pthread_mutex_lock(&m_input);
    if(input[0] == ' ') {
      read_input();
      pthread_mutex_unlock(&m_input);
      pthread_mutex_lock(&m_compare);
      for (int i = 0; i<ROW_NUM; i++) {
        check_compare[i] = true;
      }
      pthread_mutex_unlock(&m_compare);
    } else{
      pthread_mutex_unlock(&m_input);
    }
    
    // Ori check
    
    // Input can be read by multiple threads
    pthread_mutex_lock(&m);
      char* temp = on_screen[row];
      pthread_mutex_unlock(&m);
      pthread_mutex_lock(&m_input);
      if(strcmp(input, temp) != 0) {
        //check = false;
        pthread_mutex_lock(&m_compare); // m_compare
       //is currently for check_compare
        check_compare[row] = false;
        pthread_mutex_unlock(&m_compare);
      }
      pthread_mutex_unlock(&m_input);

      pthread_mutex_lock(&m_compare);
      if(check_compare[row]) {
        pthread_mutex_unlock(&m_compare);
        pthread_mutex_lock(&m_score);
        score++;
        pthread_mutex_unlock(&m_score);
        // clear
        pthread_mutex_lock(&m_input);
        for(int i = 0; i < WORD_LEN; i++) {
        
          input[i] = ' ';
          mvaddch(screen_row(BOARD_HEIGHT/2), screen_col(BOARD_WIDTH + 3 + i), ' ');
        }
        pthread_mutex_unlock(&m_input);
        pthread_mutex_lock(&m);
        for(int i = 0; i < WORD_LEN; i++) { 
          on_screen[row][i] = ' ';
        }
        // delete word on screen and on board
        for(int i = 0; i < BOARD_WIDTH; i++) {
          board[row][i] = ' ';
        }
        pthread_mutex_unlock(&m);
      } else {
        bool check = false;
        for (int i = 0; i<ROW_NUM; i++) {
          if (check_compare[i]) {
            check = true;
          }
        }
        pthread_mutex_unlock(&m_compare);

        if(!check) {
          for(int i = 0; i < WORD_LEN; i++) {
            pthread_mutex_lock(&m_input);
            input[i] = ' ';
            mvaddch(screen_row(BOARD_HEIGHT/2), screen_col(BOARD_WIDTH + 3 + i), ' ');
            pthread_mutex_unlock(&m_input);
          }
      
        }
      }
  } // while running

  return NULL;
}

// change draw_board into a thread function. we will call this in the main.
// 1. draw a random word from the library
// 2. store the word in a buffer
// 3. replace draw worm line of code with sending out the word

/**
 * Run in a thread to draw the current state of the game board.
 */
void* draw_board(void* p) {
  args_thread_t* arg = p;
  int r = arg->row;
  // Need row, need word
  while(check_running()) {
    pthread_mutex_lock(&m);
    // Loop over cells of the game board
    //for (int r=0; r<BOARD_HEIGHT; r++) {
    for(int c=0; c<BOARD_WIDTH; c++) {
      mvaddch(screen_row(r), screen_col(c), board[r][c]);
    }
    //}
  
    // Draw the score
    pthread_mutex_lock(&m_score);
    mvprintw(screen_row(-2), screen_col(BOARD_WIDTH-9), "Score = %d", score); // Get the count
    pthread_mutex_unlock(&m_score);

    // Refresh the display
    refresh();

    pthread_mutex_unlock(&m);
    // Sleep for a while before drawing the board again
    sleep_ms(DRAW_BOARD_INTERVAL);
  }
  
  return NULL;
}

/**
 * Run in a thread to move the worm around on the board
 */
// NEED TO UPDATE SPEED
void* move_word(void* p) {
  args_thread_t* arg = p;
  int row = arg->row;
  char temp[BOARD_WIDTH];

  
  while(check_running()) {
    pthread_mutex_lock(&m);

    for (int i=0; i < BOARD_WIDTH; i++) {
      temp[i] = board[row][i];
    }
    
    // Update one thread i.e. one row
    board[row][0] = ' ';
    for (int col=1; col < BOARD_WIDTH; col++) {
      board[row][col] = temp[col - 1];
    } // for col

    // Check for edge collisions
    if(board[row][BOARD_WIDTH - 1] != ' ') {
      pthread_mutex_lock(&m_running);
      running = false;
      //end_game();
      pthread_mutex_unlock(&m_running);
      //return NULL;
    }

    pthread_mutex_unlock(&m);
    
    sleep_ms(WORM_HORIZONTAL_INTERVAL);   
  } // while

  return NULL;
}

void* run_game(void* p) {
  args_thread_t* arg = p;
  int row = arg->row;
  sleep_ms(interval[row]);

  //pthread_mutex_lock(&m);
  //generate_word(stream, row);
  //pthread_mutex_unlock(&m);

  pthread_t threads[4];
  args_thread_t args[4];

  for(int i = 0; i < 4; i++) {
    args[i].row = row;
    //printf("%d ",args[i].row);
  }

  if(pthread_create(&threads[0], NULL, generate_word, &args[0])) {
    perror("pthread_creates failed\n");
    exit(2);
  } else {
    addNode(list, threads[0]);
  }
  
  if(pthread_create(&threads[1], NULL, draw_board, &args[1])) {
    perror("pthread_creates failed\n");
    exit(2);
  } else {
    addNode(list, threads[1]);
  }
  
  if(pthread_create(&threads[2], NULL, compare_word, &args[2])) {
    perror("pthread_creates failed\n");
    exit(2);
  } else {
    addNode(list, threads[2]);
  }

  if(pthread_create(&threads[3], NULL, move_word, &args[3])) {
    perror("pthread_creates failed\n");
    exit(2);
  } else {
    addNode(list, threads[3]);
  }

  for(int i = 0; i < 4; i++) {
    if(pthread_join(threads[i], NULL)) {
      perror("pthread_join main failed\n");
      exit(2);
    }
  }

  return NULL;
}


// Write a helper function check if running is true or false
// lock check unlock return
bool check_kill_thread(int num) {
  pthread_mutex_lock(&list->lst_m);
  if (num < list->length) {
    pthread_mutex_unlock(&list->lst_m);
    return true;
  } else {
    pthread_mutex_unlock(&list->lst_m);
    return false;
  }
}

void* check_thread() {
  while(check_running()) {
    // sleep_ms(100);
  }
  // terminate all other threads
  pthread_mutex_lock(&list->lst_m);
  node_t* cur = list->first;
  pthread_mutex_unlock(&list->lst_m);
  //int i = 0;

  pthread_mutex_lock(&lock_cv);
  int temp = kill_thread;
  pthread_mutex_unlock(&lock_cv);

  end_game();
  
  while(check_kill_thread(temp)) {
    pthread_mutex_lock(&list->lst_m);
    pthread_t cur_thread = cur->thread;
    if(pthread_kill(cur_thread, SIGSTOP) != 0) {
      perror("cannot kill thread");
      exit(2);
    }
    
    cur = cur->next;
    pthread_mutex_unlock(&list->lst_m);

    // cond variable piece
    pthread_mutex_lock(&lock_cv);
    kill_thread++;
    mvaddch(screen_row(BOARD_HEIGHT), screen_col(10), kill_thread);
    temp = kill_thread;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&lock_cv);
    
  }
  //pthread_mutex_lock(&list->lst_m);
  
  return NULL;
}
