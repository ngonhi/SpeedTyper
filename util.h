#ifndef UTIL_H
#define UTIL_H

#include <curses.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "interface.h"


extern char board[BOARD_HEIGHT][BOARD_WIDTH];
extern char on_screen[ROW_NUM][WORD_LEN];
extern pthread_mutex_t m; // board, on_screen

extern FILE* stream;
extern WINDOW* mainwin;

extern char input[WORD_LEN];
extern pthread_mutex_t m_input; // input

extern int score;
extern pthread_mutex_t m_score; // score

extern bool running;
extern pthread_mutex_t m_running; // running


extern size_t interval[ROW_NUM];

// Thread argument structure
typedef struct args_thread{
  int row;
} args_thread_t;

/**
 * Sleep for a given number of milliseconds
 * \param   ms  The number of milliseconds to sleep for
 * Taken from Charlie Curtsinger's worm lab
 */
void sleep_ms(size_t ms);

/**
 * Get the time in milliseconds since UNIX epoch
 * Taken from Charlie Curtsinger's worm lab
 */
size_t time_ms();

/**
 * Thread function that keeps checking if the global string (input) is empty
 *   Empty: randomly seek a position in the library file and update board and on_screen
 *   Not empty: waiting until the current row is empty
 */
void* generate_word(void* p);

/**
 * Thread function to compare input (global char array) with all words on the screen.
 */
void* compare_word();

/**
 * Thread function that prints out one row of the board on the screen.
 */
void* draw_row(void* p);

/**
 * Thread function that moves one row to the right
 */
void* move_word(void* p);

/**
 * Thread function that updates everything for one row
 */
void* run_game(void* p);

#endif
