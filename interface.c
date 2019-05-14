#include <stdio.h>
#include <stdlib.h>

#include "interface.h"
#include "util.h"

int WORM_HORIZONTAL_INTERVAL;

/**
 * Convert a board row number to a screen position
 * \param   row   The board row number to convert
 * \return        A corresponding row number for the ncurses screen
 * Taken from Charlie Curtsinger's worm lab
 */
int screen_row(int row) {
  return 2 + row;
}

/**
 * Convert a board column number to a screen position
 * \param   col   The board column number to convert
 * \return        A corresponding column number for the ncurses screen
 * Taken from Charlie Curtsinger's worm lab
 */
int screen_col(int col) {
  return 2 + col;
}

/**
 * Initialize the board display by printing the title and edges
 * Modified from Charlie Curtsinger's worm lab
 */
void init_display() {
  // Print Title Line
  move(screen_row(-2), screen_col(BOARD_WIDTH/2 - 9));
  addch(ACS_DIAMOND);
  addch(ACS_DIAMOND);
  printw(" SPEED TYPER! ");
  addch(ACS_DIAMOND);
  addch(ACS_DIAMOND);
  
  // Print corners
  mvaddch(screen_row(-1), screen_col(-1), ACS_ULCORNER);
  mvaddch(screen_row(-1), screen_col(BOARD_WIDTH), ACS_URCORNER);
  mvaddch(screen_row(BOARD_HEIGHT), screen_col(-1), ACS_LLCORNER);
  mvaddch(screen_row(BOARD_HEIGHT), screen_col(BOARD_WIDTH), ACS_LRCORNER);
  
  // Print top and bottom edges
  for(int col=0; col<BOARD_WIDTH; col++) {
    mvaddch(screen_row(-1), screen_col(col), ACS_HLINE);
    mvaddch(screen_row(BOARD_HEIGHT), screen_col(col), ACS_HLINE);
  }
  
  // Print left and right edges
  for(int row=0; row<BOARD_HEIGHT; row++) {
    mvaddch(screen_row(row), screen_col(-1), ACS_VLINE);
    mvaddch(screen_row(row), screen_col(BOARD_WIDTH), ACS_VLINE);
  }
  
  // Refresh the display
  refresh();
}

/**
 * Show a game over message and wait for a key press.
 * Modified from Charlie Curtsinger's worm lab
 */
void end_game() {
  // Clean up window before print out score
  for(int i = 0; i < BOARD_HEIGHT; i++) {
    for(int j = 0; j < BOARD_WIDTH; j++) {
      mvaddch(screen_row(i), screen_col(j), ' ');
    }
  }
    
  // Print out score for the player
  mvprintw(screen_row(BOARD_HEIGHT/2)-1, screen_col(BOARD_WIDTH/2)-6, "            ");
  mvprintw(screen_row(BOARD_HEIGHT/2),   screen_col(BOARD_WIDTH/2)-8, "Congratulations!");
  mvprintw(screen_row(BOARD_HEIGHT/2)+1, screen_col(BOARD_WIDTH/2)-10, "You earned %d points!", score);
  mvprintw(screen_row(BOARD_HEIGHT/2)+2, screen_col(BOARD_WIDTH/2)-9, "Hit Enter to exit.");
  
  refresh();
  timeout(-1);
}

/**
 * Print out the command menu for the game.
 */
void user_menu() {
  printf("Welcome to Speed Typer!\n");
  printf("Enter one of the following Commands:\n\n");
  printf("\tHelp - Instruction for the Speed Typer'\n");
  printf("\tPlay - Start the Speed Typer\n");
  printf("\tQuit - Exit the Speed Typer\n");
  printf("Enter Commands: ");
}

/**
 * Recursively read user_input and support instructions.
 */
void read_user_command(void) {
  // Read user input and store in buffer
  char command[WORD_LEN];

  int num = scanf("%s", command);
  // Error checking
  if(num == 0) {
    perror("scanf failed.");
    exit(2);
  }

  // Convert all input letters into lowercase
  for(int i = 0; command[i]; i++) {
    command[i] = tolower(command[i]);
  }

  // Respond to different commands
  if(strcmp(command, "help") == 0) { // help mode
    printf("\nSpeed Typer is a typing game aiming to provide users with interesting typing experience.\n");
    printf("\t1. Hit enter after all the letters of one word is typed.\n");
    printf("\t2. Hit enter when a typo happens to clear the input and retype.\n");
    printf("\t3. This game does not support backspace =(\n");
    printf("\t4. When a word reaches the edge of the other side, the game will end.\n");
    printf("Let us get our fingers moving and enjoy the game!\n\n");
    
    user_menu();
    // recursion
    read_user_command();
  } else if(strcmp(command, "play") == 0) { // play mode
    printf("Enter the speed level you would like to challenge (300 - 1000 decreasing speed): ");
    scanf("%d", &WORM_HORIZONTAL_INTERVAL);
  } else if(strcmp(command, "quit") == 0) { // quit game
    exit(0);
  } else { // invalid input
    printf("Invalid command. Try again!\nPlease enter command: ");
    read_user_command();
  }
}
