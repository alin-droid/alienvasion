#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "multiplayer.h"

// Constante pentru tipuri de pachete
#define PACKET_JOIN 1
#define PACKET_PLAYER_POS 2
#define PACKET_SHOT 3
#define PACKET_ALIEN_HIT 4
#define PACKET_BOSS_HIT 5
#define PACKET_PVP_START 6
#define PACKET_GAME_OVER 7

// Structura pentru packet-uri
typedef struct {
    int type;
    struct {
        int id;
        int x, y;
        int score;
        int isalive;
    } player;
    int data1;
    int data2;
} packet_t;

// Structura pentru starea jocului
typedef struct {
    int player_count;
    struct {
        int x, y;
        int score;
        int isalive;
    } players[MAX_PLAYERS];
    int boss_health;
    int boss_alive;
    int pvp_phase;
    int game_over;
} game_state_t;

// Prototipuri funcții
int create_server();
void handle_connections(int server_fd);
int send_packet(int socket, packet_t *packet);
int receive_packet(int socket, packet_t *packet);
void broadcast_state(int sockets[], game_state_t *game_state);

// Funcția main
int main() {
    printf("Pornire server Alien Invasion...\n");
    int server_fd = create_server();
    
    if (server_fd < 0) {
        fprintf(stderr, "Eroare la crearea serverului\n");
        return 1;
    }
    
    handle_connections(server_fd);
    
    close(server_fd);
    return 0;
}

// Creează un server socket și ascultă pentru conexiuni
int create_server() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    // Crearea socket-ului
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return -1;
    }
    
    // Setarea socket-ului pentru reutilizarea adresei
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        return -1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Atașarea socket-ului la portul specificat
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }
    
    // Ascultarea pentru conexiuni
    if (listen(server_fd, MAX_PLAYERS) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }
    
    printf("Server pornit. Așteaptă conexiuni pe portul %d...\n", PORT);
    
    return server_fd;
}

