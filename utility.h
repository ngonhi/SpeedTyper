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


#define WORD_LEN 50
#define ROW_NUM 10
#define BOARD_WIDTH 50
#define BOARD_HEIGHT 20 // Every two rows has one thread of words

extern char on_screen[ROW_NUM][WORD_LEN];
extern char board[BOARD_HEIGHT][BOARD_WIDTH];
extern FILE* stream;
extern bool running;
extern pthread_mutex_t m;

typedef struct args_thread{
  int row;
  //char * words; // List of words 
} args_thread_t;


void* generate_word(void* p);

void compare_word(char* input, int* count);

void* line_work(void* p);

