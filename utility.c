#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <utility.h>

#define WORD_LEN 50
#define ROW_NUM 10

char* on_screen[ROW_NUM];

void generate_word(FILE* stream, char* word) {
  
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
    word[j] = ch;
    j++;
    ch = fgetc(stream);
  }

  word[j] = '\0';
}


void compare_word(char* input, int* count) {
  while(1) {
    input[0] = getchar();

    //compare the first char of words on_screen with input[0]
    int i = 0;
    for (; i < ROW_NUM; i++) {
      if(input[0] == on_screen[i][0]) {
        break;
      }
    }

    // lock
    int size = strlen(on_screen[i]); // does not include null terminator
  
    //once locked on, only compare that one
    char ch = getchar();
    int j = 1;
    int counter = 0;
    while (ch != '\n' && ch == on_screen[i][j]) {
      j++;
      counter++; // this might not work
      ch = getchar();
    }
    
    if (counter == size) {
      (*count)++;
    }
  }
}


int main(void) {
  FILE* stream = fopen("input.txt", "r");

  char word[WORD_LEN]



  return 0;
}

