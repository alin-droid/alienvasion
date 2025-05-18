#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "aliens.h"
#include "arms.h"
#include "nava.h"

#define ENTER 10
#define NUM_ARME_DISPONIBILE 4
#define MAX_ARME_PER_SELECTIE 20
#define MAX_TOTAL_ARME_SELECTATE 80
#define MAX_PROJECTILE 100
#define ARME_PER_SELECTIE 4
#define TOTAL_ALIENS_TO_KILL 10

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

void draw_table_divider(int y, int x, int width) {
    mvaddch(y, x, ACS_LTEE);
    for (int i = 1; i < width - 1; i++) {
        mvaddch(y, x + i, ACS_HLINE);
    }
    mvaddch(y, x + width - 1, ACS_RTEE);
}

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
    desc_buffer[buffer_size - 1] = '\0';
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
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    timeout(-1);
}

void close_ncurses() {
    endwin();
}

void print_title() {
    int title_y = 2;
    int title_x = 10;
    int title_height = 6;
    int title_width = 50;
    
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
    int menu_width = 40;
    int menu_height = n_choices * 2 + 4;
    
    attron(COLOR_PAIR(2));
    draw_table(menu_y-1, menu_x-1, menu_height, menu_width);
    attroff(COLOR_PAIR(2));
    
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(menu_y, menu_x, "Welcome to Alien Invasion!");
    mvprintw(menu_y+1, menu_x, "Please select a game mode:");
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    attron(COLOR_PAIR(2));
    draw_table_divider(menu_y+2, menu_x-1, menu_width);
    attroff(COLOR_PAIR(2));

    for (int i = 0; i < n_choices; i++) {
        if (i == highlight) attron(A_REVERSE);
        mvprintw(menu_y + 3 + i * 2, menu_x + 10, "%s", choices[i]);
        attroff(A_REVERSE);
    }
}

void select_arme(arma_t *arme, arma_t *arme_selectate, int *num_arme_selectate) {
    int highlight = 0;
    *num_arme_selectate = 0;
    int total_weapons_to_select = 5;
    clear();

    int menu_height = NUM_ARME_DISPONIBILE + 8;
    int menu_width = 70;
    int start_y = (LINES - menu_height) / 2;
    int start_x = (COLS - menu_width) / 2;

    while (*num_arme_selectate < total_weapons_to_select) {
        clear();
        
        attron(COLOR_PAIR(5));
        draw_table(start_y, start_x, menu_height, menu_width);
        attroff(COLOR_PAIR(5));
        
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(start_y + 1, start_x + (menu_width - 14) / 2, "WEAPON SELECTION");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        mvprintw(start_y + 3, start_x + 2, "Use UP/DOWN to navigate, ENTER to select");
        mvprintw(start_y + 4, start_x + 2, "Weapons selected: %d/%d", *num_arme_selectate, total_weapons_to_select);
        
        attron(COLOR_PAIR(5));
        draw_table_divider(start_y + 5, start_x, menu_width);
        attroff(COLOR_PAIR(5));
        
        attron(A_BOLD);
        mvprintw(start_y + 6, start_x + 2, "%-15s | %-10s | %s", "WEAPON", "DAMAGE", "DESCRIPTION");
        attroff(A_BOLD);
        
        attron(COLOR_PAIR(5));
        draw_table_divider(start_y + 7, start_x, menu_width);
        attroff(COLOR_PAIR(5));
        
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
                
                attron(COLOR_PAIR(5) | A_BOLD);
                mvprintw(start_y + menu_height - 2, start_x + 2, "Added %s! (%d/%d)", 
                        arme[highlight].nume, *num_arme_selectate, total_weapons_to_select);
                attroff(COLOR_PAIR(5) | A_BOLD);
                refresh();
                napms(500);
            }
        }
        
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
    
    attron(COLOR_PAIR(6));
    draw_table(start_y, start_x, panel_height, panel_width);
    attroff(COLOR_PAIR(6));
    
    attron(COLOR_PAIR(6) | A_BOLD);
    mvprintw(start_y + 1, start_x + 2, "CURRENT WEAPON");
    
    draw_table_divider(start_y + 2, start_x, panel_width);
    attroff(COLOR_PAIR(6) | A_BOLD);
    
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(start_y + 3, start_x + 2, "%-15s", arme_selectate[current_weapon].nume);
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    mvprintw(start_y + 4, start_x + 2, "Damage: %d", arme_selectate[current_weapon].damage);
    mvprintw(start_y + 5, start_x + 2, "Ammo: %d", arme_selectate[current_weapon].gloanțe_disp);
}

