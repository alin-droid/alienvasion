#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define WIDTH 100
#define HEIGHT 100
#define PORT 8080

void clear_screen() {
    printf("\033[2J\033[H");  
}

void print_centered(const char *str, int y) {
    int len = strlen(str);
    int x = (WIDTH - len) / 2;
    printf("\033[%d;%dH%s", y, x, str);  
}

void display_menu(int *option) {
    clear_screen();

    print_centered("Alien Invasion Game", 5);

    print_centered("[1] Single Player", 8);
    print_centered("[2] Multiplayer", 9);
    print_centered("[Q] Quit", 11);

    char choice;
    while (1) {
        choice = getchar();  // Așteaptă o apăsare de tastă
        if (choice == '1') {
            clear_screen();
            printf("\033[10;10HSingle Player Mode\n");
            break;  // Ieși din bucla de meniu
        } else if (choice == '2') {
            clear_screen();
            printf("\033[10;10HMultiplayer Mode\n");
            break;  // Ieși din bucla de meniu
        } else if (choice == 'q' || choice == 'Q') {
            clear_screen();
            printf("\033[10;10HExiting the game...\n");
            exit(0);  // Ieși din program
        }
    }
    *option = choice - '0';  // Convertește caracterul în număr
}

int main() {
    int option;
    display_menu(&option);
}
