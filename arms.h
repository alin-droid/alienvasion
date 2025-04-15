#ifndef ARME_H
#define ARME_H

#define MAX_PROJECTILES 100
#define MAX_GLOANTE 30
#define TOTAL_GLOANTE 80
#include "aliens.h"
typedef struct {
    char *nume;
    int damage;
    int viteza_gloanț;
    int directii[4];    // Direcțiile în care poate trage (0 = stânga, 1 = dreapta, 2 = sus, 3 = jos)
    int gloanțe_disp;  // Gloanțe disponibile
} arma_t;

typedef struct {
    int x, y;
    int damage;
    int direction; // 0 - st 1 - dr  2 - sus 3 - jos
    int color_pair;
    int is_active; // 1 - on 0 - off
} projectile_t;
extern projectile_t projectiles[MAX_PROJECTILES];
int check_projectile_collision(int x, int y, alien_t *aliens, int *alien_count);
void init_projectiles();
void move_projectiles();
void draw_projectiles();
void shoot_projectile(int x, int y, int damage, int direction, int color_pair, int *projectile_count, arma_t* arme);
int check_projectile_collision(int x, int y, alien_t *aliens, int *alien_count);
void init_projectiles(); 
void shoot_projectile(int x, int y, int damage, int direction, int color_pair, int *projectile_count, arma_t* arme); 
void move_projectiles();
void draw_projectiles(); 
int check_projectile_collision(int x, int y, alien_t *aliens, int *alien_count); 

void print_menu_arme(int highlight);
void afiseaza_arme(arma_t* arme);
void init_arme(arma_t* arme);
void foloseste_arma(arma_t* arma);

#endif

