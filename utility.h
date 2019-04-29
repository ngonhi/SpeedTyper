#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROW_NUM 10

extern char* on_screen[ROW_NUM];

void generate_word(FILE* stream, char* word);

void compare_word(char* input, int* count);

void* line_work(void* p);

