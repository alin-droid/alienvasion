#include <ncurses.h>
#include "aliens.h"
#include<stdlib.h>

int main() {
    // Inițializează ncurses și generarea aleatorie
    init_ncurses();
    initialize_random();

    // Creează 80 de extratereștri
    alien_t* aliens = createAliens();

    // Afișează toți extratereștrii
    printAliens(aliens);

    // Așteaptă apăsarea unei taste pentru a închide
    refresh();
    getch();

    // Eliberează memoria
    freeAliens(aliens);

    // Închide ncurses
    close_ncurses();

    return 0;
}
