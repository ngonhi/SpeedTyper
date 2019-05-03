#include <curses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "util.h"
#include "utility.h"

// Game parameters
#define WORM_HORIZONTAL_INTERVAL 500 // This will be word speed
#define DRAW_BOARD_INTERVAL 30

#define READ_INPUT_INTERVAL 150


#define BUFFER_LEN 50

/**
 * In-memory representation of the game board
 * Zero represents an empty cell
 * Positive numbers represent worm cells (which count up at each time step until they reach worm_length)
 * Negative numbers represent apple cells (which count up at each time step)
 */
char board[BOARD_HEIGHT][BOARD_WIDTH];

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
FILE* stream;

// Worm parameters
//int worm_dir = DIR_NORTH;
//int worm_length = INIT_WORM_LENGTH;


// Is the game running?
bool running = true;
int counter = 0;
typedef struct args_thread{
  int row;
  //char * words; // List of words 
} args_thread_t;


/**
 * Convert a board row number to a screen position
 * \param   row   The board row number to convert
 * \return        A corresponding row number for the ncurses screen
 */
int screen_row(int row) {
  return 2 + row;
}

/**
 * Convert a board column number to a screen position
 * \param   col   The board column number to convert
 * \return        A corresponding column number for the ncurses screen
 */
int screen_col(int col) {
  // Ori 2 + col
  return 2 + col;
}

/**
 * Initialize the board display by printing the title and edges
 */
void init_display() {
  // Print Title Line
  move(screen_row(-2), screen_col(BOARD_WIDTH/2 - 9));
  addch(ACS_DIAMOND);
  addch(ACS_DIAMOND);
  printw(" SPEED TYPER! ");
  addch(ACS_DIAMOND);
  addch(ACS_DIAMOND);
  
  // Print corners
  mvaddch(screen_row(-1), screen_col(-1), ACS_ULCORNER);
  mvaddch(screen_row(-1), screen_col(BOARD_WIDTH), ACS_URCORNER);
  mvaddch(screen_row(BOARD_HEIGHT), screen_col(-1), ACS_LLCORNER);
  mvaddch(screen_row(BOARD_HEIGHT), screen_col(BOARD_WIDTH), ACS_LRCORNER);
  
  // Print top and bottom edges
  for(int col=0; col<BOARD_WIDTH; col++) {
    mvaddch(screen_row(-1), screen_col(col), ACS_HLINE);
    mvaddch(screen_row(BOARD_HEIGHT), screen_col(col), ACS_HLINE);
  }
  
  // Print left and right edges
  for(int row=0; row<BOARD_HEIGHT; row++) {
    mvaddch(screen_row(row), screen_col(-1), ACS_VLINE);
    mvaddch(screen_row(row), screen_col(BOARD_WIDTH), ACS_VLINE);
  }
  
  // Refresh the display
  refresh();
}

/**
 * Show a game over message and wait for a key press.
 */
