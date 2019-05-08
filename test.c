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

int main() {
	int board[10][10];
	memset(board, 0, 10*10*sizeof(int));
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			printf("%d ", board[i][j]);
		}
		printf("\n");
	}

	printf("\n");

	memset(board[5], 10, 10*sizeof(int));
	for (int i = 0; i < 10; i++) {
		board[5][i] = 1;
	}
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			printf("%d ", board[i][j]);
		}
		printf("\n");
	}
}