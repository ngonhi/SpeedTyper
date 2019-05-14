#include <curses.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define WORD_LEN 50
#define ROW_NUM 10
#define BOARD_WIDTH 50
#define BOARD_HEIGHT 10

// Game parameters
#define DRAW_BOARD_INTERVAL 70 // update interval used in draw_board

int WORM_HORIZONTAL_INTERVAL; // speed to move word (larger the value, slower the speed)

/**
 * Convert a board row number to a screen position
 * \param   row   The board row number to convert
 * \return        A corresponding row number for the ncurses screen
 * Taken from Charlie Curtsinger's worm lab
 */
int screen_row(int row);

/**
 * Convert a board column number to a screen position
 * \param   col   The board column number to convert
 * \return        A corresponding column number for the ncurses screen
 * Taken from Charlie Curtsinger's worm lab
 */
int screen_col(int col);

/**
 * Initialize the board display by printing the title and edges
 * Modified from Charlie Curtsinger's worm lab
 */
void init_display();


/**
 * Show a game over message and wait for a key press.
 * Modified from Charlie Curtsinger's worm lab
 */
void end_game();


/**
 * Print out the command menu for the game.
 */
void user_menu() ;

/**
 * Recursively read user_input and support instructions.
 */
void read_user_command();
