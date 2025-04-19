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
#define TOTAL_ALIENS_TO_KILL 30  // Total aliens to kill to win the game

// Function to draw table border
void draw_table(int y, int x, int height, int width) {
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + width - 1, ACS_URCORNER);
    mvaddch(y + height - 1, x, ACS_LLCORNER);
    mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
    
    for (int i = 1; i < width - 1; i++) {
        mvaddch(y, x + i, ACS_HLINE);
        mvaddch(y + height - 1, x + i, ACS_HLINE);
    }
    
    for (int i = 1; i < height - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + width - 1, ACS_VLINE);
    }
}

// Function to draw horizontal divider in table
void draw_table_divider(int y, int x, int width) {
    mvaddch(y, x, ACS_LTEE);
    for (int i = 1; i < width - 1; i++) {
        mvaddch(y, x + i, ACS_HLINE);
    }
    mvaddch(y, x + width - 1, ACS_RTEE);
}

// Function to get weapon description based on weapon name
void get_weapon_description(arma_t weapon, char* desc_buffer, int buffer_size) {
    if (strstr(weapon.nume, "Laser") != NULL) {
        strncpy(desc_buffer, "High precision weapon with moderate damage", buffer_size - 1);
    } else if (strstr(weapon.nume, "Plasma") != NULL) {
        strncpy(desc_buffer, "Powerful energy weapon with high damage", buffer_size - 1);
    } else if (strstr(weapon.nume, "Rocket") != NULL) {
        strncpy(desc_buffer, "Explosive projectile with area damage", buffer_size - 1);
    } else if (strstr(weapon.nume, "Machine") != NULL) {
        strncpy(desc_buffer, "Rapid fire weapon with low cooldown", buffer_size - 1);
    } else if (strstr(weapon.nume, "Crossbow") != NULL) {
        strncpy(desc_buffer, "Silent weapon with good accuracy", buffer_size - 1);
    } else {
        strncpy(desc_buffer, "Standard alien hunting weapon", buffer_size - 1);
    }
    desc_buffer[buffer_size - 1] = '\0'; // Ensure null termination
}

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
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    timeout(-1);
}

void close_ncurses() {
    endwin();
}

void print_title() {
    int title_y = 2;
    int title_x = 10;
    int title_height = 6;
    int title_width = 50;  // Increased width
    
    // Draw table around title
    attron(COLOR_PAIR(4));
    draw_table(title_y-1, title_x-2, title_height+2, title_width);
    attroff(COLOR_PAIR(4));
    
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
    
    int menu_y = 10;
    int menu_x = 10;
    int menu_width = 40;  // Increased width
    int menu_height = n_choices * 2 + 4;
    
    // Draw menu table
    attron(COLOR_PAIR(2));
    draw_table(menu_y-1, menu_x-1, menu_height, menu_width);
    attroff(COLOR_PAIR(2));
    
    // Table header
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(menu_y, menu_x, "Welcome to Alien Invasion!");
    mvprintw(menu_y+1, menu_x, "Please select a game mode:");
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    // Divider after header
    attron(COLOR_PAIR(2));
    draw_table_divider(menu_y+2, menu_x-1, menu_width);
    attroff(COLOR_PAIR(2));

    // Menu options
    for (int i = 0; i < n_choices; i++) {
        if (i == highlight) attron(A_REVERSE);
        mvprintw(menu_y + 3 + i * 2, menu_x + 10, "%s", choices[i]);
        attroff(A_REVERSE);
    }
}

