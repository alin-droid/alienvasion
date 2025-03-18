#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#define NUMBER_OF_ALIENS 10
#define MAX_HEALTH 3
#define ALIVE 1
#define DEAD 0
#define MIN_DISTANCE 5

typedef struct {
    int x, y;
    int type;
    int health;
    int isalive;
} alien_t;

void initialize_random() {
    srand(time(NULL));
}

void printAlien_type1(int x, int y) {
    attron(COLOR_PAIR(1));
    mvprintw(y, x, "<o>");
    mvprintw(y + 1, x, "/|\\");
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
    mvprintw(y, x, "{0}");
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
        x = rand() % (COLS - 5);
        y = rand() % (max_y - 2);
    } while (check_collision(x, y, aliens, index));

    alien->x = x;
    alien->y = y;
    alien->type = rand() % 4 + 1;
    alien->health = rand() % MAX_HEALTH + 1;
    alien->isalive = ALIVE;
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
