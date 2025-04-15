#include <ncurses.h>
#include "aliens.h"
#include <stdlib.h>
#include <time.h>

#define ALIENS_TO_REGENERATE 4

void initialize_random() {
    srand(time(NULL));
}

void printAlien_type1(int x, int y)
{
    attron(COLOR_PAIR(1));

    mvprintw(y + 1, x,"*******");
    mvprintw(y + 2, x, "* * *");
    mvprintw(y + 3, x, "  *** " );
    mvprintw(y + 4, x, "* * ");

    attroff(COLOR_PAIR(1));
}

void printAlien_type2(int x, int y) {
    attron(COLOR_PAIR(2));
    mvprintw(y, x, "[ ]");
    mvprintw(y + 1, x, "/|\\");
    attroff(COLOR_PAIR(2));
}

void printAlien_type3(int x, int y) {
    attron(COLOR_PAIR(3));
    mvprintw(y, x, "{o}");
    mvprintw(y + 1, x, "/-\\");
    attroff(COLOR_PAIR(3));
}

void printAlien_type4(int x, int y) {
    attron(COLOR_PAIR(4));
    mvprintw(y, x, "(^)");
    mvprintw(y + 1, x, "/V\\");
    attroff(COLOR_PAIR(4));
}

void printAlien(alien_t* alien) {
    if (alien->isalive == ALIVE && alien->x >= 0 && alien->x < COLS && alien->y >= 0 && alien->y < LINES - 2) {
        switch (alien->type) {
            case 1:
                printAlien_type1(alien->x, alien->y);
                break;
            case 2:
                printAlien_type2(alien->x, alien->y);
                break;
            case 3:
                printAlien_type3(alien->x, alien->y);
                break;
            case 4:
                printAlien_type4(alien->x, alien->y);
                break;
            default:
                break;
        }
    }
}

int check_collision(int x, int y, alien_t* aliens, int index) {
    for (int i = 0; i < index; i++) {
        int dx = abs(aliens[i].x - x);
        int dy = abs(aliens[i].y - y);
        if (dx < MIN_DISTANCE && dy < 2) {
            return 1;
        }
    }
    return 0;
}

void createAlien(alien_t* alien, alien_t* aliens, int index) {
    int x, y;
    int max_y = (LINES * 2) / 3;

    do {
        x = rand() % (COLS - 7); 
        y = rand() % (max_y - 2);
    } while (check_collision(x, y, aliens, index));

    alien->x = x;
    alien->y = y;
    alien->type = rand() % 4 + 1;
    alien->health = rand() % MAX_HEALTH + 1;
    alien->isalive = ALIVE;
    alien->move_timer = 0; 
    alien->move_direction = (rand() % 2 == 0) ? 1 : -1; 
}

void createAliens(alien_t* aliens) {
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
        createAlien(&aliens[i], aliens, i);
    }
}

void printAliens(alien_t* aliens) {
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
        printAlien(&aliens[i]);
    }
    refresh();
}

void clear_screen() {
    clear();
}

void moveAliens(alien_t* aliens) {
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
        if (aliens[i].isalive == ALIVE) {
            aliens[i].move_timer += ALIEN_MOVE_DELAY;
            if (aliens[i].move_timer >= ALIEN_MOVE_DELAY) {
                aliens[i].move_timer = 0;
                aliens[i].x += aliens[i].move_direction;

                if (aliens[i].x <= 0 || aliens[i].x >= COLS - 7) {
                    aliens[i].move_direction *= -1;
                    aliens[i].y++;
                }
            }
        }
    }
}

int check_alien_collision(alien_t* aliens, int ship_x, int ship_y) {
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
        if (aliens[i].isalive == ALIVE) {
            if (aliens[i].x < ship_x + 7 &&
                aliens[i].x + 7 > ship_x &&
                aliens[i].y < ship_y + 4 &&
                aliens[i].y + 2 > ship_y) {
                return 1;
            }
        }
    }
    return 0;
}

void regenerate_aliens(alien_t* aliens, int* alien_count) {
    static time_t last_spawn_time = 0;
    time_t current_time = time(NULL);

    if (current_time - last_spawn_time >= ALIEN_SPAWN_INTERVAL / 1000) {
        last_spawn_time = current_time;
        int spawned_count = 0;
        for (int i = 0; i < NUMBER_OF_ALIENS && spawned_count < ALIENS_TO_REGENERATE; i++) {
            if (aliens[i].isalive == DEAD) {
                createAlien(&aliens[i], aliens, i);
                (*alien_count)++;
                spawned_count++;
            }
        }
    }
}