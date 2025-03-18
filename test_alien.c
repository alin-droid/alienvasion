#include "aliens.h"

int main()
{
    initialize_random();
    init_ncurses();

    alien_t aliens[NUMBER_OF_ALIENS];

    createAliens(aliens);
    clear_screen();
    printAliens(aliens);

    getch(); // Așteaptă input pentru a putea vedea extratereștrii

    close_ncurses();
    return 0;
}
