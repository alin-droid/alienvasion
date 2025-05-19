#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "aliens.h"
#include "arms.h"

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
    
    int left_panel_width = 30;  
    int right_panel_width = 30; 
    int panel_height = 8;      
    
    if (alien->isalive == ALIVE && 
        alien->x >= 0 && alien->x < COLS && 
        alien->y >= 0 && alien->y < LINES - 4) {
        if (alien->y < panel_height && alien->x < left_panel_width) {
            return;
        }
        if (alien->y < panel_height && alien->x > COLS - right_panel_width) {
            return;
        }
        
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
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
       
        if (i == index || aliens[i].isalive == DEAD) {
            continue;
        }
        
       
        int width1 = (aliens[i].type == 1) ? 7 : 3;
        int height1 = (aliens[i].type == 1) ? 4 : 2;
        
        
        int min_x_distance = width1 + 5;  
        int min_y_distance = height1 + 3; 
        
        
        if (abs(aliens[i].x - x) < min_x_distance && 
            abs(aliens[i].y - y) < min_y_distance) {
            return 1; 
        }
    }
    return 0; 
}

void createAlien(alien_t* alien, alien_t* aliens, int index) {
    int x, y;
    int max_y = (LINES * 2) / 3;
    int attempts = 0;
    int collision = 1;
    int type = rand() % 4 + 1;
    int width = (type == 1) ? 7 : 3;
    
    
    int left_panel_width = 30;  
    int right_panel_width = 30; 
    int panel_height = 8;       
    
    while (collision && attempts < 50) {
        x = rand() % (COLS - width - 10);  
        y = rand() % (max_y - 10);      
        
       
        if (x < 5) x = 5;
        if (y < 5) y = 5;
      
        if (y < panel_height && x < left_panel_width) {
            collision = 1;
            attempts++;
            continue;
        }
    
        if (y < panel_height && x > COLS - right_panel_width - width) {
            collision = 1;
            attempts++;
            continue;
        }
        collision = check_collision(x, y, aliens, index);
        attempts++;
    }

    if (collision) {
        x = -100;
        y = -100;
        alien->isalive = DEAD;
    } else {
        alien->isalive = ALIVE;
    }

    alien->x = x;
    alien->y = y;
    alien->type = type;
    alien->health = rand() % MAX_HEALTH + 1;
    alien->move_timer = 0;
    alien->move_direction = (rand() % 2 == 0) ? 1 : -1;
}

void createAliens(alien_t* aliens) {
   
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
        aliens[i].isalive = DEAD;
        aliens[i].x = -100;
        aliens[i].y = -100;
    }
    
    int created_count = 0;
    for (int i = 0; i < NUMBER_OF_ALIENS && created_count < 10; i++) {
        createAlien(&aliens[i], aliens, i);
        if (aliens[i].isalive == ALIVE) {
            created_count++;
        }

        for (int j = 0; j < 10; j++) {
            rand();
        }
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
    int left_panel_width = 30; 
    int right_panel_width = 30;
    int panel_height = 8;       
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
        if (aliens[i].isalive == ALIVE) {
            aliens[i].move_timer += ALIEN_MOVE_DELAY;
            if (aliens[i].move_timer >= ALIEN_MOVE_DELAY) {
                aliens[i].move_timer = 0;
    
                int old_x = aliens[i].x;
                int old_y = aliens[i].y;
                aliens[i].x += aliens[i].move_direction;
                if (aliens[i].x <= 0 || aliens[i].x >= COLS - 10) {
                    aliens[i].move_direction *= -1;  
                    aliens[i].x = old_x;          
                    aliens[i].y++;                
                }
                
               
                if (aliens[i].y < panel_height && aliens[i].x < left_panel_width) {
                    aliens[i].x = old_x;
                    aliens[i].y = old_y;
                    aliens[i].move_direction *= -1; 
                }
                
                if (aliens[i].y < panel_height && aliens[i].x > COLS - right_panel_width - 10) {
                    aliens[i].x = old_x;
                    aliens[i].y = old_y;
                    aliens[i].move_direction *= -1; 
                }
                
                if (check_collision(aliens[i].x, aliens[i].y, aliens, i)) {
                    aliens[i].x = old_x;           
                    aliens[i].move_direction *= -1; 
                }
                
                if (aliens[i].y >= LINES - 8) {
                    aliens[i].y = LINES - 8;
                }
            }
        }
    }
}

