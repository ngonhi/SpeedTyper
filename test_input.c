#include <stdio.h>
#include <stdlib.h>
#include<time.h> 

#define WORD_LENGTH 50

// Entry point: Set up the game, create jobs, then run the scheduler
int main(void) {
  srand(time(0));
  // load the dictionary
  FILE *stream;

  stream = fopen("input.txt", "r");

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
  printf("file size = %zu\n", size);
  int offset = rand() % size;
  while(offset > size-8) {
    offset = rand() % size;
  }
  
  printf("offset = %d\n", offset);

  if(fseek(stream, offset, SEEK_SET) != 0) {
    perror("Unable to seek to offset");
    exit(2);
  }

  char c =fgetc(stream);
  while(c != '\n') {
    //printf("%c\n", c);
    offset++;
    c = fgetc(stream);
  }

  offset += 1; // Move to new word

  if(fseek(stream, offset, SEEK_SET) != 0) {
    perror("Unable to seek to offset");
    exit(2);
  }

  char* word = malloc(WORD_LENGTH);
  int j = 0;
  

  char ch = fgetc(stream);
  while(ch != '\n') {
    word[j] = ch;
    j++;
    printf("j = %d\n", j);
    ch = fgetc(stream);
  }

  word[j] = '\0';

  fclose(stream);

  printf("%s\n", word);

  return 0;
}