// Gestionarea conexiunilor de la clienți
void handle_connections(int server_fd) {
    fd_set read_fds;
    int client_sockets[MAX_PLAYERS] = {0};
    int max_sd, activity, sd, i;
    int player_count = 0;
    
    // Inițializarea stării jocului
    game_state_t game_state;
    memset(&game_state, 0, sizeof(game_state));
    game_state.player_count = 0;
    game_state.boss_health = 100;
    game_state.boss_alive = 1;
    game_state.pvp_phase = 0;
    game_state.game_over = 0;
    
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Buclă infinită pentru acceptarea conexiunilor
    while (1) {
        // Resetarea setului de file descriptors
        FD_ZERO(&read_fds);
        
        // Adăugarea socket-ului server la set
        FD_SET(server_fd, &read_fds);
        max_sd = server_fd;
        
        // Adăugarea socket-urilor client la set
        for (i = 0; i < MAX_PLAYERS; i++) {
            sd = client_sockets[i];
            
            if (sd > 0) {
                FD_SET(sd, &read_fds);
            }
            
            if (sd > max_sd) {
                max_sd = sd;
            }
        }
        
        // Așteptăm pentru activitate pe unul dintre socket-uri
        activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
        
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            continue;
        }
        
        // Dacă este activitate pe socket-ul server, este o nouă conexiune
        if (FD_ISSET(server_fd, &read_fds)) {
            int new_socket;
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                continue;
            }
            
            // Verificăm dacă putem adăuga încă un jucător
            if (player_count < MAX_PLAYERS) {
                // Găsim un slot liber
                for (i = 0; i < MAX_PLAYERS; i++) {
                    if (client_sockets[i] == 0) {
                        client_sockets[i] = new_socket;
                        printf("Jucător nou conectat, socket fd este %d, id: %d\n", new_socket, i);
                        
                        // Trimitem pachetul de bun venit
                        packet_t welcome_packet;
                        welcome_packet.type = PACKET_JOIN;
                        welcome_packet.player.id = player_count;
                        
                        // Trimitem datele inițiale
                        send_packet(new_socket, &welcome_packet);
                        
                        // Actualizăm contorul de jucători
                        player_count++;
                        game_state.player_count = player_count;
                        
                        break;
                    }
                }
            } else {
                printf("Serverul este plin, conexiune respinsă\n");
                close(new_socket);
            }
        }
        
        // Verificăm activitatea pe socket-urile client
        while (!game_state.game_over) {
            for (i = 0; i < MAX_PLAYERS; i++) {
                sd = client_sockets[i];
                
                if (FD_ISSET(sd, &read_fds)) {
                    // Verificăm dacă este închiderea conexiunii
                    packet_t packet;
                    memset(&packet, 0, sizeof(packet_t));
                    
                    // Primim packetul
                    if (receive_packet(sd, &packet) <= 0) {
                        // Jucătorul s-a deconectat
                        printf("Jucătorul %d s-a deconectat\n", i);
                        close(sd);
                        client_sockets[i] = 0;
                        
                        // Actualizăm starea jucătorului în joc
                        game_state.players[i].isalive = 0;
                        
                        continue;
                    }
                    
                    // Procesăm packetul în funcție de tipul său
                    switch (packet.type) {
                        case PACKET_PLAYER_POS:
                            // Actualizăm poziția jucătorului
                            game_state.players[i].x = packet.player.x;
                            game_state.players[i].y = packet.player.y;
                            break;
                            
                        case PACKET_SHOT:
                            // Trimitem packetul către toți ceilalți jucători
                            for (int j = 0; j < MAX_PLAYERS; j++) {
                                if (client_sockets[j] != 0 && j != i) {
                                    send_packet(client_sockets[j], &packet);
                                }
                            }
                            break;
                            
                        case PACKET_ALIEN_HIT:
                            // Actualizăm scorul jucătorului
                            game_state.players[i].score += packet.data1;
                            break;
                            
                        case PACKET_BOSS_HIT:
                            // Actualizăm sănătatea boss-ului
                            game_state.boss_health -= packet.data1;
                            game_state.players[i].score += packet.data1;
                            
                            // Verificăm dacă boss-ul a fost învins
                            if (game_state.boss_health <= 0 && game_state.boss_alive) {
                                game_state.boss_alive = 0;
                                game_state.pvp_phase = 1;
                                
                                // Anunțăm toți jucătorii că începe faza PvP
                                packet_t pvp_packet;
                                pvp_packet.type = PACKET_PVP_START;
                                
                                for (int j = 0; j < player_count; j++) {
                                    if (client_sockets[j] != 0) {
                                        send_packet(client_sockets[j], &pvp_packet);
                                    }
                                }
                            }
                            break;
                            
                        case PACKET_GAME_OVER:
                            // Marcăm jucătorul ca fiind mort
                            game_state.players[i].isalive = 0;
                            
                            // Verificăm dacă mai sunt jucători în viață
                            int alive_count = 0;
                            int last_alive = -1;
                            
                            for (int j = 0; j < MAX_PLAYERS; j++) {
                                if (game_state.players[j].isalive) {
                                    alive_count++;
                                    last_alive = j;
                                }
                            }
                            
                            // Dacă a mai rămas doar un jucător în viață și suntem în faza PvP, jocul s-a terminat
                            if (alive_count <= 1 && game_state.pvp_phase) {
                                game_state.game_over = 1;
                                
                                // Anunțăm câștigătorul
                                packet_t winner_packet;
                                winner_packet.type = PACKET_GAME_OVER;
                                winner_packet.data1 = last_alive;  // ID-ul câștigătorului
                                
                                for (int j = 0; j < MAX_PLAYERS; j++) {
                                    if (client_sockets[j] != 0) {
                                        send_packet(client_sockets[j], &winner_packet);
                                    }
                                }
                            }
                            break;
                    }
                }
            }
            
            // Broadcast starea jocului către toți jucătorii
            broadcast_state(client_sockets, &game_state);
            
            // Dacă jocul s-a terminat, ieșim din buclă
            if (game_state.game_over) {
                break;
            }
            
            // Adăugăm o mică pauză pentru a nu consuma prea multe resurse CPU
            usleep(50000);  // 50ms
        }
    }
}

// Trimitere packet
int send_packet(int socket, packet_t *packet) {
    return send(socket, packet, sizeof(packet_t), 0);
}

// Primire packet
int receive_packet(int socket, packet_t *packet) {
    return recv(socket, packet, sizeof(packet_t), 0);
}

// Broadcast stare joc
// Broadcast stare joc
void broadcast_state(int sockets[], game_state_t *game_state) {
    packet_t state_packet;
    
    // Creăm un pachet cu informații despre starea jocului
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (sockets[i] > 0) {
            // Trimitem informații despre toți jucătorii
            for (int j = 0; j < MAX_PLAYERS; j++) {
                if (game_state->players[j].isalive) {
                    state_packet.type = PACKET_PLAYER_POS;
                    
                    // Copiem membrii individual în loc să atribuim întreaga structură
                    state_packet.player.id = j;
                    state_packet.player.x = game_state->players[j].x;
                    state_packet.player.y = game_state->players[j].y;
                    state_packet.player.score = game_state->players[j].score;
                    state_packet.player.isalive = game_state->players[j].isalive;
                    
                    state_packet.data1 = game_state->boss_health;
                    state_packet.data2 = game_state->pvp_phase;
                    
                    send_packet(sockets[i], &state_packet);
                }
            }
        }
    }
}