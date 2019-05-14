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
#include "interface.h"

//
char board[BOARD_HEIGHT][BOARD_WIDTH];
char on_screen[ROW_NUM][WORD_LEN];
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // board, on_screen

FILE* stream;
WINDOW* mainwin;

char input[WORD_LEN];
pthread_mutex_t m_input = PTHREAD_MUTEX_INITIALIZER; // input

int score = 0;
pthread_mutex_t m_score = PTHREAD_MUTEX_INITIALIZER; // score

bool running = true;
pthread_mutex_t m_running = PTHREAD_MUTEX_INITIALIZER; // running

size_t interval[ROW_NUM] = {0, 1000, 2000, 5000, 9000, 4000, 7000, 3000, 8000, 6000};


// Entry point: Set up the game, create jobs, then run the scheduler
int main(void) {
  user_menu();
  read_user_command();
  
  if ((stream = fopen("input.txt", "r")) == NULL) {
    fprintf(stderr, "Error opening input file");
    exit(2);
  } // Check error
  //Initialize the ncurses window
  mainwin = initscr();
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
  
  /*
  pthread_t check;
  if(pthread_create(&check, NULL, check_thread, NULL)) {
    perror("pthread_creates failed\n");
    exit(2);
  }*/
  
  pthread_t compare;
  if(pthread_create(&compare, NULL, compare_word, NULL)) {
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
    }
  }

  if(pthread_join(compare, NULL)) {
      perror("pthread_join main failed\n");
      exit(2);
   }
  
  for(int i = 0; i < ROW_NUM; i++) {
    if(pthread_join(threads[i], NULL)) {
      perror("pthread_join main failed\n");
      exit(2);
    }
  }

  // close window
  delwin(mainwin);
  endwin();
  
  return 0;
} // main

