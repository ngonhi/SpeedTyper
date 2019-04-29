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

#include "scheduler.h"
#include "util.h"

// Defines used to track the worm direction
#define DIR_NORTH 0
#define DIR_EAST 1
#define DIR_SOUTH 2
#define DIR_WEST 3

// Game parameters
#define INIT_WORM_LENGTH 4
#define WORM_HORIZONTAL_INTERVAL 200
#define WORM_VERTICAL_INTERVAL 300
#define DRAW_BOARD_INTERVAL 33
#define APPLE_UPDATE_INTERVAL 120
#define READ_INPUT_INTERVAL 150
#define GENERATE_APPLE_INTERVAL 2000
#define BOARD_WIDTH 50
#define BOARD_HEIGHT 25
#define BUFFER_LEN 20
#define LINE_NUM 9897

/**
 * In-memory representation of the game board
 * Zero represents an empty cell
 * Positive numbers represent worm cells (which count up at each time step until they reach worm_length)
 * Negative numbers represent apple cells (which count up at each time step)
 */
int board[BOARD_HEIGHT][BOARD_WIDTH];

char* library[LINE_NUM];

void read_library() {
  FILE* stream = fopen("./input.txt", "r");
  for(int i = 0; i < LINE_NUM; i++) {
    if(fgets(library[i], BUFFER_LEN, stream) == NULL) {
      perror("fgets failed\n");
      exit(2);
    }
  }
}

// Worm parameters
int worm_dir = DIR_NORTH;
int worm_length = INIT_WORM_LENGTH;

// Apple parameters
int apple_age = 120;

// Is the game running?
bool running = true;

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
  return 2 + col;
}

/**
 * Initialize the board display by printing the title and edges
 */
void init_display() {
  // Print Title Line
  move(screen_row(-2), screen_col(BOARD_WIDTH/2 - 12));
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
  mvprintw(screen_row(BOARD_HEIGHT/2)-1, screen_col(BOARD_WIDTH/2)-6, "            ");
  mvprintw(screen_row(BOARD_HEIGHT/2),   screen_col(BOARD_WIDTH/2)-6, " Game Over! ");
  mvprintw(screen_row(BOARD_HEIGHT/2)+1, screen_col(BOARD_WIDTH/2)-6, "            ");
  mvprintw(screen_row(BOARD_HEIGHT/2)+2, screen_col(BOARD_WIDTH/2)-11, "Press any key to exit.");
  refresh();
  timeout(-1);
  task_readchar();
}


// change draw_board into a thread function. we will call this in the main.
// 1. draw a random word from the library
// 2. store the word in a buffer
// 3. replace draw worm line of code with sending out the word

/**
 * Run in a thread to draw the current state of the game board.
 */
void* draw_board(void* p) {
  int* r = p;

  // Need row, need word
  while(running) {
    // int line = rand() % LINE_NUM;
    // read specific line and store in buffer
    
    // Loop over cells of the game board
    for(int c=0; c<BOARD_WIDTH; c++) {
      if(board[*r][c] == 0) {  // Draw blank spaces
        mvaddch(screen_row(*r), screen_col(c), ' ');
      } else if (board[*r][c] > 0) {  // Draw worm
        mvaddch(screen_row(*r), screen_col(c), 'O');
      }
    }
  }
  
  // Draw the score
  mvprintw(screen_row(-2), screen_col(BOARD_WIDTH-9), "Score %03d\r", worm_length-INIT_WORM_LENGTH);
  
  // Refresh the display
  refresh();
    
  // Sleep for a while before drawing the board again
  task_sleep(DRAW_BOARD_INTERVAL);

  return NULL;
}

/**
 * Run in a thread to process user input.
 */
void* read_input() {
  while(running) {
    // Read a character, potentially blocking this thread until a key is pressed
    int key = task_readchar();
    
    // Make sure the input was read correctly
    if(key == ERR) {
      running = false;
      fprintf(stderr, "ERROR READING INPUT\n");
    }
    
    // Handle the key press
    if(key == KEY_UP && worm_dir != DIR_SOUTH) {
      worm_dir = DIR_NORTH;
    } else if(key == KEY_RIGHT && worm_dir != DIR_WEST) {
      worm_dir = DIR_EAST;
    } else if(key == KEY_DOWN && worm_dir != DIR_NORTH) {
      worm_dir = DIR_SOUTH;
    } else if(key == KEY_LEFT && worm_dir != DIR_EAST) {
      worm_dir = DIR_WEST;
    } else if(key == 'q') {
      running = false;
    }
  }

  return NULL;
}