int check_alien_collision(alien_t* aliens, int ship_x, int ship_y) {
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
        if (aliens[i].isalive == ALIVE) {
            int alien_width = (aliens[i].type == 1) ? 7 : 3;
            int alien_height = (aliens[i].type == 1) ? 4 : 2;
            int ship_width = 6;
            int ship_height = 7;
            
            if (aliens[i].x < ship_x + ship_width &&
                aliens[i].x + alien_width > ship_x &&
                aliens[i].y < ship_y + ship_height &&
                aliens[i].y + alien_height > ship_y) {
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
        
        int active_aliens = 0;
        for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
            if (aliens[i].isalive == ALIVE) {
                active_aliens++;
            }
        }
        
      
        if (active_aliens < 15) {  
            for (int i = 0; i < NUMBER_OF_ALIENS && spawned_count < ALIENS_TO_REGENERATE; i++) {
                if (aliens[i].isalive == DEAD) {
                    createAlien(&aliens[i], aliens, i);
                    if (aliens[i].isalive == ALIVE) {  
                        (*alien_count)++;
                        spawned_count++;
                    }
                }
            }
        }
    }
}
void createBoss(boss_t* boss) {
    boss->x = COLS / 2 - 5;
    boss->y = 5;
    boss->health = BOSS_HEALTH;
    boss->isalive = ALIVE;
    boss->move_timer = 0;
    boss->move_direction = 1;
}

void printBoss(boss_t* boss) {
    if (boss->isalive == ALIVE) {
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(boss->y, boss->x,     "  /XXXXX\\  ");
        mvprintw(boss->y + 1, boss->x, " |X  X  X| ");
        mvprintw(boss->y + 2, boss->x, "/XXXXXXXXX\\");
        mvprintw(boss->y + 3, boss->x, "\\X  XXX  X/");
        mvprintw(boss->y + 4, boss->x, " \\XXXXXXX/ ");
        attroff(COLOR_PAIR(1) | A_BOLD);
    }
}

void moveBoss(boss_t* boss) {
    if (boss->isalive == ALIVE) {
        boss->move_timer += 1;
        if (boss->move_timer >= 3) { // Mișcare mai lentă decât extratereștrii normali
            boss->move_timer = 0;
            boss->x += boss->move_direction;

            if (boss->x <= 0 || boss->x >= COLS - 11) {
                boss->move_direction *= -1;
            }
            
            // Mișcare aleatoare sus-jos ocazional
            if (rand() % 10 == 0) {
                int direction = (rand() % 2) * 2 - 1; // -1 sau 1
                boss->y += direction;
                
                // Limitează poziția pe verticală
                if (boss->y < 3) boss->y = 3;
                if (boss->y > LINES / 3) boss->y = LINES / 3;
            }
        }
    }
}

int check_boss_collision(boss_t* boss, int ship_x, int ship_y) {
    if (boss->isalive == ALIVE) {
        if (boss->x < ship_x + 7 &&
            boss->x + 11 > ship_x &&
            boss->y < ship_y + 7 &&
            boss->y + 5 > ship_y) {
            return 1;
        }
    }
    return 0;
}

int check_projectile_boss_collision(int x, int y, boss_t* boss) {
    if (boss->isalive == ALIVE) {
        if (x >= boss->x && x < boss->x + 11 &&
            y >= boss->y && y < boss->y + 5) {
            
            boss->health--;
            
            if (boss->health <= 0) {
                boss->isalive = DEAD;
                return 2; // 2 = boss eliminat
            }
            
            return 1; // 1 = hit
        }
    }
    return 0; // 0 = nicio coliziune
}