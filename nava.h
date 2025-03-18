#ifndef NAVA_H
#define NAVA_H

#include <ncurses.h>


#define INIT_X  (COLS / 2 - 4)
#define INIT_Y  (LINES - 7)


#define MAX_LEFT  10
#define MAX_RIGHT 10
#define MAX_UP    5
#define MAX_DOWN  5


void draw_spaceship(int x, int y);
void erase_spaceship(int x, int y);
void move_spaceship();

#endif 
