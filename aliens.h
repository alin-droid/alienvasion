
#ifndef ALIENS_H
#define ALIENS_H
#define ALIEN_MOVE_DELAY 700 
#define ALIEN_SPAWN_INTERVAL 5000
#define NUMBER_OF_ALIENS 30 
#define ALIVE 1
#define DEAD 0
#define MIN_DISTANCE 5 
#define MAX_HEALTH 3
typedef struct {
    int x, y;
    int type;
    int health;
    int isalive;
    int move_timer;    
    int move_direction; 
} alien_t;

void initialize_random();
void createAliens(alien_t* aliens);
void printAliens(alien_t* aliens);
void moveAliens(alien_t* aliens);
int check_alien_collision(alien_t* aliens, int ship_x, int ship_y);
void clear_screen();
void regenerate_aliens(alien_t* aliens, int* alien_count); 
void handle_alien_hit(alien_t* alien); 
#endif