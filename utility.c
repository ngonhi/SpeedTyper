#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "utility.h"

bool running = true;

char on_screen[ROW_NUM][WORD_LEN];
char board[BOARD_HEIGHT][BOARD_WIDTH];
FILE* stream;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

// Check if row is empty (no word)
bool is_empty(int row) {
  for (int col = 0; col < BOARD_WIDTH; col++) {
    if (board[row][col] != ' ') {
      return false;
    }
  }

  return true;
}

void* generate_word(void* p) {
  args_thread_t*args = p;
  int row = args->row;

  srand(time(NULL));
  while(running) {
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
        pthread_mutex_lock(&m);
        on_screen[row][j] = ch;
        board[row][j] = ch;
        j++;
        ch = fgetc(stream);
        pthread_mutex_unlock(&m);
      }
    }
  } //while running
  return NULL;
}

/* helper for compare_word
void match_letter(FILE* stream, int i, int* j, int* counter) {
  char ch = getchar();
  while(ch != '\n' && ch == on_screen[i][*j]) {
    *j++;
    *counter++;
    ch = getchar();
  }
}


void compare_word(FILE* stream, char* input, int* count, int* row) {
  while(1) {
    input[0] = getchar();

    //compare the first char of words on_screen with input[0]
    int i = 0;
    for (; i < ROW_NUM; i++) {
      if(input[0] == on_screen[i][0]) {
        break;
      }
    }

    // Specify the row
    *row = i;

    // lock
    int size = strlen(on_screen[i]); // does not include null terminator
  
    //once locked on, only compare that one
    char ch = getchar();
    int j = 1;
    int counter = 0;

    // loop to check word until every char is matched
    while (counter != size) {
      match_letter(i, &j, &counter);
    }

    // Delete word from the board
    // block
    for (int col = 0; col < BOARD_WIDTH; col++) {
      board[i][col] = ' ';
      on_screen[i][col] = ' ';
    }

    generate_word(stream, i);

    
    // exit loop means completing match
    *count++;
  } // while(1)
}
*/
/*
int main() {
  FILE* stream;
  stream = fopen("small_input.txt", "r");
  generate_word(stream, 5);
  for(int i = 0; i < BOARD_HEIGHT; i++) {
    for(int j = 0; j < BOARD_WIDTH; j++) {
      printf("%c ", board[i][j]);
    }
    printf("\n");
  }
}*/
