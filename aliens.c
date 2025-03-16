#include "aliens.h"
#include <stdlib.h>
#include <time.h>

void initialize_random()
{
    srand(time(NULL));
}

void init_ncurses()
{
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
}

void close_ncurses()
{
    endwin();
}

void printAlien_type1(int x, int y)
{
    attron(COLOR_PAIR(1));
    mvprintw(y + 1, x, "*******");
    mvprintw(y + 2, x, "*  *  *");
    mvprintw(y + 3, x, "  ***  ");
    mvprintw(y + 4, x, "*     *");
    attroff(COLOR_PAIR(1));
}

void printAlien_type2(int x, int y)
{
    attron(COLOR_PAIR(2));
    mvprintw(y, x, "**  **");
    mvprintw(y + 1, x, "  **  ");
    mvprintw(y + 2, x, "  **  ");
    mvprintw(y + 3, x, "**  **");
    attroff(COLOR_PAIR(2));
}

void printAlien_type3(int x, int y)
{
    attron(COLOR_PAIR(3));
    mvprintw(y, x, " *     *");
    mvprintw(y + 1, x, "  *   *  ");
    mvprintw(y + 2, x, "   ***   ");
    mvprintw(y + 3, x, " *******");
    attroff(COLOR_PAIR(3));
}

void printAlien_type4(int x, int y)
{
    attron(COLOR_PAIR(4));
    mvprintw(y, x, "     *   ");
    mvprintw(y + 1, x, "   * * *  ");
    mvprintw(y + 2, x, " *   *   *");
    mvprintw(y + 3, x, "*         *");
    attroff(COLOR_PAIR(4));
}

void clear_screen()
{
    clear();
    refresh();
}

int check_collision(int x, int y, alien_t* aliens, int index)
{
    for (int i = 0; i < index; i++)
    {
        int dx = abs(aliens[i].x - x);
        int dy = abs(aliens[i].y - y);
        if (dx < MIN_DISTANCE && dy < MIN_DISTANCE)
        {
            return 1;
        }
    }
    return 0;
}

alien_t* createAlien(alien_t* aliens, int index)
{
    alien_t* alien = (alien_t*)malloc(sizeof(alien_t));

    int x, y;
    do {
        x = rand() % 100;
        y = rand() % 30;
    } while (check_collision(x, y, aliens, index));

    alien->x = x;
    alien->y = y;
    alien->type = rand() % 4 + 1;
    alien->health = rand() % MAX_HEALTH + 1;
    alien->isalive = rand() % 2;

    return alien;
}

alien_t* createAliens()
{
    alien_t* aliens = (alien_t*)malloc(NUMBER_OF_ALIENS * sizeof(alien_t));
    if (!aliens)
    {
        return NULL;
    }

    for (int i = 0; i < NUMBER_OF_ALIENS; i++)
    {
        aliens[i] = *createAlien(aliens, i);
    }
    return aliens;
}

void printAlien(alien_t* alien)
{
    if (alien->isalive == ALIVE)
    {
        switch (alien->type)
        {
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

void printAliens(alien_t* aliens)
{
    for (int i = 0; i < NUMBER_OF_ALIENS; i++)
    {
        printAlien(&aliens[i]);
    }
}

void freeAliens(alien_t* aliens)
{
    free(aliens);
}

void freeAlien(alien_t* alien)
{
    free(alien);
}