/**
 * Run in a thread to move the worm around on the board
 */
void* update_worm() {
  while(running) {
    int worm_row;
    int worm_col;
  
    // "Age" each existing segment of the worm
    for(int r=0; r<BOARD_HEIGHT; r++) {
      for(int c=0; c<BOARD_WIDTH; c++) {
        if(board[r][c] == 1) {  // Found the head of the worm. Save position
          worm_row = r;
          worm_col = c;
        }
      
        // Add 1 to the age of the worm segment
        if(board[r][c] > 0) {
          board[r][c]++;
        
          // Remove the worm segment if it is too old
          if(board[r][c] > worm_length) {
            board[r][c] = 0;
          }
        }
      }
    }
  
    // Move the worm into a new space
    if(worm_dir == DIR_NORTH) {
      worm_row--;
    } else if(worm_dir == DIR_SOUTH) {
      worm_row++;
    } else if(worm_dir == DIR_EAST) {
      worm_col++;
    } else if(worm_dir == DIR_WEST) {
      worm_col--;
    }
  
    // Check for edge collisions
    if(worm_row < 0 || worm_row >= BOARD_HEIGHT || worm_col < 0 || worm_col >= BOARD_WIDTH) {
      running = false;
      
      // Add a key to the input buffer so the read_input thread can exit
      ungetch(0);
    }
  
    // Check for worm collisions
    if(board[worm_row][worm_col] > 0) {
      running = false;
      
      // Add a key to the input buffer so the read_input thread can exit
      ungetch(0);
    }
  
    // Check for apple collisions
    if(board[worm_row][worm_col] < 0) {
      // Worm gets longer
      worm_length++;
    }
  
    // Add the worm's new position
    board[worm_row][worm_col] = 1;
  
    // Update the worm movement speed to deal with rectangular cursors
    if(worm_dir == DIR_NORTH || worm_dir == DIR_SOUTH) {
      task_sleep(WORM_VERTICAL_INTERVAL);
    } else {
      task_sleep(WORM_HORIZONTAL_INTERVAL);
    }
  }

  return NULL;
}

void* run_game(void* p) {
  args_thread_t* args = p;
  int row = args->row;

  pthread_t threads[3];
  pthread_create(&threads[0], NULL, update_worm, NULL);
  pthread_create(&threads[1], NULL, draw_board, &row);
  pthread_create(&threads[2], NULL, read_input, NULL);

  for(int i = 0; i < 3; i++) {
    if(pthread_join(threads[i], NULL)) {
      perror("pthread_join failed\n");
      exit(2);
    }
  }

  return NULL;
}

// Entry point: Set up the game, create jobs, then run the scheduler
int main(void) {
  // load the dictionary
  read_library();
  
  // Initialize the ncurses window
  WINDOW* mainwin = initscr();
  if(mainwin == NULL) {
    fprintf(stderr, "Error initializing ncurses.\n");
    exit(2);
  }
  
  // Seed random number generator with the time in milliseconds
  srand(time_ms());
  
  noecho();               // Don't print keys when pressed
  keypad(mainwin, true);  // Support arrow keys
  nodelay(mainwin, true); // Non-blocking keyboard access
  
  // Initialize the game display
  init_display();
  
  // Zero out the board contents
  memset(board, 0, BOARD_WIDTH*BOARD_HEIGHT*sizeof(int));

  // Put the worm at the right border of the board
  board[0][BOARD_WIDTH-1] = 1;
  
  pthread_t threads[BOARD_HEIGHT];
  args_thread_t args[BOARD_HEIGHT];
  for(int i = 0; i < BOARD_HEIGHT; i++) {
    args[i].row = rand() % BOARD_HEIGHT;
    if(pthread_create(&threads[i], NULL, run_game, &args[i])) {
      perror("pthread_creates failed\n");
      exit(2);
    }
  }
  
  // Don't wait for the generate_apple task because it sleeps for 2 seconds,
  // which creates a noticeable delay when exiting.
  //task_wait(generate_apple_thread);
  
  // Display the end of game message and wait for user input
  end_game();
  
  // Clean up window
  delwin(mainwin);
  endwin();

  return 0;
}
