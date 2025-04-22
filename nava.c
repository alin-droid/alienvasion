#include <ncurses.h>
#include "nava.h"

#define POLE_STATE_RED 1
#define POLE_STATE_BLUE 2

typedef struct {
    int x, y;
    int height;
    int state;
    int is_flashing;
    int flash_timer;
} warning_pole_t;

warning_pole_t warning_pole;

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

void init_warning_pole(int x, int y) {
    warning_pole.x = x;
    warning_pole.y = y;
    warning_pole.height = 16;
    warning_pole.state = POLE_STATE_RED;
    warning_pole.is_flashing = 1;
    warning_pole.flash_timer = 0;
}

void draw_warning_pole() {
    int color_pair;
    
    if (warning_pole.state == POLE_STATE_RED) {
        color_pair = 1;
    } else {
        color_pair = 2;
    }
    
    attron(COLOR_PAIR(7) | A_BOLD);
    mvprintw(warning_pole.y + warning_pole.height, warning_pole.x - 2, "=========");
    mvprintw(warning_pole.y + warning_pole.height + 1, warning_pole.x, "=====");
    attroff(COLOR_PAIR(7) | A_BOLD);
    
    attron(COLOR_PAIR(7) | A_BOLD);
    for (int i = 1; i < warning_pole.height; i++) {
        mvprintw(warning_pole.y + i, warning_pole.x + 1, "||");
    }
    attroff(COLOR_PAIR(7) | A_BOLD);

    if (!warning_pole.is_flashing || (warning_pole.flash_timer < 15)) {
        attron(COLOR_PAIR(color_pair) | A_BOLD);
        mvprintw(warning_pole.y, warning_pole.x - 1, "[=====]");
        attroff(COLOR_PAIR(color_pair) | A_BOLD);
    }
    
    warning_pole.flash_timer = (warning_pole.flash_timer + 1) % 30;
}

void set_warning_pole_state(int state) {
    warning_pole.state = state;
    warning_pole.flash_timer = 0;  
    warning_pole.is_flashing = 1;
}
