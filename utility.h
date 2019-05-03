#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define WORD_LEN 50
#define ROW_NUM 10
#define BOARD_WIDTH 100
#define BOARD_HEIGHT 20 // Every two rows has one thread of words

extern char on_screen[ROW_NUM][WORD_LEN];
extern char board[BOARD_HEIGHT][BOARD_WIDTH];
extern FILE* stream;

void generate_word(FILE* stream, int row);

void compare_word(char* input, int* count);

void* line_work(void* p);

