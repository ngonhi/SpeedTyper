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

#define WORD_LEN 50
#define ROW_NUM 10
#define BOARD_WIDTH 50
#define BOARD_HEIGHT 20 // Every two rows has one thread of words

// Game parameters
#define WORM_HORIZONTAL_INTERVAL 700 // This will be word speed
#define DRAW_BOARD_INTERVAL 700

#define READ_INPUT_INTERVAL 150

typedef struct args_thread{
  int row;
  //char * words; // List of words 
} args_thread_t;

/**
 * In-memory representation of the game board
 * Zero represents an empty cell
 * Positive numbers represent worm cells (which count up at each time step until they reach worm_length)
 * Negative numbers represent apple cells (which count up at each time step)
 */
char board[BOARD_HEIGHT][BOARD_WIDTH];
char on_screen[ROW_NUM][WORD_LEN];
FILE* stream;
char input[WORD_LEN];
int count_thread;
bool running = true;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m3 = PTHREAD_MUTEX_INITIALIZER;

// Worm parameters
//int worm_dir = DIR_NORTH;
//int worm_length = INIT_WORM_LENGTH;


// Is the game running?

int counter = 0;


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
  pthread_mutex_lock(&m);
  if (running) {
    pthread_mutex_unlock(&m);
    return true;
  } else {
    pthread_mutex_unlock(&m);
    return false;
  }
}

bool check_not_empty() {
  pthread_mutex_lock(&m2);
  if (input[0] != ' ') {
    pthread_mutex_unlock(&m2);
    return true;
  } else {
    pthread_mutex_unlock(&m2);
    return false;
  }
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
      ssize_t nread;
      if((nread = getline(&string, &len, stream)) != -1) {
        for (int i = 0; i < nread-1; i++) {
          on_screen[row][i] = *(string+i);
          board[row][i] = *(string+i);
          mvprintw(screen_row(BOARD_HEIGHT + 10), screen_col(BOARD_WIDTH + i), "%c", on_screen[row][i]);
        }
        
      }
      on_screen[row][nread-1] = '\0';
      //board[row][nread-1] = ' ';
      //on_screen[row][nread-1] = ' ';
      
      /*
        int j = 0;
  
        char ch;
        if((ch = fgetc(stream)) == EOF) {
        if (ferror(stream) == 0) {
        fprintf(stderr, "Error read char from input file 3\n");
        exit(2);
        }
        }
        while(ch != '\n' && ch != EOF) {
        on_screen[row][j] = ch;
        board[row][j] = ch;
        j++;
        //printf("%c\n", ch);
        if((ch = fgetc(stream)) == EOF) {
        if (ferror(stream) == 0) {
        fprintf(stderr, "Error read char from input file 4\n");
        exit(2);
        }
        }
        }
      */
      free(string);
    }
    pthread_mutex_unlock(&m);
  } // while running
  return NULL;
}

