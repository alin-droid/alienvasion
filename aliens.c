#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "aliens.h"

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
    // Define no-go zones for status panels
    int left_panel_width = 30;  // Width of left status panel
    int right_panel_width = 30; // Width of right status panel
    int panel_height = 8;       // Height of status panels
    
    if (alien->isalive == ALIVE && 
        alien->x >= 0 && alien->x < COLS && 
        alien->y >= 0 && alien->y < LINES - 4) {
        
        // Don't print aliens in panel areas
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
        // Skip the alien being checked and dead aliens
        if (i == index || aliens[i].isalive == DEAD) {
            continue;
        }
        
        // Define dimensions based on alien type
        int width1 = (aliens[i].type == 1) ? 7 : 3;
        int height1 = (aliens[i].type == 1) ? 4 : 2;
        
        // Calculate minimum spacing required between aliens
        int min_x_distance = width1 + 5;  // Increased spacing
        int min_y_distance = height1 + 3; // Increased spacing
        
        // Check for collision
        if (abs(aliens[i].x - x) < min_x_distance && 
            abs(aliens[i].y - y) < min_y_distance) {
            return 1; // Collision detected
        }
    }
    return 0; // No collision
}

void createAlien(alien_t* alien, alien_t* aliens, int index) {
    int x, y;
    int max_y = (LINES * 2) / 3;
    int attempts = 0;
    int collision = 1;
    int type = rand() % 4 + 1;
    int width = (type == 1) ? 7 : 3;
    
    // Define no-spawn zones for status panels
    int left_panel_width = 30;  // Width of left status panel
    int right_panel_width = 30; // Width of right status panel
    int panel_height = 8;       // Height of status panels
    
    // Try to find a non-overlapping position with a maximum number of attempts
    while (collision && attempts < 50) {
        x = rand() % (COLS - width - 10);  // Leave margin from right side
        y = rand() % (max_y - 10);         // Keep aliens away from the bottom
        
        // Make sure alien is not too close to screen edges
        if (x < 5) x = 5;
        if (y < 5) y = 5;
        
        // Check if in the no-spawn zone (top left panel)
        if (y < panel_height && x < left_panel_width) {
            collision = 1;
            attempts++;
            continue;
        }
        
        // Check if in the no-spawn zone (top right panel)
        if (y < panel_height && x > COLS - right_panel_width - width) {
            collision = 1;
            attempts++;
            continue;
        }
        
        // Check for collision with other aliens
        collision = check_collision(x, y, aliens, index);
        attempts++;
    }
    
    // If we couldn't find a non-colliding position after max attempts,
    // place the alien off-screen temporarily, it will be regenerated later
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
    // Initialize all aliens as dead first
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
        aliens[i].isalive = DEAD;
        aliens[i].x = -100;
        aliens[i].y = -100;
    }
    
    // Create aliens with proper spacing
    int created_count = 0;
    for (int i = 0; i < NUMBER_OF_ALIENS && created_count < 10; i++) {
        createAlien(&aliens[i], aliens, i);
        if (aliens[i].isalive == ALIVE) {
            created_count++;
        }
        
        // Add some jitter to random number generator
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
    // Define no-go zones for status panels
    int left_panel_width = 30;  // Width of left status panel
    int right_panel_width = 30; // Width of right status panel
    int panel_height = 8;       // Height of status panels
    
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
        if (aliens[i].isalive == ALIVE) {
            aliens[i].move_timer += ALIEN_MOVE_DELAY;
            if (aliens[i].move_timer >= ALIEN_MOVE_DELAY) {
                aliens[i].move_timer = 0;
                
                // Save old position for collision testing
                int old_x = aliens[i].x;
                int old_y = aliens[i].y;
                aliens[i].x += aliens[i].move_direction;

                // Check for collision with screen edge
                if (aliens[i].x <= 0 || aliens[i].x >= COLS - 10) {
                    aliens[i].move_direction *= -1;  // Reverse direction
                    aliens[i].x = old_x;            // Restore position
                    aliens[i].y++;                  // Move down
                }
                
                // Check if alien would enter the left panel area
                if (aliens[i].y < panel_height && aliens[i].x < left_panel_width) {
                    aliens[i].x = old_x;
                    aliens[i].y = old_y;
                    aliens[i].move_direction *= -1; // Reverse direction
                }
                
                // Check if alien would enter the right panel area
                if (aliens[i].y < panel_height && aliens[i].x > COLS - right_panel_width - 10) {
                    aliens[i].x = old_x;
                    aliens[i].y = old_y;
                    aliens[i].move_direction *= -1; // Reverse direction
                }
                
                // Check for collision with other aliens
                if (check_collision(aliens[i].x, aliens[i].y, aliens, i)) {
                    aliens[i].x = old_x;           // Restore position
                    aliens[i].move_direction *= -1; // Reverse direction
                }
                
                // Keep aliens away from the status area at the bottom
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
            int ship_width = 6;  // Width of the spaceship
            int ship_height = 7; // Height of the spaceship
            
            // Check for overlap between alien and ship
            if (aliens[i].x < ship_x + ship_width &&
                aliens[i].x + alien_width > ship_x &&
                aliens[i].y < ship_y + ship_height &&
                aliens[i].y + alien_height > ship_y) {
                return 1; // Collision detected
            }
        }
    }
    return 0; // No collision
}

void regenerate_aliens(alien_t* aliens, int* alien_count) {
    static time_t last_spawn_time = 0;
    time_t current_time = time(NULL);

    if (current_time - last_spawn_time >= ALIEN_SPAWN_INTERVAL / 1000) {
        last_spawn_time = current_time;
        int spawned_count = 0;
        
        // Count current active aliens
        int active_aliens = 0;
        for (int i = 0; i < NUMBER_OF_ALIENS; i++) {
            if (aliens[i].isalive == ALIVE) {
                active_aliens++;
            }
        }
        
        // Only spawn new aliens if we're below the maximum
        if (active_aliens < 15) {  // Limit maximum aliens on screen
            for (int i = 0; i < NUMBER_OF_ALIENS && spawned_count < ALIENS_TO_REGENERATE; i++) {
                if (aliens[i].isalive == DEAD) {
                    createAlien(&aliens[i], aliens, i);
                    if (aliens[i].isalive == ALIVE) {  // Only count if successfully created
                        (*alien_count)++;
                        spawned_count++;
                    }
                }
            }
        }
    }
}