void select_arme(arma_t *arme, arma_t *arme_selectate, int *num_arme_selectate) {
    int highlight = 0;
    *num_arme_selectate = 0;
    int total_weapons_to_select = 5; // Limit the player to 5 weapons total
    clear();

    int menu_height = NUM_ARME_DISPONIBILE + 8;
    int menu_width = 70;  // Wider table to fit all text
    int start_y = (LINES - menu_height) / 2;
    int start_x = (COLS - menu_width) / 2;

    while (*num_arme_selectate < total_weapons_to_select) {
        clear();
        
        // Draw weapons selection table
        attron(COLOR_PAIR(5));
        draw_table(start_y, start_x, menu_height, menu_width);
        attroff(COLOR_PAIR(5));
        
        // Table header
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(start_y + 1, start_x + (menu_width - 14) / 2, "WEAPON SELECTION");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        // Instructions row
        mvprintw(start_y + 3, start_x + 2, "Use UP/DOWN to navigate, ENTER to select");
        mvprintw(start_y + 4, start_x + 2, "Weapons selected: %d/%d", *num_arme_selectate, total_weapons_to_select);
        
        // Header divider
        attron(COLOR_PAIR(5));
        draw_table_divider(start_y + 5, start_x, menu_width);
        attroff(COLOR_PAIR(5));
        
        // Column headers
        attron(A_BOLD);
        mvprintw(start_y + 6, start_x + 2, "%-15s | %-10s | %s", "WEAPON", "DAMAGE", "DESCRIPTION");
        attroff(A_BOLD);
        
        // Column header divider
        attron(COLOR_PAIR(5));
        draw_table_divider(start_y + 7, start_x, menu_width);
        attroff(COLOR_PAIR(5));
        
        // Weapon list
        for (int i = 0; i < NUM_ARME_DISPONIBILE; i++) {
            char desc[40];
            get_weapon_description(arme[i], desc, sizeof(desc));
            
            if (i == highlight) 
                attron(A_REVERSE);
            
            mvprintw(start_y + 8 + i, start_x + 2, "%-15s | %-10d | %s", 
                     arme[i].nume, 
                     arme[i].damage,
                     desc);
                     
            if (i == highlight)
                attroff(A_REVERSE);
        }
        
        refresh();

        int key = getch();
        if (key == KEY_UP) highlight = (highlight - 1 + NUM_ARME_DISPONIBILE) % NUM_ARME_DISPONIBILE;
        if (key == KEY_DOWN) highlight = (highlight + 1) % NUM_ARME_DISPONIBILE;
        if (key == ENTER) {
            if (*num_arme_selectate < total_weapons_to_select) {
                arme_selectate[*num_arme_selectate] = arme[highlight];
                (*num_arme_selectate)++;
                
                // Visual feedback
                attron(COLOR_PAIR(5) | A_BOLD);
                mvprintw(start_y + menu_height - 2, start_x + 2, "Added %s! (%d/%d)", 
                        arme[highlight].nume, *num_arme_selectate, total_weapons_to_select);
                attroff(COLOR_PAIR(5) | A_BOLD);
                refresh();
                napms(500);
            }
        }
        
        // If we've selected all required weapons, show a message
        if (*num_arme_selectate >= total_weapons_to_select) {
            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(start_y + menu_height - 2, start_x + 2, "All weapons selected! Press any key to start.");
            attroff(COLOR_PAIR(3) | A_BOLD);
            refresh();
            getch();
        }
    }

    clear();
}

void draw_weapon_panel(arma_t *arme_selectate, int num_arme_selectate, int current_weapon) {
    int panel_width = 25;
    int panel_height = 6;
    int start_x = 2;
    int start_y = 1;
    
    // Draw panel table
    attron(COLOR_PAIR(6));
    draw_table(start_y, start_x, panel_height, panel_width);
    attroff(COLOR_PAIR(6));
    
    // Panel title
    attron(COLOR_PAIR(6) | A_BOLD);
    mvprintw(start_y + 1, start_x + 2, "CURRENT WEAPON");
    
    // Title divider
    draw_table_divider(start_y + 2, start_x, panel_width);
    attroff(COLOR_PAIR(6) | A_BOLD);
    
    // Current weapon info
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(start_y + 3, start_x + 2, "%-15s", arme_selectate[current_weapon].nume);
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    mvprintw(start_y + 4, start_x + 2, "Damage: %d", arme_selectate[current_weapon].damage);
    mvprintw(start_y + 5, start_x + 2, "Ammo: %d", arme_selectate[current_weapon].gloanțe_disp);
}

void draw_game_status(int aliens_killed, int alien_count) {
    int status_width = 50;
    int status_height = 5;
    int start_x = COLS - status_width - 2;  // Align to right side with small margin
    int start_y = 1;
    
    // Draw status table
    attron(COLOR_PAIR(5));
    draw_table(start_y, start_x, status_height, status_width);
    attroff(COLOR_PAIR(5));
    
    // Status title
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(start_y + 1, start_x + (status_width - 11)/2, "GAME STATUS");
    
    // Title divider
    draw_table_divider(start_y + 2, start_x, status_width);
    attroff(COLOR_PAIR(5) | A_BOLD);
    
    // Status info
    attron(COLOR_PAIR(5));
    mvprintw(start_y + 3, start_x + 5, "ALIENS KILLED: %d/%d    ALIENS ACTIVE: %d", 
             aliens_killed, TOTAL_ALIENS_TO_KILL, alien_count);
    attroff(COLOR_PAIR(5));
    
    // Progress bar
    int progress_width = status_width - 10;
    int filled = (aliens_killed * progress_width) / TOTAL_ALIENS_TO_KILL;
    
    mvprintw(start_y + 4, start_x + 5, "[");
    for (int i = 0; i < progress_width; i++) {
        if (i < filled)
            addch('|');
        else
            addch(' ');
    }
    printw("]");
}