void end_game() {
  // Print out the count for each player
  // Create print score function
  mvprintw(screen_row(BOARD_HEIGHT/2)-1, screen_col(BOARD_WIDTH/2)-6, "            ");
  mvprintw(screen_row(BOARD_HEIGHT/2),   screen_col(BOARD_WIDTH/2)-6, " Game Over! ");
  mvprintw(screen_row(BOARD_HEIGHT/2)+1, screen_col(BOARD_WIDTH/2)-6, "            ");
  mvprintw(screen_row(BOARD_HEIGHT/2)+2, screen_col(BOARD_WIDTH/2)-11, "Press any key to exit.");
  refresh();
  timeout(-1);
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
  while(running) {
    pthread_mutex_lock(&m);
    // Loop over cells of the game board
    //for (int r=0; r<BOARD_HEIGHT; r++) {
      for(int c=0; c<BOARD_WIDTH; c++) {
        mvaddch(screen_row(r), screen_col(c), board[r][c]);
      }
    //}
  
  // Draw the score
  mvprintw(screen_row(-2), screen_col(BOARD_WIDTH-9), "Score 100\r"); // Get the count
  
  // Refresh the display
  refresh();
    
  // Sleep for a while before drawing the board again
  sleep_ms(DRAW_BOARD_INTERVAL);
  pthread_mutex_unlock(&m);
  }
  
  return NULL;
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

void* del_word(void* p) {
  int* row = (int*) p;
  char temp[BOARD_WIDTH];

  // Init change to memcpy
  for (int i=0; i < BOARD_WIDTH; i++) {
    temp[i] = board[*row][i];
  }
  
  while(running) {
    // Update one thread i.e. one row
    board[*row][0] = ' ';
    for (int col=1; col < BOARD_WIDTH; col++) {
      board[*row][col] = temp[col - 1];
    } // for col

    for (int i=0; i < BOARD_WIDTH; i++) {
      temp[i] = board[*row][i];
    }

    // Check for edge collisions
    if(board[*row][BOARD_WIDTH - 1] == ' ') {
      running = false;
    }
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

  // Init change to memcpy
  for (int i=0; i < BOARD_WIDTH; i++) {
    temp[i] = board[row][i];
  }
  
  while(running) {
    // Update one thread i.e. one row
    board[row][0] = ' ';
    for (int col=1; col < BOARD_WIDTH; col++) {
      board[row][col] = temp[col - 1];
    } // for col

    for (int i=0; i < BOARD_WIDTH; i++) {
      temp[i] = board[row][i];
    }

    // Check for edge collisions
    if(board[row][BOARD_WIDTH - 1] != ' ') {
      running = false;
      //end_game();
    }

    sleep_ms(WORM_HORIZONTAL_INTERVAL);
  } // while

  return NULL;
}

void* run_game(void* p) {
  args_thread_t* arg = p;
  int row = arg->row;

  //pthread_mutex_lock(&m);
  //generate_word(stream, row);
  //pthread_mutex_unlock(&m);

  pthread_t threads[2];
  args_thread_t args[2];
  for(int i = 0; i < 2; i++) {
    args[i].row = row;
    //printf("%d ",args[i].row);
  }
  
  if(pthread_create(&threads[0], NULL, draw_board, &args[0])) {
      perror("pthread_creates failed\n");
      exit(2);
  }

  if(pthread_create(&threads[1], NULL, move_word, &args[1])) {
      perror("pthread_creates failed\n");
      exit(2);
  }

  for(int i = 0; i < 2; i++) {
    if(pthread_join(threads[i], NULL)) {
      perror("pthread_join main failed\n");
      exit(2);
    }
  }
  return NULL;
}


// Entry point: Set up the game, create jobs, then run the scheduler
int main(void) {
  
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
  stream = fopen("./small_input.txt", "r");
  // Initialize the game display
  init_display();
  
  // Zero out the board contents
  memset(board, ' ', BOARD_WIDTH*BOARD_HEIGHT*sizeof(char));

  for(int i = 0; i<BOARD_HEIGHT; i++) {
    board[i][0] = 'a';
  }

/*
  for(int i = 0; i < BOARD_HEIGHT; i++) {
    for(int j = 0; j < BOARD_WIDTH; j++) {
      printf("%c ", board[i][j]);
      //mvaddch(screen_row(i), screen_col(j), board[i][j]);
    }
    printf("\n");
  }
*/
/*
  args_thread_t args[10];
  for(int i = 0; i < 10; i++) {
    args[i].row = i*2+3;
    printf("%d ",args[i].row);
  }
*/
  
  pthread_t threads[10];
  args_thread_t args[10];
  for(int i = 0; i < 10; i++) {
    args[i].row = i*2;
    if(pthread_create(&threads[i], NULL, run_game, &args[i])) {
      perror("pthread_creates failed\n");
      exit(2);
    }
  }
  
  for(int i = 0; i < 10; i++) {
    if(pthread_join(threads[i], NULL)) {
      perror("pthread_join main failed\n");
      exit(2);
    }
  }


  //run_game(counter+2);

  // Display the end of game message and wait for user input
  end_game();
  
  // Clean up window
  delwin(mainwin);
  endwin();

  return 0;
} // main


/* In main, we create 10 threads, each thread responsible for one row of words.
 * Each thread will generate word if a line is empty, move and compare word simultaneously
 * Struct to keep the state of each row, if empty, generate word, if not move and compare
*/

