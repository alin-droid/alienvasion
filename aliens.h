#ifndef ALIENS_H
#define ALIENS_H
#define ALIEN_MOVE_DELAY 600 
#define ALIEN_SPAWN_INTERVAL 5000
#define NUMBER_OF_ALIENS 30 
#define ALIVE 1
#define DEAD 0
#define MIN_DISTANCE 5 
#define MAX_HEALTH 3

#define BOSS_TYPE 5
#define BOSS_HEALTH 50
#define BOSS_DAMAGE 5

typedef struct {
    int x, y;
    int type;
    int health;
    int isalive;
    int move_timer;    
    int move_direction; 
} alien_t;

typedef struct {
    int x, y;
    int health;
    int isalive;
    int move_timer;
    int move_direction;
    int attack_timer;
    int attack_cooldown;
} boss_t;

void initialize_random();
void createAliens(alien_t* aliens);
void printAliens(alien_t* aliens);
void moveAliens(alien_t* aliens);
int check_alien_collision(alien_t* aliens, int ship_x, int ship_y);
void clear_screen();
void regenerate_aliens(alien_t* aliens, int* alien_count); 

void init_boss(boss_t* boss, int screenWidth);
void draw_boss(boss_t* boss);
void move_boss(boss_t* boss, int max_x);
void enhanced_move_boss(boss_t* boss, int max_x);
void boss_attack(boss_t* boss, int* projectile_count);
int check_boss_collision(int x, int y, boss_t* boss);
int check_ship_boss_collision(int ship_x, int ship_y, boss_t* boss);

#endif