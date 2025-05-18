#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <ncurses.h>
#include "aliens.h"
#include "arms.h"
#include "nava.h"

#define SERVER_PORT 8888
#define BUFFER_SIZE 1024
#define MAX_PLAYERS 2
#define MAX_USERNAME_LEN 20

// Coduri pentru pachetele de date
#define PACKET_JOIN 1
#define PACKET_PLAYER_POS 2
#define PACKET_SHOT 3
#define PACKET_ALIEN_HIT 4
#define PACKET_BOSS_HIT 5
#define PACKET_BOSS_DEAD 6
#define PACKET_GAME_OVER 7
#define PACKET_PVP_START 8
#define PACKET_DISCONNECT 9
#define PACKET_RELOAD 10

// Structură pentru jucător
typedef struct {
    char username[MAX_USERNAME_LEN];
    int x, y;
    int health;
    int score;
    int isalive;
    int weapon_type;
    int ammo;
    int id;
} player_t;

// Structură pentru pachetele de date
typedef struct {
    int type;
    player_t player;
    int data1;  // Folosit pentru diferite scopuri
    int data2;  // Folosit pentru diferite scopuri
} packet_t;

// Structură pentru starea jocului
typedef struct {
    player_t players[MAX_PLAYERS];
    int player_count;
    int boss_health;
    int boss_alive;
    int pvp_phase;  // 0 = faza cooperativă, 1 = PvP
    int game_over;
} game_state_t;

// Funcții pentru server
int create_server();
void handle_connections(int server_socket);
void handle_client(int client_socket, game_state_t *game_state, int player_id);
void broadcast_state(int sockets[], game_state_t *game_state);

// Funcții pentru client
int connect_to_server(const char *server_ip);
int send_packet(int socket, packet_t *packet);
int receive_packet(int socket, packet_t *packet);

// Funcție pentru alegerea modului (host sau join)
int select_multiplayer_mode(char *server_ip);

// Funcție pentru introducerea numelui de utilizator
void get_username(char *username);

// Funcția principală pentru modul multiplayer
void multiplayer_mode();

// Funcții pentru gestionarea fazelor de joc
void coop_phase(int socket, player_t *player, alien_t *aliens, arma_t *arme_selectate, int num_arme_selectate);
void pvp_phase(int socket, player_t *player, arma_t *arme_selectate, int num_arme_selectate);

// Funcții pentru interfața grafică în multiplayer
void draw_multiplayer_hud(player_t *players, int player_count, int boss_health, int pvp_phase);
void draw_other_player(player_t *player);

#endif