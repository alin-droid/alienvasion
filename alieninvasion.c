#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include "nava.h"
#include "aliens.h"
#include <stdio.h>

void init_ncurses() {
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);  
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
}


void close_ncurses() {
    endwin();
}


void print_title() {
    attron(A_BOLD | COLOR_PAIR(4)); 

    mvprintw(2, 10,  "    /\\      | |      | |    |  _____|  | |\\    | |");
    mvprintw(3, 10,  "   /  \\     | |      | |    |  |____   | | \\   | |");
    mvprintw(4, 10,  "  / /\\ \\   | |      | |    |  |____|  | |  \\  | |");
    mvprintw(5, 10,  " / ____ \\   | |___   | |    |  |____   | |   \\ | |");
    mvprintw(6, 10,  "/_/    \\_\\ |_____|  |_|    |_______|  |_|    \\| |"); 

    attroff(A_BOLD | COLOR_PAIR(4));  
}


void print_menu(int highlight) {
    char *choices[] = {"Single Player", "Multiplayer", "Exit"};
    int n_choices = sizeof(choices) / sizeof(choices[0]);

    for (int i = 0; i < n_choices; i++) {
        if (i == highlight)
            attron(A_REVERSE);
        mvprintw(12 + i * 2, 15, "%s", choices[i]);
        attroff(A_REVERSE);
    }
}


void single_player_mode() {
    clear();
    refresh();

    alien_t *aliens = malloc(NUMBER_OF_ALIENS * sizeof(alien_t));
    if (!aliens) {
        endwin();
        printf("Eroare: alocare memorie eșuată!\n");
        exit(1);
    }

    initialize_random();
    createAliens(aliens);
    draw_spaceship(COLS / 2 - 4, LINES - 7);
    printAliens(aliens);
    move_spaceship();

    getch();
    
    clear();
    refresh();
    close_ncurses();

    free(aliens);
}


int main() {
    int choice = 0;
    int key;

    init_ncurses();

    while (1) {
        clear(); 
        print_title();
        print_menu(choice);
        refresh();

        key = getch();
        switch (key) {
            case KEY_UP:
                choice = (choice == 0) ? 2 : choice - 1;
                break;
            case KEY_DOWN:
                choice = (choice == 2) ? 0 : choice + 1;
                break;
            case 10:  
                if (choice == 2) {  
                    close_ncurses();
                    return 0;
                } else if (choice == 0) {  
                    clear();
                    refresh();
                    single_player_mode();
                }
                break;
        }
        napms(50); 
    }

    close_ncurses();
    return 0;
}
