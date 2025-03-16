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

    mvprintw(y, x, "<o>");
    mvprintw(y + 1, x, "/|\\");
    attroff(COLOR_PAIR(1));
}

void printAlien_type2(int x, int y)
{
    attron(COLOR_PAIR(2));
    mvprintw(y, x, "[#]");
    mvprintw(y + 1, x, "/ \\");
    attroff(COLOR_PAIR(2));
}

void printAlien_type3(int x, int y)
{
    attron(COLOR_PAIR(3));
    mvprintw(y, x, "{0}");
    mvprintw(y + 1, x, "/-\\");
    attroff(COLOR_PAIR(3));
}

void printAlien_type4(int x, int y)
{
    attron(COLOR_PAIR(4));
    mvprintw(y, x, "(^)");
    mvprintw(y + 1, x, "/V\\");
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
        if (dx < MIN_DISTANCE && dy < 2) // Evită suprapunerea
        {
            return 1;
        }
    }
    return 0;
}

void createAlien(alien_t* alien, alien_t* aliens, int index)
{
    int x, y;
    int max_y = (LINES * 2) / 3;  // Limitează extratereștrii la primele 2/3 din ecran

    do {
        x = rand() % (COLS - 5); // Ajustare pentru ca să încapă în ecran
        y = rand() % (max_y - 2); // Numai în primele 2/3 din ecran
    } while (check_collision(x, y, aliens, index));

    alien->x = x;
    alien->y = y;
    alien->type = rand() % 4 + 1;
    alien->health = rand() % MAX_HEALTH + 1;
    alien->isalive = ALIVE;
}

void createAliens(alien_t* aliens)
{
    for (int i = 0; i < NUMBER_OF_ALIENS; i++)
    {
        createAlien(&aliens[i], aliens, i);
    }
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
    refresh();
}
