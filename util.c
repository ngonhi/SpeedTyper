#define _GNU_SOURCE

#include <curses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

#include "util.h"
#include "interface.h"

/**
 * Sleep for a given number of milliseconds
 * \param   ms  The number of milliseconds to sleep for
 * Taken from Charlie Curtsinger's worm lab
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
 * Taken from Charlie Curtsinger's worm lab
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

/**
 * Check if the row is empty (no word) and return a boolean
 */
bool is_empty(int row) {
  // Lock board's lock before checking
  pthread_mutex_lock(&m);
  for (int col = 0; col < BOARD_WIDTH; col++) {
    if (board[row][col] != ' ') {
      // Release board's lock before returning
      pthread_mutex_unlock(&m);
      return false;
    }
  }
  // Release board's lock before returning
  pthread_mutex_unlock(&m);
  return true;
}

/**
 * Check if running (the global boolean) is true or false
 */
bool check_running() {
  // Lock running's lock before checking
  pthread_mutex_lock(&m_running);
  if (running) {
    // Release running's lock before returning
    pthread_mutex_unlock(&m_running);
    return true;
  } else {
    // Release running's lock before returning
    pthread_mutex_unlock(&m_running);
    return false;
  }
}

/**
 * Thread function that keeps checking if the global string (input) is empty
 *   Empty: randomly seek a position in the library file and update board and on_screen
 *   Not empty: waiting until the current row is empty
 */
void* generate_word(void* p) {
  // Unpack thread argument
  args_thread_t*args = p;
  int row = args->row;

  // Check if the game is still running
  while(check_running()) {
    // Check if there is no word stored in the current row
    if(is_empty(row)) {
      pthread_mutex_lock(&m);
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

      // Generate a random offset
      int offset = rand() % size;
      while(offset > size-8) { // (last word in input.txt is poison)
        offset = rand() % size;
      }

      // Seek to the position of the random offset in stream
      if(fseek(stream, offset, SEEK_SET) != 0) {
        perror("Unable to seek to offset");
        exit(2);
      }

      // Move the offset to the last char of the current line
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
      
      // Move the offset to the first char of the word in next line
      offset += 1;

      // Seek to the position of updated offset
      if(fseek(stream, offset, SEEK_SET) != 0) {
        perror("Unable to seek to offset");
        exit(2);
      }

      // Read the line and store the word in both board and on_screen
      char* string = NULL;
      size_t len = 0;
      ssize_t nread = getline(&string, &len, stream);
      // Error handling for getline
      if(nread == -1) {
        perror("cannot getline");
        exit(2);
      } else {
        for (int i = 0; i < nread-1; i++) {
          on_screen[row][i] = *(string+i);
          board[row][i] = *(string+i);
        }
      }
      // Add a null terminator in the end of the word for on_screen for later comparison using strcmp
      on_screen[row][nread-1] = '\0';
      // Release the lock for board and on_screen
      pthread_mutex_unlock(&m);
      // Free for calling getline
      free(string);
    }
    
  } // while running
  return NULL;
}

/**
 * Read user input into input (global char array).
 */
void read_input() {
  char ch;
  if((ch = getch()) == ERR) {
    perror("getch failed.");
    exit(2);
  } 
  int i = 0;
  
  // Stop reading when the user hit enter
  while(ch != '\n') {
    input[i] = ch;
    // Print the typing by the side of the game window
    mvaddch(screen_row(BOARD_HEIGHT/2), screen_col(BOARD_WIDTH + 3 + i), ch);
    i++;
    if((ch = getch()) == ERR) {
      perror("getch failed\n");
      exit(2);
    } 
  }
  // Add a null terminator in the end for strcmp in compare_word().
  input[i] = '\0';
}

/**
 * Thread function to compare input (global char array) with all words on the screen.
 */