// helper for compare_word
/*
  void match_letter(FILE* stream, int i, int* j, int* counter) {
  char ch = getchar();
  while(ch != '\n' && ch == on_screen[i][*j]) {
  *j++;
  *counter++;
  ch = getchar();
  }
  }
*/

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
    mvaddch(screen_row(BOARD_HEIGHT + 3), screen_col(BOARD_WIDTH + 5 + i), ch);
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
  // maximum one word on each row

  // read the length of the string
  //int length = strlen(input);
  
  
  while(check_running()) {
    pthread_mutex_lock(&m2);
    if(input[0] == ' ') {
      read_input();
      pthread_mutex_lock(&m3);
      count_thread = 0;
      pthread_mutex_unlock(&m3);
    }
    pthread_mutex_unlock(&m2);
    
    //int i = 0;
    bool check = true;
    // Input can be read by multiple threads
    //while(check_not_empty()) {
      pthread_mutex_lock(&m);

      if(strcmp(input, on_screen[row]) != 0) {
        check = false;
        //pthread_mutex_unlock(&m);
        //break;
      }
      /*
      if(input[i] != on_screen[row][i]) {
        check = false;
        pthread_mutex_unlock(&m);
        break;
      }
      */
      //i++;
      //pthread_mutex_unlock(&m);
      //} // while empty
    //pthread_mutex_unlock(&m);

    //pthread_mutex_lock(&m);
    if(check) {
      
      // clear
      for(int i = 0; i < WORD_LEN; i++) {
        pthread_mutex_lock(&m2);
        input[i] = ' ';
        mvaddch(screen_row(BOARD_HEIGHT + 3), screen_col(BOARD_WIDTH + 5 + i), ' ');
        pthread_mutex_unlock(&m2);
        on_screen[row][i] = ' ';
      }
      // delete word on screen and on board
      for(int i = 0; i < BOARD_WIDTH; i++) {
        board[row][i] = ' ';
      }
    } else {
      pthread_mutex_lock(&m3);
      count_thread++;
      pthread_mutex_unlock(&m3);
    }

pthread_mutex_lock(&m3);
    mvprintw(screen_row(BOARD_HEIGHT + 5), screen_col(BOARD_WIDTH + 5), "count_thread = %d", count_thread);  
    pthread_mutex_unlock(&m3);
      // check if there is no match and clear the buffer if so
    pthread_mutex_lock(&m3);
      if (count_thread == 10) {
        for(int i = 0; i < WORD_LEN; i++) {
          pthread_mutex_lock(&m2);
          input[i] = ' ';
          mvaddch(screen_row(BOARD_HEIGHT + 3), screen_col(BOARD_WIDTH + 5 + i), ' ');
          pthread_mutex_unlock(&m2);
        }
      }
      pthread_mutex_unlock(&m3);
    pthread_mutex_unlock(&m);

   
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
      //printf("DRAWING\n");
    }
    //}
  
    // Draw the score
    mvprintw(screen_row(-2), screen_col(BOARD_WIDTH-9), "Score 100\r"); // Get the count
  
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

  /* Init change to memcpy
  pthread_mutex_lock(&m);
  for (int i=0; i < BOARD_WIDTH; i++) {
    temp[i] = board[row][i];
  }
  pthread_mutex_unlock(&m);*/
  
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

    for (int i=0; i < BOARD_WIDTH; i++) {
      temp[i] = board[row][i];
    }

    // Check for edge collisions
    if(board[row][BOARD_WIDTH - 1] != ' ') {
      running = false;
      end_game();
    }

    pthread_mutex_unlock(&m);
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

  pthread_t threads[4];
  args_thread_t args[4];

  for(int i = 0; i < 4; i++) {
    args[i].row = row;
    //printf("%d ",args[i].row);
  }

  if(pthread_create(&threads[0], NULL, generate_word, &args[0])) {
    perror("pthread_creates failed\n");
    exit(2);
  }
  
  if(pthread_create(&threads[1], NULL, draw_board, &args[1])) {
    perror("pthread_creates failed\n");
    exit(2);
  }
  
  if(pthread_create(&threads[2], NULL, compare_word, &args[2])) {
    perror("pthread_creates failed\n");
    exit(2);
  }

  if(pthread_create(&threads[3], NULL, move_word, &args[3])) {
    perror("pthread_creates failed\n");
    exit(2);
  }

  for(int i = 0; i < 4; i++) {
    if(pthread_join(threads[i], NULL)) {
      perror("pthread_join main failed\n");
      exit(2);
    }
  }

  return NULL;
}


// Entry point: Set up the game, create jobs, then run the scheduler
int main(void) {
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

  /*
    args_thread_t args[10];
    for(int i = 0; i < 10; i++) {
    args[i].row = i*2+3;
    printf("%d ",args[i].row);
    }
  */
  //count = malloc(sizeof(int));
  //*count = 0;

  // clean at first
  for(int i = 0; i < WORD_LEN; i++) {
    input[i] = ' ';
  }

  for (int i = 0; i < ROW_NUM; i++) {
    for(int j = 0; j < WORD_LEN; j++) {
      on_screen[i][j] = ' ';
    }
  }
  
  pthread_t threads[10];
  args_thread_t args[10];
  for(int i = 0; i < 10; i++) {
    args[i].row = i;
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


  // Display the end of game message and wait for user input
  //end_game();
  
  // Clean up window
  delwin(mainwin);
  endwin();

  fclose(stream);
  return 0;
} // main


/* In main, we create 10 threads, each thread responsible for one row of words.
 * Each thread will generate word if a line is empty, move and compare word simultaneously
 * Struct to keep the state of each row, if empty, generate word, if not move and compare
 */

