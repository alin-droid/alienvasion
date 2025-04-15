#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "aliens.h"
#include "arms.h"
#include "nava.h"

#define ENTER 10
#define NUM_ARME_DISPONIBILE 5
#define MAX_ARME_PER_SELECTIE 20
#define MAX_TOTAL_ARME_SELECTATE 80
#define MAX_PROJECTILE 100
#define ARME_PER_SELECTIE 4

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
    timeout(-1);
}

void close_ncurses() {
    endwin();
}

void print_title() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(2, 10, "  /\\     | |     | |   | _____|  | |\\   | |");
    mvprintw(3, 10, " /  \\    | |     | |   | |____   | | \\  | |");
    mvprintw(4, 10, "/ /\\ \\   | |     | |   | |____|  | |  \\ | |");
    mvprintw(5, 10, "/ ____ \\  | |___  | |   | |____   | |   \\| |");
    mvprintw(6, 10,"/_/    \\_\\ |_____| |_|   |_______|  |_|    \\| |");
    attroff(A_BOLD | COLOR_PAIR(4));
}

void print_menu(int highlight) {
    char *choices[] = {"Single Player", "Multiplayer", "Exit"};
    int n_choices = sizeof(choices) / sizeof(choices[0]);

    mvprintw(10, 10, "Welcome to Alien Invasion!");
    mvprintw(11, 10, "Please select a game mode:");

    for (int i = 0; i < n_choices; i++) {
        if (i == highlight) attron(A_REVERSE);
        mvprintw(12 + i * 2, 15, "%s", choices[i]);
        attroff(A_REVERSE);
    }
}

void select_arme(arma_t *arme, arma_t *arme_selectate, int *num_arme_selectate) {
    int highlight = 0;
    *num_arme_selectate = 0;
    int counts[NUM_ARME_DISPONIBILE] = {0};
    clear();

    while (*num_arme_selectate < MAX_TOTAL_ARME_SELECTATE) {
        clear();
        mvprintw(10, 10, "Selectează armele (SUS/JOS navighează, ENTER adaugă %d, 'q' termină)", ARME_PER_SELECTIE);
        for (int i = 0; i < NUM_ARME_DISPONIBILE; i++) {
            if (i == highlight) attron(A_REVERSE);
            mvprintw(13 + i * 2, 15, "%s (%d/%d)", arme[i].nume, counts[i], MAX_ARME_PER_SELECTIE);
            attroff(A_REVERSE);
        }
        refresh();

        int key = getch();
        if (key == 'q') break;
        if (key == KEY_UP) highlight = (highlight - 1 + NUM_ARME_DISPONIBILE) % NUM_ARME_DISPONIBILE;
        if (key == KEY_DOWN) highlight = (highlight + 1) % NUM_ARME_DISPONIBILE;
        if (key == ENTER) {
            int to_add = ARME_PER_SELECTIE;
            if (counts[highlight] + to_add > MAX_ARME_PER_SELECTIE) {
                to_add = MAX_ARME_PER_SELECTIE - counts[highlight];
            }
            if (*num_arme_selectate + to_add > MAX_TOTAL_ARME_SELECTATE) {
                to_add = MAX_TOTAL_ARME_SELECTATE - *num_arme_selectate;
            }
            for (int i = 0; i < to_add; i++) {
                arme_selectate[*num_arme_selectate] = arme[highlight];
                (*num_arme_selectate)++;
            }
            counts[highlight] += to_add;
        }
    }

    clear();
}

void single_player_mode() {
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    timeout(20);

    clear();
    refresh();

    arma_t arme[NUM_ARME_DISPONIBILE];
    arma_t arme_selectate[MAX_TOTAL_ARME_SELECTATE];
    int num_arme_selectate;
    int projectile_count = 0;

    init_arme(arme);
    select_arme(arme, arme_selectate, &num_arme_selectate);

    alien_t *aliens = malloc(NUMBER_OF_ALIENS * sizeof(alien_t));
    if (!aliens) {
        endwin();
        printf("Memory allocation error!\n");
        exit(1);
    }

    initialize_random();
    createAliens(aliens);
    init_projectiles();

    int ship_x = COLS / 2 - 3;
    int ship_y = LINES - 8;
    int current_weapon = 0;
    int alien_count = NUMBER_OF_ALIENS;
    int game_over = 0;

    time_t last_alien_move = time(NULL);
    time_t last_alien_spawn = time(NULL);

    while (!game_over) {
        clear();

        time_t current_time = time(NULL);
        if (current_time - last_alien_move >= ((double)ALIEN_MOVE_DELAY / 1000.0)) {
            moveAliens(aliens);
            last_alien_move = current_time;
        }

        if (current_time - last_alien_spawn >= ((double)ALIEN_SPAWN_INTERVAL / 1000.0)) {
            regenerate_aliens(aliens, &alien_count);
            last_alien_spawn = current_time;
        }

        move_projectiles();

        for (int i = 0; i < MAX_PROJECTILE; i++) {
            if (projectiles[i].is_active) {
                if (check_projectile_collision(projectiles[i].x, projectiles[i].y, aliens, &alien_count)) {
                    projectiles[i].is_active = 0;
                }
            }
        }

        draw_projectiles();
        draw_spaceship(ship_x, ship_y);
        printAliens(aliens);

        mvprintw(LINES - 2, 0, "Arma curentă: %s (Gloanțe: %d)",
                 arme_selectate[current_weapon].nume,
                 arme_selectate[current_weapon].gloanțe_disp);
        mvprintw(LINES - 1, 0, "Extratereștri rămași: %d", alien_count);

        refresh();

        usleep(16666);

        int ch = getch();
        switch (ch) {
            case KEY_LEFT:
                if (ship_x > 0) ship_x -= 1;
                break;
            case KEY_RIGHT:
                if (ship_x < COLS - 7) ship_x += 1;
                break;
            case ' ':
                if (current_weapon < num_arme_selectate && arme_selectate[current_weapon].gloanțe_disp > 0) {
                    arme_selectate[current_weapon].gloanțe_disp--;
                    shoot_projectile(
                        ship_x + 3,
                        ship_y - 1,
                        arme_selectate[current_weapon].damage,
                        2,
                        arme_selectate[current_weapon].damage % 4 + 1,
                        &projectile_count,
                        &arme_selectate[current_weapon]
                    );
                }
                break;
            case '1': case '2': case '3': case '4': case '5':
                {
                    int new_weapon = ch - '1';
                    if (new_weapon < num_arme_selectate) {
                        current_weapon = new_weapon;
                        mvprintw(LINES - 3, 0, "Schimbat pe arma %s", arme_selectate[current_weapon].nume);
                    }
                }
                break;
            case ENTER:
                if (num_arme_selectate > 0) {
                    current_weapon = (current_weapon + 1) % num_arme_selectate;
                    mvprintw(LINES - 3, 0, "Schimbat pe arma %s", arme_selectate[current_weapon].nume);
                }
                break;
            case 'q':
                game_over = 1;
                break;
        }

        if (alien_count <= 0) {
            mvprintw(LINES / 2, COLS / 2 - 10, "Victorie! Toți extratereștrii distruși.");
            refresh();
            nodelay(stdscr, FALSE);
            getch();
            game_over = 1;
        }
    }

    nodelay(stdscr, FALSE);
    free(aliens);
    timeout(-1);
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
        if (key == KEY_UP) choice = (choice - 1 + 3) % 3;
        if (key == KEY_DOWN) choice = (choice + 1) % 3;
        if (key == ENTER) {
            if (choice == 2) break;
            else if (choice == 0) single_player_mode();
        }
    }

    close_ncurses();
    return 0;
}