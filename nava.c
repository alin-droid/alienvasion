#include <ncurses.h>
#include "nava.h"

void draw_spaceship(int x, int y) {
    mvprintw(y, x,    "  /\\  ");
    mvprintw(y+1, x,  " /  \\ ");
    mvprintw(y+2, x,  "|----|");
    mvprintw(y+3, x,  "| () |");
    mvprintw(y+4, x,  "| () |");
    mvprintw(y+5, x,  "| __ |");
    mvprintw(y+6, x,  "/_\\ /_\\");
    refresh();
}

void erase_spaceship(int x, int y) {
    for (int i = 0; i < 7; i++) {
        mvprintw(y + i, x, "      ");
    }
    refresh();
}

void move_spaceship() {
    int x = COLS / 2 - 3; 
    int y = LINES - 7;
    int ch;

    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    draw_spaceship(x, y);

    while ((ch = getch()) != 'q') {
        int new_x = x, new_y = y;

        switch (ch) {
            case KEY_LEFT:  if (x > 1) new_x--; break;
            case KEY_RIGHT: if (x < COLS - 7) new_x++; break; 
            case KEY_UP:    if (y > 1) new_y--; break;
            case KEY_DOWN:  if (y < LINES - 7) new_y++; break;
        }

        if (new_x != x || new_y != y) {
            erase_spaceship(x, y);
            x = new_x;
            y = new_y;
            draw_spaceship(x, y);
        }

        napms(50);
    }
}