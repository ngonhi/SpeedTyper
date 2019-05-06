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


char input[50];

// helper to read user input
void read_input() {
  char ch = getch();
  printf("ch is %c\n", ch);
  int i = 0;
  while(ch != '\n') {
    //pthread_mutex_lock(&m);
    input[i] = ch;
    //pthread_mutex_unlock(&m);
    i++;
    ch = getch();
  }
}


int main(void) {
  //stream = fopen("input.txt", "r");
  //Initialize the ncurses window
  WINDOW* mainwin = initscr();
  if(mainwin == NULL) {
    fprintf(stderr, "Error initializing ncurses.\n");
    exit(2);
  }
  
  // Seed random number generator with the time in milliseconds
  //srand(time_ms());
  
  //noecho();               // Don't print keys when pressed
  keypad(mainwin, true);  // Support arrow keys
  //nodelay(mainwin, true); // Non-blocking keyboard access
  // Initialize the game display
  init_display();
  read_input();
  // Zero out the board contents
  //memset(board, ' ', BOARD_WIDTH*BOARD_HEIGHT*sizeof(char));
  // Clean up window
  delwin(mainwin);
  endwin();
  return 0;
}