void* compare_word() {
  // Check if the game is still running
  while (check_running()) {
    // Lock for input
    pthread_mutex_lock(&m_input);
    bool check = true;
    int row;
    // Decide whether to read user input
    if (input[0] == ' ') {
      read_input();
    }

    // Compare input with all the words stored in on_screen
    pthread_mutex_lock(&m);
    for(int i=0; i<ROW_NUM; i++) {
      if(strcmp(input, on_screen[i]) != 0) { // mismatch case
        check = false;
      } else if (strcmp(input, on_screen[i]) == 0) { // match case
        row = i;
        check = true;
        break;
      }
    }
    pthread_mutex_unlock(&m);

    // Check if there is a match being found
    if (check) { // match found
      pthread_mutex_lock(&m_score);
      score++; // increment score
      pthread_mutex_unlock(&m_score);

      // Clear input
      for(int i=0; i<WORD_LEN; i++) {
        input[i] = ' ';
        mvaddch(screen_row(BOARD_HEIGHT/2), screen_col(BOARD_WIDTH + 3 + i), ' ');
      }

      // Clear on_screen and board
      pthread_mutex_lock(&m);
      for(int i = 0; i < WORD_LEN; i++) { 
        on_screen[row][i] = ' ';
      }
      // Replace word on screen and on board with spaces
      for(int i = 0; i < BOARD_WIDTH; i++) {
        board[row][i] = ' ';
      }
      pthread_mutex_unlock(&m);
    } else { // mismatch case
      for(int i = 0; i < WORD_LEN; i++) {
        input[i] = ' ';
        mvaddch(screen_row(BOARD_HEIGHT/2), screen_col(BOARD_WIDTH + 3 + i), ' ');
      }
    }
    // Update
    refresh();
    pthread_mutex_unlock(&m_input);
  } // while running
  return NULL;
}


/**
 * Thread function that prints out one row of the board on the screen.
 */
void* draw_row(void* p) {
  // Unpack
  args_thread_t* arg = p;
  int r = arg->row;
  
  // Run this function while the game is running
  while(check_running()) {
    pthread_mutex_lock(&m);
    // Print out each char in the row on the screen.
    for(int c=0; c<BOARD_WIDTH; c++) {
      mvaddch(screen_row(r), screen_col(c), board[r][c]);
    }
    // Update the score continuously
    pthread_mutex_lock(&m_score);
    mvprintw(screen_row(-2), screen_col(BOARD_WIDTH-9), "Score = %d", score); // Get the count
    pthread_mutex_unlock(&m_score);

    // Refresh the display
    refresh();
    pthread_mutex_unlock(&m);
    // Sleep for a while before drawing the row again
    sleep_ms(DRAW_BOARD_INTERVAL);
  }
  return NULL;
}

/**
 * Thread function that moves one row to the right
 */
void* move_row(void* p) {
  // Unpack
  args_thread_t* arg = p;
  int row = arg->row;
  // Buffer to store the current row of the board
  char temp[BOARD_WIDTH];
  // Run this function while the game is running
  while(check_running()) {
    pthread_mutex_lock(&m);
    // Update the buffer to the current row of the board
    for (int i=0; i < BOARD_WIDTH; i++) {
      temp[i] = board[row][i];
    }
    // Update the row of the board (move everything to right)
    board[row][0] = ' ';
    // Update the buffer
    for (int col=1; col < BOARD_WIDTH; col++) {
      board[row][col] = temp[col - 1];
    }
    // Check for edge collisions
    if(board[row][BOARD_WIDTH - 1] != ' ') {
      pthread_mutex_lock(&m_running);
      // Change the running boolean
      running = false;
      // Close the file
      if(fclose(stream)) {
        perror("Error closing file!");
        exit(2);
      }
      
      end_game();
      pthread_mutex_unlock(&m_running);
    }
    pthread_mutex_unlock(&m);
    // Sleep for a while before moving the row again
    sleep_ms(WORM_HORIZONTAL_INTERVAL); 
  } // while
  return NULL;
}

/**
 * Thread function that updates everything for one row
 */
void* run_game(void* p) {
  // Unpack
  args_thread_t* arg = p;
  int row = arg->row;
  // Start to print out word at a different time
  sleep_ms(interval[row]);

  // Declare sub-threads and thread arguments
  pthread_t threads[3];
  args_thread_t args[3];

  for(int i = 0; i < 3; i++) {
    args[i].row = row;
  }
  // Call generate thread function
  if(pthread_create(&threads[0], NULL, generate_word, &args[0])) {
    perror("pthread_creates failed\n");
    exit(2);
  }
  // Call draw_row thread function
  if(pthread_create(&threads[1], NULL, draw_row, &args[1])) {
    perror("pthread_creates failed\n");
    exit(2);
  }
  // Call move_row thread function
  if(pthread_create(&threads[2], NULL, move_row, &args[2])) {
    perror("pthread_creates failed\n");
    exit(2);
  }
  // Join all threads created
  for(int i = 0; i < 3; i++) {
    if(pthread_join(threads[i], NULL)) {
      perror("pthread_join main failed\n");
      exit(2);
    }
  }
  return NULL;
}


