#ifndef ALIENS_H
#define ALIENS_H
#define ALIEN_MOVE_DELAY 600 
#define ALIEN_SPAWN_INTERVAL 5000
#define NUMBER_OF_ALIENS 50  // Modificat din 30 la 50 conform documentației
#define ALIVE 1
#define DEAD 0
#define MIN_DISTANCE 5 
#define MAX_HEALTH 3

// Definirea boss-ului
#define BOSS_HEALTH 10
#define BOSS_DAMAGE 3

typedef struct {
    int x, y;
    int type;
    int health;
    int isalive;
    int move_timer;    
    int move_direction; 
} alien_t;

// Structura pentru boss
typedef struct {
    int x, y;
    int health;
    int isalive;
    int move_timer;
    int move_direction;
} boss_t;

void initialize_random();
void createAliens(alien_t* aliens);
void printAliens(alien_t* aliens);
void moveAliens(alien_t* aliens);
int check_alien_collision(alien_t* aliens, int ship_x, int ship_y);
void clear_screen();
void regenerate_aliens(alien_t* aliens, int* alien_count); 
void handle_alien_hit(alien_t* alien); 

// Funcții pentru boss
void createBoss(boss_t* boss);
void printBoss(boss_t* boss);
void moveBoss(boss_t* boss);
int check_boss_collision(boss_t* boss, int ship_x, int ship_y);
int check_projectile_boss_collision(int x, int y, boss_t* boss);

#endif