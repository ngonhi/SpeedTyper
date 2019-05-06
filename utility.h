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
extern char input[WORD_LEN];
extern int *count;

extern bool running;
extern pthread_mutex_t m;
extern pthread_mutex_t m2;

typedef struct args_thread{
  int row;
  //char * words; // List of words 
} args_thread_t;

bool is_empty(int row);

bool check_running();

void* generate_word(void* p);

void read_input();

void* compare_word(void* p);

void* line_work(void* p);

