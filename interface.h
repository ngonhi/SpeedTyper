#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define WORD_LEN 50
#define ROW_NUM 10
#define BOARD_WIDTH 50
#define BOARD_HEIGHT 10 // Every two rows has one thread of words

// Game parameters
//#define WORM_HORIZONTAL_INTERVAL 700 // This will be word speed
#define DRAW_BOARD_INTERVAL 70

int WORM_HORIZONTAL_INTERVAL;

/**
 * Convert a board row number to a screen position
 * \param   row   The board row number to convert
 * \return        A corresponding row number for the ncurses screen
 */
int screen_row(int row);

/**
 * Convert a board column number to a screen position
 * \param   col   The board column number to convert
 * \return        A corresponding column number for the ncurses screen
 */
int screen_col(int col);

/**
 * Initialize the board display by printing the title and edges
 */
void init_display();


/**
 * Show a game over message and wait for a key press.
 */
void end_game();


// print menu
void user_menu() ;

void read_user_command();