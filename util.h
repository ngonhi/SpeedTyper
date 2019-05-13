#ifndef UTIL_H
#define UTIL_H

#include <curses.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "list.h"
#include "interface.h"


extern char board[BOARD_HEIGHT][BOARD_WIDTH];
extern char on_screen[ROW_NUM][WORD_LEN];
extern pthread_mutex_t m;// board, on_screen

extern FILE* stream;

extern char input[WORD_LEN];
extern pthread_mutex_t m_input; // input

extern bool check_compare[ROW_NUM];
extern pthread_mutex_t m_compare; // check_compare

extern int score;
extern pthread_mutex_t m_score; // score

extern bool running;
extern pthread_mutex_t m_running; // running

extern int kill_thread;
extern pthread_cond_t cv; 
extern pthread_mutex_t lock_cv;

extern size_t interval[ROW_NUM];

extern linkedlist_t* list;

typedef struct args_thread{
  int row;
  //char * words; // List of words 
} args_thread_t;

// Sleep for a given number of milliseconds
void sleep_ms(size_t ms);

// Get the time in milliseconds since UNIX epoch
size_t time_ms();

void* generate_word(void* p);

void* compare_word(void* p);

void* draw_board(void* p);

void* move_word(void* p);

void* run_game(void* p);

void* check_thread(void* p);

#endif
