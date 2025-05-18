#include "multiplayer.h"

int create_server() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    // Creează socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Setează opțiuni pentru socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        return -1;
    }
    
    // Pregătește adresa
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);
    
    // Leagă socket-ul la port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return -1;
    }
    
    // Ascultă pentru conexiuni
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return -1;
    }
    
    return server_fd;
}

void handle_connections(int server_socket) {
    int client_sockets[MAX_PLAYERS] = {0};
    int player_count = 0;
    game_state_t game_state;
    
    // Inițializează starea jocului
    memset(&game_state, 0, sizeof(game_state_t));
    game_state.player_count = 0;
    game_state.boss_health = 100;
    game_state.boss_alive = 1;
    game_state.pvp_phase = 0;
    game_state.game_over = 0;
    
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    fd_set readfds;
    
    while (player_count < MAX_PLAYERS) {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        
        int max_sd = server_socket;
        for (int i = 0; i < player_count; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }
        
        // Așteaptă activitate pe oricare dintre socket-uri
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("Select error");
            continue;
        }
        
        // Dacă este o nouă conexiune
        if (FD_ISSET(server_socket, &readfds)) {
            int new_socket;
            if ((new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept error");
                continue;
            }
            
            // Adaugă noul client în array
            client_sockets[player_count] = new_socket;
            
            // Alocă un ID pentru noul jucător
            packet_t welcome_packet;
            welcome_packet.type = PACKET_JOIN;
            welcome_packet.player.id = player_count;
            
            // Trimite ID-ul înapoi la client
            send_packet(new_socket, &welcome_packet);
            
            printf("New connection, socket fd: %d, ip: %s, port: %d, assigned id: %d\n", 
                   new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port), player_count);
            
            player_count++;
            game_state.player_count = player_count;
        }
    }
    
    printf("All players connected. Starting game...\n");
    
    // Bucla principală a jocului
    while (!game_state.game_over) {
        FD_ZERO(&readfds);
        
        // Adaugă socket-urile clienților la setul de citire
        int max_sd = 0;
        for (int i = 0; i < player_count; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }
        
        // Setează timeout pentru select
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;  // 10ms
        
        int activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);
        if (activity < 0 && errno != EINTR) {
            perror("Select error");
            continue;
        }
        
        // Verifică activitate pe socket-urile clienților
        for (int i = 0; i < player_count; i++) {
            int sd = client_sockets[i];
            
            if (FD_ISSET(sd, &readfds)) {
                packet_t packet;
                
                // Citește pachetul de la client
                if (receive_packet(sd, &packet) <= 0) {
                    // Client deconectat
                    printf("Player %d disconnected\n", i);
                    close(sd);
                    client_sockets[i] = 0;
                    
                    // Marchează jucătorul ca deconectat
                    game_state.players[i].isalive = 0;
                    continue;
                }
                
                // Procesează pachetul în funcție de tip
                switch (packet.type) {
                    case PACKET_PLAYER_POS:
                        // Actualizează poziția jucătorului
                        game_state.players[i].x = packet.player.x;
                        game_state.players[i].y = packet.player.y;
                        break;
                    
                    case PACKET_SHOT:
                        // Un jucător a tras
                        // În acest caz, data1 poate fi tipul armei, data2 poate fi direcția
                        // Retransmitem către toți ceilalți jucători
                        for (int j = 0; j < player_count; j++) {
                            if (j != i && client_sockets[j] > 0) {
                                send_packet(client_sockets[j], &packet);
                            }
                        }
                        break;
                    
                    case PACKET_ALIEN_HIT:
                        // Un jucător a lovit un extraterestru
                        // Actualizează scorul jucătorului
                        game_state.players[i].score += packet.data1;
                        break;
                    
                    case PACKET_BOSS_HIT:
                        // Un jucător a lovit boss-ul
                        // Actualizează viața boss-ului și scorul jucătorului
                        game_state.boss_health -= packet.data1;
                        game_state.players[i].score += packet.data1;
                        
                        // Verifică dacă boss-ul a fost învins
                        if (game_state.boss_health <= 0 && game_state.boss_alive) {
                            game_state.boss_alive = 0;
                            game_state.pvp_phase = 1;
                            
                            // Trimite pachet către toți jucătorii că începe faza PvP
                            packet_t pvp_packet;
                            pvp_packet.type = PACKET_PVP_START;
                            
                            for (int j = A; j < player_count; j++) {
                                if (client_sockets[j] > 0) {
                                    send_packet(client_sockets[j], &pvp_packet);
                                }
                            }
                        }
                        break;
                    
                    case PACKET_GAME_OVER:
                        // Un jucător a pierdut în faza PvP
                        game_state.players[i].isalive = 0;
                        
                        // Verifică dacă jocul s-a terminat
                        int alive_count = 0;
                        int last_alive = -1;
                        
                        for (int j = 0; j < player_count; j++) {
                            if (game_state.players[j].isalive) {
                                alive_count++;
                                last_alive = j;
                            }
                        }
                        
                        if (alive_count <= 1 && game_state.pvp_phase) {
                            game_state.game_over = 1;
                            
                            // Anunță câștigătorul
                            packet_t winner_packet;
                            winner_packet.type = PACKET_GAME_OVER;
                            winner_packet.data1 = last_alive;  // ID-ul câștigătorului
                            
                            for (int j = 0; j < player_count; j++) {
                                if (client_sockets[j] > 0) {
                                    send_packet(client_sockets[j], &winner_packet);
                                }
                            }
                        }
                        break;
                }
            }
        }
        
        // Trimite starea actualizată a jocului la toți jucătorii
        broadcast_state(client_sockets, &game_state);
        
        // O scurtă pauză pentru a nu supraîncărca CPU
        usleep(16666);  // ~60 FPS
    }
    
    // Închide toate conexiunile active
    for (int i = 0; i < player_count; i++) {
        if (client_sockets[i] > 0) {
            close(client_sockets[i]);
        }
    }
}

void broadcast_state(int sockets[], game_state_t *game_state) {
    packet_t state_packet;
    state_packet.type = PACKET_JOIN;  // Refolosim acest tip pentru a trimite starea jocului
    
    for (int i = 0; i < game_state->player_count; i++) {
        if (sockets[i] > 0) {
            // Copiază starea jocului în pachet
            memcpy(&state_packet.player, &game_state->players[i], sizeof(player_t));
            state_packet.data1 = game_state->boss_health;
            state_packet.data2 = game_state->pvp_phase;
            
            // Trimite starea jocului către acest jucător
            send_packet(sockets[i], &state_packet);
        }
    }
}

int send_packet(int socket, packet_t *packet) {
    return send(socket, packet, sizeof(packet_t), 0);
}

int receive_packet(int socket, packet_t *packet) {
    return recv(socket, packet, sizeof(packet_t), 0);
}