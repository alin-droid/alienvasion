#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "arms.h"
void init_arme(arma_t* arme) {
    arme[0].nume = "Pistol";
    arme[0].damage = 1;
    arme[0].viteza_gloanț = 5;
    arme[0].directii[0] = 2;  
    arme[0].gloanțe_disp = 30;

    arme[1].nume = "Machine Gun";
    arme[1].damage = 2;
    arme[1].viteza_gloanț = 8;
    arme[1].directii[0] = 2; 
    arme[1].gloanțe_disp = 50;

    arme[2].nume = "Rocket Launcher";
    arme[2].damage = 5;
    arme[2].viteza_gloanț = 3;
    arme[2].directii[0] = 2;  
    arme[2].gloanțe_disp = 10;

    arme[3].nume = "Crossbow";
    arme[3].damage = 3;
    arme[3].viteza_gloanț = 6;
    arme[3].directii[0] = 2; 
    arme[3].directii[1] = 0;
    arme[3].directii[2] = 1;  
    arme[3].gloanțe_disp = 20;

    arme[4].nume = "Laser";
    arme[4].damage = 4;
    arme[4].viteza_gloanț = 10;
    arme[4].directii[0] = 0;  
    arme[4].directii[1] = 1;  
    arme[4].directii[2] = 2;  
    arme[4].directii[3] = 3;  
    arme[4].gloanțe_disp = 5;
}

void print_menu_arme(int highlight) {
    const char* arme[] = {"Pistol", "Machine Gun", "Rocket Launcher", "Crossbow", "Laser"};
    int n_arme = sizeof(arme) / sizeof(arme[0]);

    clear();
    mvprintw(1, 2, "Select your weapon:");
    for (int i = 0; i < n_arme; ++i) {
        if (highlight == i)
            attron(A_REVERSE);
        mvprintw(3 + i, 4, "%s", arme[i]);
        if (highlight == i)
            attroff(A_REVERSE);
    }
    refresh();
}

void afiseaza_arme(arma_t* arme) {
    clear();
    mvprintw(1, 2, "Available Weapons:");
    for (int i = 0; i < 5; ++i) {
        mvprintw(3 + i * 2, 4, "Name: %s", arme[i].nume);
        mvprintw(4 + i * 2, 6, "Damage: %d | Speed: %d | Ammo: %d", arme[i].damage, arme[i].viteza_gloanț, arme[i].gloanțe_disp);
    }
    refresh();
}

void foloseste_arma(arma_t* arma) {
    if (arma->gloanțe_disp > 0) {
        arma->gloanțe_disp--;
        mvprintw(LINES - 2, 2, "Fired %s! Remaining ammo: %d", arma->nume, arma->gloanțe_disp);
    } else {
        mvprintw(LINES - 2, 2, "No ammo left for %s!", arma->nume);
    }
    refresh();
}
#define MAX_PROJECTILES 100

projectile_t projectiles[MAX_PROJECTILES];

void init_projectiles() {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectiles[i].is_active = 0;
    }
}

void shoot_projectile(int x, int y, int damage, int direction, int color_pair, int *projectile_count, arma_t* arme) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].is_active == 0) {
            projectiles[i].x = x;
            projectiles[i].y = y;
            projectiles[i].damage = damage;
            projectiles[i].direction = direction;
            projectiles[i].color_pair = color_pair;
            projectiles[i].is_active = 1;
            (*projectile_count)++;
            int valid_direction = 0;
            for (int j = 0; j < 4; j++) {
                if (arme->directii[j] == direction) {
                    valid_direction = 1;
                    break;
                }
            }

            if (!valid_direction) {
                projectiles[i].is_active = 0;
                (*projectile_count)--;
            }

            break;
        }
    }
}

void move_projectiles() {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].is_active) {
            switch (projectiles[i].direction) {
                case 0:
                    projectiles[i].x--;
                    break;
                case 1:
                    projectiles[i].x++;
                    break;
                case 2: 
                    projectiles[i].y--;
                    break;
                case 3: 
                    projectiles[i].y++;
                    break;
            }

         
            if (projectiles[i].x < 0 || projectiles[i].x >= COLS || projectiles[i].y < 0 || projectiles[i].y >= LINES) {
                projectiles[i].is_active = 0;
            }
        }
    }
}

void draw_projectiles() {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].is_active) {
            attron(COLOR_PAIR(projectiles[i].color_pair));
            mvprintw(projectiles[i].y, projectiles[i].x, "*");
            attroff(COLOR_PAIR(projectiles[i].color_pair));
        }
    }
}

int check_projectile_collision(int x, int y, alien_t *aliens, int *alien_count) {
    for (int i = 0; i < NUMBER_OF_ALIENS; i++) {  
        if (aliens[i].isalive == ALIVE) {  
   
            int width = 3;  
            int height = 2; 
            
            if (aliens[i].type == 1) {
                width = 7;  
                height = 4;
            }
            
         
            if (x >= aliens[i].x && x < aliens[i].x + width &&
                y >= aliens[i].y && y < aliens[i].y + height) {
                
              
                aliens[i].health--;  
                
                if (aliens[i].health <= 0) {
                    aliens[i].isalive = DEAD; 
                    aliens[i].x = -100;  
                    aliens[i].y = -100;
                    (*alien_count)--;    
                }
                
                return 1;
            }
        }
    }
    return 0; 
}