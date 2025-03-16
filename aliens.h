#ifndef ALIENS_H
#define ALIENS_H

#include <ncurses.h>

#define MIN_DISTANCE 7
#define MAX_HEALTH 3
#define ALIVE 1
#define DEAD 0
#define NUMBER_OF_ALIENS 80

typedef struct {
    int x, y;
    int type;
    int health;
    int isalive;
} alien_t;

void initialize_random();
void init_ncurses();
void close_ncurses();
void clear_screen();

void printAlien_type1(int x, int y);
void printAlien_type2(int x, int y);
void printAlien_type3(int x, int y);
void printAlien_type4(int x, int y);

int check_collision(int x, int y, alien_t* aliens, int index);
alien_t* createAlien(alien_t* aliens, int index);
alien_t* createAliens();
void printAlien(alien_t* alien);
void printAliens(alien_t* aliens);
void freeAliens(alien_t* aliens);
void freeAlien(alien_t* alien);

#endif
