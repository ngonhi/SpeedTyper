#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "utility.h"

bool running = true;

char on_screen[ROW_NUM][WORD_LEN];
char board[BOARD_HEIGHT][BOARD_WIDTH];
char input[WORD_LEN];
int *count;
FILE* stream;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

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
  
      int offset = rand() % size;
      while(offset > size-8) {
        offset = rand() % size;
      }

      if(fseek(stream, offset, SEEK_SET) != 0) {
        perror("Unable to seek to offset");
        exit(2);
      }

      char c =fgetc(stream);
      while(c != '\n') {
        offset++;
        c = fgetc(stream);
      }

      offset += 1; // Move to new word

      if(fseek(stream, offset, SEEK_SET) != 0) {
        perror("Unable to seek to offset");
        exit(2);
      }

      int j = 0;
  
      char ch = fgetc(stream);
      while(ch != '\n') {
        
        on_screen[row][j] = ch;
        board[row][j] = ch;
        j++;
        ch = fgetc(stream);
        
      }
    }
    pthread_mutex_unlock(&m);
  } //while running
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
  char ch = getchar();
  printf("ch is %c \n", ch);
  int i = 0;
  while(ch != '\n') {
    //pthread_mutex_lock(&m);
    input[i] = ch;
    //pthread_mutex_unlock(&m);
    i++;
    ch = getchar();
  }
}

bool check_empty(int row) {
  pthread_mutex_lock(&m);
  if (on_screen[row][0] != ' ') {
    pthread_mutex_unlock(&m);
    return true;
  } else {
    pthread_mutex_unlock(&m);
    return false;
  }
}

// put input word as a global
void* compare_word(void* p) {
  args_thread_t* arg = p;
  int row = arg->row;
  // maximum one word on each row

  // read the length of the string
  //int length = strlen(input);
  while(check_running()) {
    pthread_mutex_lock(&m2);
    // Input can only be changed by one thread
    // If input is empty
    if (input[0] == ' ') {
      read_input();
    }
    pthread_mutex_unlock(&m2);

    int i = 0;
    bool check = true;
    // Input can be read by multiple threads
    //pthread_mutex_lock(&m);
    while (check_empty(row)) {
      // Compare with words that are on_screen
      pthread_mutex_lock(&m);
      if (input[i] != on_screen[row][i]) {
        check = false;
        pthread_mutex_unlock(&m);
        break; // If input does not match on_screen at row
      }
      i++;
      pthread_mutex_unlock(&m);
    } // while compare 

    // Increase score and reset input
    if (check) {
      // Reset on_screen and board
      pthread_mutex_lock(&m);
      for (int i = 0; i < WORD_LEN; i++) {
        input[i] = ' ';
        on_screen[row][i] = ' ';
      }

      for (int i = 0; i < BOARD_WIDTH; i++) {
        board[row][i] = ' ';
      }
      //memset(on_screen[row], ' ', WORD_LEN*sizeof(char));
      //memset(board[row], ' ', BOARD_WIDTH*sizeof(char));
      //memset(input, ' ', WORD_LEN*sizeof(char));
      pthread_mutex_unlock(&m);
    }
  } // while running
  
  /*
  while(check_running()) {
    pthread_mutex_lock(&m2);
    if(input[0] != ' ') {
      read_input();
    }
    int i = 0;
    bool check = true;
    while(input[i] != ' ') {
      pthread_mutex_lock(&m);
      if(input[i] != on_screen[*row][i]) {
        check = false;
        pthread_mutex_unlock(&m);
        break;
      }
      i++;
      pthread_mutex_unlock(&m);
    }
    //pthread_mutex_unlock(&m);
    if(check) {
      pthread_mutex_lock(&m);
      // increase count
      *count++;
      // clear input
      memset(input, ' ', WORD_LEN);
      // delete word on screen and on board
      memset(board[*row], ' ', BOARD_WIDTH);
      memset(on_screen[*row], ' ', WORD_LEN);
      
      pthread_mutex_unlock(&m);
    }
    pthread_mutex_unlock(&m2);
  }*/

  return NULL;
}