void draw_game_status(int aliens_killed, int alien_count) {
    int status_width = 50;
    int status_height = 5;
    int start_x = COLS - status_width - 2;
    int start_y = 1;
    
    attron(COLOR_PAIR(5));
    draw_table(start_y, start_x, status_height, status_width);
    attroff(COLOR_PAIR(5));
    
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(start_y + 1, start_x + (status_width - 11)/2, "GAME STATUS");
    
    draw_table_divider(start_y + 2, start_x, status_width);
    attroff(COLOR_PAIR(5) | A_BOLD);
    
    attron(COLOR_PAIR(5));
    mvprintw(start_y + 3, start_x + 5, "ALIENS KILLED: %d/%d    ALIENS ACTIVE: %d", 
             aliens_killed, TOTAL_ALIENS_TO_KILL, alien_count);
    attroff(COLOR_PAIR(5));
    
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
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
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

    int status_area_height = 3;
    int panel_height = 8;
    
    int ship_x = COLS / 2 - 3;
    int ship_y = LINES - status_area_height - 8;
    
    init_warning_pole(COLS - 20, LINES - status_area_height - 16 - 5);
    
    boss_t boss;
    int boss_initialized = 0;
    
    int current_weapon = 0;
    int alien_count = NUMBER_OF_ALIENS;
    int aliens_killed = 0;
    int game_over = 0;
    int boss_phase = 0;

    time_t last_alien_move = time(NULL);
    time_t last_alien_spawn = time(NULL);

    while (!game_over) {
        clear();

        draw_weapon_panel(arme_selectate, num_arme_selectate, current_weapon);
        draw_game_status(aliens_killed, alien_count);

        attron(COLOR_PAIR(5) | A_BOLD);
        for (int i = 0; i < COLS; i++) {
            mvaddch(LINES - status_area_height, i, ACS_HLINE);
        }
        attroff(COLOR_PAIR(5) | A_BOLD);

        time_t current_time = time(NULL);
        
        if (boss_phase == 0) {
            if (current_time - last_alien_move >= ((double)ALIEN_MOVE_DELAY / 1000.0)) {
                moveAliens(aliens);
                last_alien_move = current_time;
            }

            if (current_time - last_alien_spawn >= ((double)ALIEN_SPAWN_INTERVAL / 1000.0)) {
                regenerate_aliens(aliens, &alien_count);
                last_alien_spawn = current_time;
            }
        }

        move_projectiles();

        for (int i = 0; i < MAX_PROJECTILE; i++) {
            if (projectiles[i].is_active) {
                if (boss_phase == 0 && check_projectile_collision(projectiles[i].x, projectiles[i].y, aliens, &alien_count)) {
                    projectiles[i].is_active = 0;
                    aliens_killed++;
                    
                    if (aliens_killed >= TOTAL_ALIENS_TO_KILL && boss_phase == 0) {
                        set_warning_pole_state(POLE_STATE_BLUE);
                        
                        for (int j = 0; j < NUMBER_OF_ALIENS; j++) {
                            aliens[j].isalive = DEAD;
                            aliens[j].x = -100;
                            aliens[j].y = -100;
                        }
                        alien_count = 0;
                        
                        clear();
                        
                        for (int j = 0; j < 5; j++) {
                            clear();
                            attron(COLOR_PAIR(5) | A_BOLD);
                            for (int k = 0; k < COLS; k++) {
                                mvaddch(LINES - status_area_height, k, ACS_HLINE);
                            }
                            attroff(COLOR_PAIR(5) | A_BOLD);
                            
                            draw_spaceship(ship_x, ship_y);
                            draw_warning_pole();
                            
                            attron(COLOR_PAIR(2) | A_BOLD | (j % 2 ? A_BLINK : 0));
                            mvprintw(LINES / 2 - 2, COLS / 2 - 25, "ALL NORMAL ALIENS HAVE BEEN ELIMINATED!");
                            attroff(A_BLINK);
                            mvprintw(LINES / 2, COLS / 2 - 30, "WARNING POLE SIGNALS THE ARRIVAL OF THE FINAL BOSS!");
                            mvprintw(LINES / 2 + 2, COLS / 2 - 12, "AMMUNITION HAS BEEN RELOADED");
                            attroff(COLOR_PAIR(2) | A_BOLD);
                            
                            refresh();
                            napms(800);
                        }
                        
                        clear();
                        draw_spaceship(ship_x, ship_y);
                        draw_warning_pole();
                        
                        attron(COLOR_PAIR(1) | A_BOLD | A_BLINK);
                        mvprintw(LINES / 2 - 5, COLS / 2 - 20, "!!! WARNING !!! WARNING !!! WARNING !!!");
                        attroff(A_BLINK);
                        mvprintw(LINES / 2 - 3, COLS / 2 - 18, "A POWERFUL PRESENCE DETECTED!");
                        mvprintw(LINES / 2 - 1, COLS / 2 - 10, "FINAL BOSS APPROACHING!");
                        mvprintw(LINES / 2 + 2, COLS / 2 - 18, "PREPARE FOR THE ULTIMATE BATTLE!");
                        attroff(COLOR_PAIR(1) | A_BOLD);
                        
                        refresh();
                        napms(2000);
                        
                        init_boss(&boss, COLS);
                        boss_initialized = 1;
                        
                        for (int j = 0; j < 10; j++) {
                            clear();
                            draw_spaceship(ship_x + (rand() % 3 - 1), ship_y + (rand() % 3 - 1));
                            draw_warning_pole();
                            
                            if (j > 5) {
                                boss.y = 12;
                                attron(COLOR_PAIR(1) | A_BOLD);
                                mvprintw(boss.y + 4, boss.x + 5, ">>> INCOMING <<<");
                                attroff(COLOR_PAIR(1) | A_BOLD);
                            }
                            
                            refresh();
                            napms(200);
                        }
                        
                        boss_phase = 1;
                        
                        clear();
                        draw_spaceship(ship_x, ship_y);
                        draw_warning_pole();
                        
                        attron(COLOR_PAIR(3) | A_BOLD);
                        mvprintw(LINES / 2, COLS / 2 - 15, "RELOADING WEAPONS...");
                        attroff(COLOR_PAIR(3) | A_BOLD);
                        refresh();
                        
                        for (int j = 0; j < num_arme_selectate; j++) {
                            for (int k = 0; k <= 30; k++) {
                                attron(COLOR_PAIR(3));
                                mvprintw(LINES / 2 + 2 + j, COLS / 2 - 25, "%s: [", arme_selectate[j].nume);
                                
                                for (int l = 0; l < 30; l++) {
                                    if (l < k) addch('|');
                                    else addch(' ');
                                }
                                
                                printw("] %d%%", (k * 100) / 30);
                                attroff(COLOR_PAIR(3));
                                refresh();
                                napms(10);
                            }
                            
                            arme_selectate[j].gloanțe_disp = 30;
                        }
                        
                        attron(COLOR_PAIR(3) | A_BOLD);
                        mvprintw(LINES / 2 + 2 + num_arme_selectate, COLS / 2 - 10, "RELOAD COMPLETE!");
                        attroff(COLOR_PAIR(3) | A_BOLD);
                        refresh();
                        napms(1000);
                    }
                }
                else if (boss_phase == 1 && boss_initialized && boss.isalive) {
                    int result = check_boss_collision(projectiles[i].x, projectiles[i].y, &boss);
                    if (result > 0) {
                        projectiles[i].is_active = 0;
                        
                        if (result == 2) {
                            aliens_killed++;
                        }
                    }
                }
            }
        }

        draw_projectiles();  
        
        if (boss_phase == 0) {
            printAliens(aliens);
        }
        
        draw_spaceship(ship_x, ship_y);
        draw_warning_pole();
        
        if (boss_phase == 1 && boss_initialized && boss.isalive) {
            enhanced_move_boss(&boss, COLS);
            boss_attack(&boss, &projectile_count);
            draw_boss(&boss);
        }

        draw_controls_panel();

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(LINES - status_area_height + 1, 2, "Weapon: %s (Damage: %d, Ammo: %d)",
                 arme_selectate[current_weapon].nume,
                 arme_selectate[current_weapon].damage,
                 arme_selectate[current_weapon].gloanțe_disp);
        attroff(COLOR_PAIR(3) | A_BOLD);

        refresh();

        usleep(16666);

        int ch = getch();
        switch (ch) {
            case KEY_LEFT:
                if (ship_x > 0) {
                    ship_x -= 1;
                }
                break;
            case KEY_RIGHT:
                if (ship_x < COLS - 7) {
                    ship_x += 1;
                }
                break;
            case KEY_UP:
                if (ship_y > panel_height + 2) {
                    ship_y -= 1;
                }
                break;
            case KEY_DOWN:
                if (ship_y < LINES - status_area_height - 7) {
                    ship_y += 1;
                }
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

        if (boss_phase == 0 && check_alien_collision(aliens, ship_x, ship_y)) {
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
        
        
        if (boss_phase == 1 && boss_initialized && boss.isalive) {
            if (check_ship_boss_collision(ship_x, ship_y, &boss)) {
                if (boss_phase == 1 && boss_initialized && boss.isalive) {
                    if (check_ship_boss_collision(ship_x, ship_y, &boss)) {
                        // Doar afișăm un mesaj că boss-ul a lovit nava, fără să încheiem jocul
                        attron(COLOR_PAIR(3) | A_BOLD);
                        mvprintw(ship_y - 2, ship_x - 5, "BOSS ATTACK!");
                        attroff(COLOR_PAIR(3) | A_BOLD);
                        
                        // Efect vizual pentru impact
                        for (int i = 0; i < 3; i++) {
                            attron(COLOR_PAIR(1 + i % 3));
                            mvaddch(ship_y - 1 + i % 3, ship_x + i % 3, '*');
                            mvaddch(ship_y - 1 + i % 3, ship_x + 6 - i % 3, '*');
                            attroff(COLOR_PAIR(1 + i % 3));
                            refresh();
                            napms(50);
                        }
                    }
                }
        }
        
        if (boss_phase == 1 && boss_initialized && !boss.isalive) {
            for (int i = 0; i < 5; i++) {
                clear();
                draw_spaceship(ship_x, ship_y);
                draw_warning_pole();
        
                attron(COLOR_PAIR(1 + i % 3) | A_BOLD);
                for (int r = i; r < 15; r += 2) {
                    for (int angle = 0; angle < 360; angle += 40) {
                        double rad = angle * 3.14159265 / 180.0;
                        int explosion_x = boss.x + 12 + (int)(r * cos(rad));
                        int explosion_y = boss.y + 4 + (int)(r * sin(rad) / 2);
                        if (explosion_x >= 0 && explosion_x < COLS && explosion_y >= 0 && explosion_y < LINES) {
                            mvaddch(explosion_y, explosion_x, '*');
                        }
                    }
                }
                attroff(COLOR_PAIR(1 + i % 3) | A_BOLD);
        
                refresh();
                napms(150);
            }
        
            for (int i = 0; i < 20; i++) {
                clear();
                draw_spaceship(ship_x, ship_y);
                draw_warning_pole();
        
                for (int p = 0; p < 30; p++) {
                    int px = boss.x + 12 + (p % 5) * (i + 1) * cos(p * 12);
                    int py = boss.y + 4 + (p % 5) * (i + 1) * sin(p * 12) / 2;
                    if (px >= 0 && px < COLS && py >= 0 && py < LINES - status_area_height) {
                        attron(COLOR_PAIR(p % 4 + 1));
                        mvaddch(py, px, ".*+o"[p % 4]);
                        attroff(COLOR_PAIR(p % 4 + 1));
                    }
                }
        
                refresh();
                napms(100);
            }
        
            clear();
        
            for (int i = 0; i < 5; i++) {
                clear();
        
                int msg_width = 60;
                int msg_height = 12;
                int msg_x = (COLS - msg_width) / 2;
                int msg_y = (LINES - msg_height) / 2;
        
                attron(COLOR_PAIR(3) | A_BOLD);
                draw_table(msg_y, msg_x, msg_height, msg_width);
        
                if (i % 2 == 0) attron(A_BLINK);
                mvprintw(msg_y + 2, msg_x + (msg_width - 20)/2, "COMPLETE VICTORY!");
                attroff(A_BLINK);
        
                mvprintw(msg_y + 4, msg_x + (msg_width - 40)/2, "THE FINAL BOSS HAS BEEN DESTROYED!");
                mvprintw(msg_y + 6, msg_x + (msg_width - 30)/2, "Earth has been saved!");
                mvprintw(msg_y + 8, msg_x + (msg_width - 45)/2, "Congratulations on completing your mission!");
                mvprintw(msg_y + 10, msg_x + 5, "Aliens eliminated: %d", aliens_killed);
                attroff(COLOR_PAIR(3) | A_BOLD);
        
                refresh();
                napms(500);
            }
        
            nodelay(stdscr, FALSE);
            mvprintw(LINES - 2, COLS / 2 - 15, "Press any key to continue");
            refresh();
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
            else if (choice == 1) multiplayer_mode();  // Modul multiplayer
        }
    }

    close_ncurses();
    return 0;
}