void draw_controls_panel() {
    int controls_width = 70;
    int start_x = (COLS - controls_width) / 2;
    int start_y = LINES - 2;
    
    // Controls info
    attron(A_DIM);
    mvprintw(start_y, start_x, "ENTER: change weapon | SPACE: fire | ←→: move ship | Q: quit");
    attroff(A_DIM);
}
void single_player_mode() {
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
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

    // Define status area height and top panel height
    int status_area_height = 3;
    int panel_height = 8;  // Height of the top panel
    
    // Position the ship above the green line (status area)
    int ship_x = COLS / 2 - 3;
    int ship_y = LINES - status_area_height - 8; // Position ship above the status area
    
    int current_weapon = 0;
    int alien_count = NUMBER_OF_ALIENS;
    int aliens_killed = 0;
    int game_over = 0;

    time_t last_alien_move = time(NULL);
    time_t last_alien_spawn = time(NULL);

    while (!game_over) {
        clear();

        // Draw weapon and status panels at the top
        draw_weapon_panel(arme_selectate, num_arme_selectate, current_weapon);
        draw_game_status(aliens_killed, alien_count);

        // Draw a boundary line for the status area at the bottom with GREEN color
        attron(COLOR_PAIR(5) | A_BOLD);
        for (int i = 0; i < COLS; i++) {
            mvaddch(LINES - status_area_height, i, ACS_HLINE);
        }
        attroff(COLOR_PAIR(5) | A_BOLD);

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
                    aliens_killed++;
                }
            }
        }

        // Draw game elements
        draw_projectiles();  
        printAliens(aliens);
        draw_spaceship(ship_x, ship_y);

        // Draw controls panel
        draw_controls_panel();

        // Show weapon info in bottom status area
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(LINES - status_area_height + 1, 2, "Arma: %s (Damage: %d, Ammo: %d)",
                 arme_selectate[current_weapon].nume,
                 arme_selectate[current_weapon].damage,
                 arme_selectate[current_weapon].gloanțe_disp);
        attroff(COLOR_PAIR(3) | A_BOLD);

        refresh();

        usleep(16666);  // ~60 FPS

        int ch = getch();
        switch (ch) {
            case KEY_LEFT:
                if (ship_x > 0) ship_x -= 1;
                break;
            case KEY_RIGHT:
                if (ship_x < COLS - 7) ship_x += 1;
                break;
            case KEY_UP:
                // Limit upward movement to not go too high
                if (ship_y > panel_height + 2) ship_y -= 1;
                break;
            case KEY_DOWN:
                // Limit downward movement to stay above the status area
                if (ship_y < LINES - status_area_height - 7) ship_y += 1;
                break;
            case ' ':
                if (current_weapon < num_arme_selectate && arme_selectate[current_weapon].gloanțe_disp > 0) {
                    arme_selectate[current_weapon].gloanțe_disp--;
                    shoot_projectile(
                        ship_x + 3,
                        ship_y - 1,
                        arme_selectate[current_weapon].damage,
                        2,  // Direction is up
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
                    }
                }
                break;
            case ENTER:
                if (num_arme_selectate > 0) {
                    current_weapon = (current_weapon + 1) % num_arme_selectate;
                }
                break;
            case 'q':
                game_over = 1;
                break;
        }

        // Check winning condition
        if (aliens_killed >= TOTAL_ALIENS_TO_KILL) {
            // Create a victory message box
            int msg_width = 40;
            int msg_height = 5;
            int msg_x = (COLS - msg_width) / 2;
            int msg_y = (LINES - msg_height) / 2;
            
            attron(COLOR_PAIR(3) | A_BOLD);
            draw_table(msg_y, msg_x, msg_height, msg_width);
            mvprintw(msg_y + 2, msg_x + (msg_width - 30)/2, "VICTORY! ALL ALIENS DESTROYED!");
            attroff(COLOR_PAIR(3) | A_BOLD);
            
            refresh();
            nodelay(stdscr, FALSE);
            getch();
            game_over = 1;
        }
        
        // Check ship collision with aliens
        if (check_alien_collision(aliens, ship_x, ship_y)) {
            // Create a game over message box
            int msg_width = 40;
            int msg_height = 5;
            int msg_x = (COLS - msg_width) / 2;
            int msg_y = (LINES - msg_height) / 2;
            
            attron(COLOR_PAIR(1) | A_BOLD);
            draw_table(msg_y, msg_x, msg_height, msg_width);
            mvprintw(msg_y + 2, msg_x + (msg_width - 30)/2, "GAME OVER! SHIP DESTROYED!");
            attroff(COLOR_PAIR(1) | A_BOLD);
            
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