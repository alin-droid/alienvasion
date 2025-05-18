#include "multiplayer.h"

int connect_to_server(const char *server_ip) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    // Creează socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    
    // Convertește adresa IP din string în format binar
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }
    
    // Conectare la server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }
    
    return sock;
}

int select_multiplayer_mode(char *server_ip) {
    clear();
    
    int choice = 0;
    int max_choice = 2;
    
    while (1) {
        clear();
        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(5, COLS/2 - 10, "MULTIPLAYER MODE");
        attroff(COLOR_PAIR(4) | A_BOLD);
        
        // Afișează opțiunile
        for (int i = 0; i < max_choice; i++) {
            if (i == choice) {
                attron(A_REVERSE);
            }
            
            switch (i) {
                case 0:
                    mvprintw(8 + i*2, COLS/2 - 5, "Host Game");
                    break;
                case 1:
                    mvprintw(8 + i*2, COLS/2 - 5, "Join Game");
                    break;
            }
            
            if (i == choice) {
                attroff(A_REVERSE);
            }
        }
        
        refresh();
        
        int key = getch();
        
        switch (key) {
            case KEY_UP:
                choice = (choice - 1 + max_choice) % max_choice;
                break;
            case KEY_DOWN:
                choice = (choice + 1) % max_choice;
                break;
            case 10:  // Enter
                if (choice == 0) {
                    // Host Game
                    strcpy(server_ip, "127.0.0.1");
                    return 0;
                } else if (choice == 1) {
                    // Join Game
                    clear();
                    echo();
                    mvprintw(8, COLS/2 - 15, "Enter server IP: ");
                    refresh();
                    
                    char input[16];
                    getstr(input);
                    strcpy(server_ip, input);
                    
                    noecho();
                    return 1;
                }
                break;
        }
    }
    
    return -1;
}

void get_username(char *username) {
    clear();
    echo();
    mvprintw(8, COLS/2 - 20, "Enter your username (max 20 chars): ");
    refresh();
    
    getstr(username);
    
    // Asigură-te că username-ul nu depășește lungimea maximă
    username[MAX_USERNAME_LEN - 1] = '\0';
    
    noecho();
}

void draw_multiplayer_hud(player_t *players, int player_count, int boss_health, int pvp_phase) {
    int start_y = LINES - 5;
    
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(start_y, 2, "MULTIPLAYER MODE");
    
    if (pvp_phase) {
        mvprintw(start_y, COLS/2 - 10, "PVP PHASE - LAST PLAYER STANDING WINS!");
    } else {
        mvprintw(start_y, COLS/2 - 10, "COOPERATIVE PHASE - BOSS HEALTH: %d%%", boss_health);
    }
    
    for (int i = 0; i < player_count; i++) {
        mvprintw(start_y + 1 + i, 2, "Player %d: %s - Score: %d", 
                 i + 1, players[i].username, players[i].score);
    }
    
    attroff(COLOR_PAIR(3) | A_BOLD);
}

void draw_other_player(player_t *player) {
    attron(COLOR_PAIR(6) | A_BOLD);
    mvprintw(player->y, player->x,    "  /\\  ");
    mvprintw(player->y+1, player->x,  " /  \\ ");
    mvprintw(player->y+2, player->x,  "|----|");
    mvprintw(player->y+3, player->x,  "| () |");
    mvprintw(player->y+4, player->x,  "| () |");
    mvprintw(player->y+5, player->x,  "| __ |");
    mvprintw(player->y+6, player->x,  "/_\\ /_\\");
    attroff(COLOR_PAIR(6) | A_BOLD);
}

void multiplayer_mode() {
    // Inițializează culorile
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    
    timeout(20);  // Non-blocking getch
    
    char server_ip[16];
    int is_client = select_multiplayer_mode(server_ip);
    
    // Obține numele de utilizator
    char username[MAX_USERNAME_LEN];
    get_username(username);
    
    int socket_fd;
    
    if (is_client) {
        // Conectare la server
        mvprintw(10, COLS/2 - 15, "Connecting to server...");
        refresh();
        
        socket_fd = connect_to_server(server_ip);
        
        if (socket_fd < 0) {
            mvprintw(12, COLS/2 - 15, "Failed to connect to server!");
            refresh();
            getch();
            return;
        }
    } else {
        // Pornește serverul
        mvprintw(10, COLS/2 - 15, "Starting server...");
        refresh();
        
        int server_fd = create_server();
        
        if (server_fd < 0) {
            mvprintw(12, COLS/2 - 15, "Failed to start server!");
            refresh();
            getch();
            return;
        }
        
        // Fork-ul procesului pentru a rula serverul în background
        pid_t pid = fork();
        
        if (pid < 0) {
            mvprintw(12, COLS/2 - 15, "Failed to fork process!");
            refresh();
            getch();
            return;
        } else if (pid == 0) {
            // Procesul copil - serverul
            handle_connections(server_fd);
            exit(0);
        }
        
        // Procesul părinte - clientul
        mvprintw(12, COLS/2 - 15, "Server started. Connecting...");
        refresh();
        
        // Conectare la serverul local
        socket_fd = connect_to_server("127.0.0.1");
        
        if (socket_fd < 0) {
            mvprintw(14, COLS/2 - 15, "Failed to connect to local server!");
            refresh();
            getch();
            return;
        }
    }
    
    // Primește ID-ul de la server
    packet_t join_packet;
    join_packet.type = PACKET_JOIN;
    strncpy(join_packet.player.username, username, MAX_USERNAME_LEN);
    
    send_packet(socket_fd, &join_packet);
    
    if (receive_packet(socket_fd, &join_packet) <= 0) {
        mvprintw(14, COLS/2 - 15, "Failed to receive player ID!");
        refresh();
        getch();
        close(socket_fd);
        return;
    }
    
    int player_id = join_packet.player.id;
    
    mvprintw(14, COLS/2 - 15, "Connected! You are Player %d", player_id + 1);
    mvprintw(16, COLS/2 - 15, "Waiting for other players...");
    refresh();
    
    // Așteptăm ca toți jucătorii să se conecteze
    game_state_t game_state;
    memset(&game_state, 0, sizeof(game_state_t));
    
    while (game_state.player_count < MAX_PLAYERS) {
        if (receive_packet(socket_fd, &join_packet) <= 0) {
            mvprintw(18, COLS/2 - 15, "Connection lost!");
            refresh();
            getch();
            close(socket_fd);
            return;
        }
        
        // Actualizează starea jocului
        game_state.player_count = join_packet.player.id + 1;
        mvprintw(18, COLS/2 - 15, "Players connected: %d/%d", 
                 game_state.player_count, MAX_PLAYERS);
        refresh();
        
        napms(100);
    }
    
    mvprintw(20, COLS/2 - 15, "All players connected! Starting game...");
    refresh();
    napms(2000);
    
    // Inițializează jocul
    arma_t arme[NUM_ARME_DISPONIBILE];
    arma_t arme_selectate[MAX_TOTAL_ARME_SELECTATE];
    int num_arme_selectate;
    
    init_arme(arme);
    select_arme(arme, arme_selectate, &num_arme_selectate);
    
    alien_t *aliens = malloc(NUMBER_OF_ALIENS * sizeof(alien_t));
    if (!aliens) {
        endwin();
        printf("Memory allocation error!\n");
        exit(1);
    }
    
    initialize_random();
    createAliens(aliens);
    init_projectiles();
    
    // Structură pentru jucătorul local
    player_t local_player;
    strncpy(local_player.username, username, MAX_USERNAME_LEN);
    local_player.x = COLS / 2 - 3;
    local_player.y = LINES - 8;
    local_player.health = 100;
    local_player.score = 0;
    local_player.isalive = 1;
    local_player.weapon_type = 0;
    local_player.ammo = arme_selectate[0].gloanțe_disp;
    local_player.id = player_id;
    
    // Bucla principală a jocului
    int game_over = 0;
    int pvp_phase = 0;
    
    // Faza cooperativă
    while (!game_over && !pvp_phase) {
        coop_phase(socket_fd, &local_player, aliens, arme_selectate, num_arme_selectate);
        
        // Verifică dacă boss-ul a fost învins
        packet_t packet;
        if (receive_packet(socket_fd, &packet) > 0) {
            if (packet.type == PACKET_PVP_START) {
                pvp_phase = 1;
                clear();
                attron(COLOR_PAIR(3) | A_BOLD);
                mvprintw(LINES/2, COLS/2 - 25, "THE BOSS HAS BEEN DEFEATED! PVP PHASE STARTING!");
                attroff(COLOR_PAIR(3) | A_BOLD);
                refresh();
                napms(3000);
            }
        }
    }
    
    // Faza PvP
    if (pvp_phase) {
        // Resetăm poziția jucătorului pentru faza spațială
        local_player.x = COLS / 4 + player_id * (COLS / 2);
        local_player.y = LINES / 4;
        
        // Reîncărcăm armele
        for (int i = 0; i < num_arme_selectate; i++) {
            arme_selectate[i].gloanțe_disp = 30;
        }
        
        // Afișăm un mesaj de tranziție
        clear();
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(LINES/2 - 2, COLS/2 - 20, "ENTERING SPACE DUEL PHASE");
        mvprintw(LINES/2, COLS/2 - 30, "SHIPS ARE LAUNCHING INTO SPACE FOR THE FINAL BATTLE");
        mvprintw(LINES/2 + 2, COLS/2 - 15, "LAST PLAYER STANDING WINS!");
        attroff(COLOR_PAIR(1) | A_BOLD);
        refresh();
        napms(3000);
        
        // Bucla pentru faza PvP
        while (!game_over && local_player.isalive) {
            pvp_phase(socket_fd, &local_player, arme_selectate, num_arme_selectate);
        }
    }
    
    close(socket_fd);
    free(aliens);
}

void coop_phase(int socket, player_t *player, alien_t *aliens, arma_t *arme_selectate, int num_arme_selectate) {
    int current_weapon = 0;
    int projectile_count = 0;
    packet_t packet;
    
    clear();
    
    // Desenează elemente de joc
    draw_spaceship(player->x, player->y);
    printAliens(aliens);
    
    // Primește actualizări de la server
    if (receive_packet(socket, &packet) > 0) {
        // Actualizează starea jocului în funcție de tipul pachetului
        
        // Afișează alți jucători
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (i != player->id && packet.player.isalive) {
                draw_other_player(&packet.player);
            }
        }
        
        // Afișează HUD
        player_t players[MAX_PLAYERS];
        players[player->id] = *player;
        players[1 - player->id] = packet.player;
        
        draw_multiplayer_hud(players, MAX_PLAYERS, packet.data1, packet.data2);
    }
    
    // Procesează input de la tastatură
    int ch = getch();
    switch (ch) {
        case KEY_LEFT:
            if (player->x > 0) player->x--;
            break;
        case KEY_RIGHT:
            if (player->x < COLS - 7) player->x++;
            break;
        case KEY_UP:
            if (player->y > 10) player->y--;
            break;
        case KEY_DOWN:
            if (player->y < LINES - 10) player->y++;
            break;
        case ' ':
            if (current_weapon < num_arme_selectate && arme_selectate[current_weapon].gloanțe_disp > 0) {
                arme_selectate[current_weapon].gloanțe_disp--;
                shoot_projectile(
                    player->x + 3,
                    player->y - 1,
                    arme_selectate[current_weapon].damage,
                    2,
                    arme_selectate[current_weapon].damage % 4 + 1,
                    &projectile_count,
                    &arme_selectate[current_weapon]
                );
                
                // Trimite pachet pentru tragere la server
                packet.type = PACKET_SHOT;
                packet.player = *player;
                packet.data1 = current_weapon;
                packet.data2 = 2;  // Direcția - sus
                send_packet(socket, &packet);
            }
            break;
        case '1': case '2': case '3': case '4': case '5':
            {
                int new_weapon = ch - '1';
                if (new_weapon < num_arme_selectate) {
                    current_weapon = new_weapon;
                    player->weapon_type = current_weapon;
                }
            }
            break;
        case '\n':  // Enter
            if (num_arme_selectate > 0) {
                current_weapon = (current_weapon + 1) % num_arme_selectate;
                player->weapon_type = current_weapon;
            }
            break;
        case 'q':
            // Deconectare de la server
            packet.type = PACKET_DISCONNECT;
            packet.player = *player;
            send_packet(socket, &packet);
            player->isalive = 0;
            return;
    }
    
    // Verifică coliziunile cu extratereștrii
    for (int i = 0; i < MAX_PROJECTILE; i++) {
        if (projectiles[i].is_active) {
            if (check_projectile_collision(projectiles[i].x, projectiles[i].y, aliens, &alien_count)) {
                projectiles[i].is_active = 0;
                
                // Trimite pachet pentru lovirea unui extraterestru
                packet.type = PACKET_ALIEN_HIT;
                packet.player = *player;
                packet.data1 = 10;  // Puncte pentru lovirea unui extraterestru
                send_packet(socket, &packet);
                
                player->score += 10;
            }
        }
    }
    
    // Verifică coliziunea cu boss-ul (dacă există)
    // Această parte depinde de cum gestionați boss-ul în jocul vostru
    // Aici este doar un exemplu
    for (int i = 0; i < MAX_PROJECTILE; i++) {
        if (projectiles[i].is_active) {
            // Verifică coliziunea cu boss-ul
            // Exemplu:
            // if (check_boss_collision(projectiles[i].x, projectiles[i].y, boss)) {
            //     projectiles[i].is_active = 0;
            //     
            //     // Trimite pachet pentru lovirea boss-ului
            //     packet.type = PACKET_BOSS_HIT;
            //     packet.player = *player;
            //     packet.data1 = arme_selectate[current_weapon].damage;
            //     send_packet(socket, &packet);
            //     
            //     player->score += 20;
            // }
        }
    }
    
    // Verifică coliziunea cu extratereștrii
    if (check_alien_collision(aliens, player->x, player->y)) {
        // Jucătorul a fost lovit de un extraterestru
        player->health -= 10;
        
        if (player->health <= 0) {
            player->isalive = 0;
            
            // Trimite pachet pentru game over
            packet.type = PACKET_GAME_OVER;
            packet.player = *player;
            send_packet(socket, &packet);
            
            // Afișează mesaj de game over
            clear();
            attron(COLOR_PAIR(1) | A_BOLD);
            mvprintw(LINES/2, COLS/2 - 20, "GAME OVER! Your ship was destroyed!");
            attroff(COLOR_PAIR(1) | A_BOLD);
            refresh();
            napms(3000);
            
            return;
        }
    }
    
    // Trimite poziția actualizată a jucătorului la server
    packet.type = PACKET_PLAYER_POS;
    packet.player = *player;
    send_packet(socket, &packet);
    
    // Afișează informații despre arma curentă
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(LINES - 2, 2, "Weapon: %s (Damage: %d, Ammo: %d)",
             arme_selectate[current_weapon].nume,
             arme_selectate[current_weapon].damage,
             arme_selectate[current_weapon].gloanțe_disp);
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    refresh();
    
    // Actualizează starea jucătorului
    player->ammo = arme_selectate[current_weapon].gloanțe_disp;
    
    // Pauză scurtă pentru a limita rata de cadre
    usleep(16666);  // ~60 FPS
}

void pvp_phase(int socket, player_t *player, arma_t *arme_selectate, int num_arme_selectate) {
    int current_weapon = 0;
    int projectile_count = 0;
    packet_t packet;
    
    clear();
    
    // Desenăm un fundal spațial (stele)
    for (int i = 0; i < 100; i++) {
        int x = rand() % COLS;
        int y = rand() % (LINES - 5);
        attron(COLOR_PAIR(7));
        mvaddch(y, x, '.');
        attroff(COLOR_PAIR(7));
    }
    
    // Desenează nava jucătorului
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(player->y, player->x,    "  /\\  ");
    mvprintw(player->y+1, player->x,  " /  \\ ");
    mvprintw(player->y+2, player->x,  "|----|");
    mvprintw(player->y+3, player->x,  "| () |");
    mvprintw(player->y+4, player->x,  "| () |");
    mvprintw(player->y+5, player->x,  "| __ |");
    mvprintw(player->y+6, player->x,  "/_\\ /_\\");
    attroff(COLOR_PAIR(5) | A_BOLD);
    
    // Primește actualizări de la server
    if (receive_packet(socket, &packet) > 0) {
        // Actualizează starea jocului în funcție de tipul pachetului
        
        // Afișează celălalt jucător dacă este în viață
        if (packet.player.isalive) {
            draw_other_player(&packet.player);
        }
        
        // Afișează HUD pentru PvP
        player_t players[MAX_PLAYERS];
        players[player->id] = *player;
        players[1 - player->id] = packet.player;
        
        draw_multiplayer_hud(players, MAX_PLAYERS, 0, 1);
    }
    
    // Procesează input de la tastatură
    int ch = getch();
    switch (ch) {
        case KEY_LEFT:
            if (player->x > 0) player->x--;
            break;
        case KEY_RIGHT:
            if (player->x < COLS - 7) player->x++;
            break;
        case KEY_UP:
            if (player->y > 0) player->y--;
            break;
        case KEY_DOWN:
            if (player->y < LINES - 10) player->y++;
            break;
        case ' ':
            if (current_weapon < num_arme_selectate && arme_selectate[current_weapon].gloanțe_disp > 0) {
                arme_selectate[current_weapon].gloanțe_disp--;
                
                // În PvP, jucătorii pot trage în orice direcție
                // Sus, jos, stânga, dreapta
                for (int dir = 0; dir < 4; dir++) {
                    shoot_projectile(
                        player->x + 3,
                        player->y + 3,
                        arme_selectate[current_weapon].damage,
                        dir,
                        arme_selectate[current_weapon].damage % 4 + 1,
                        &projectile_count,
                        &arme_selectate[current_weapon]
                    );
                }
                
                // Trimite pachet pentru tragere la server
                packet.type = PACKET_SHOT;
                packet.player = *player;
                packet.data1 = current_weapon;
                packet.data2 = 4;  // Direcție specială pentru PvP - toate direcțiile
                send_packet(socket, &packet);
            }
            break;
        case '1': case '2': case '3': case '4': case '5':
            {
                int new_weapon = ch - '1';
                if (new_weapon < num_arme_selectate) {
                    current_weapon = new_weapon;
                    player->weapon_type = current_weapon;
                }
            }
            break;
        case '\n':  // Enter
            if (num_arme_selectate > 0) {
                current_weapon = (current_weapon + 1) % num_arme_selectate;
                player->weapon_type = current_weapon;
            }
            break;
        case 'q':
            // Deconectare de la server
            packet.type = PACKET_DISCONNECT;
            packet.player = *player;
            send_packet(socket, &packet);
            player->isalive = 0;
            return;
    }
    
    move_projectiles();
    draw_projectiles();
    
    // Verifică coliziunea cu celălalt jucător
    // Această parte este simplificată - în practica reală ar trebui
    // să implementați o verificare mai complexă folosind pachetele primite
    for (int i = 0; i < MAX_PROJECTILE; i++) {
        if (projectiles[i].is_active) {
            // Verificăm coliziunea cu celălalt jucător
            if (packet.player.isalive &&
                projectiles[i].x >= packet.player.x && 
                projectiles[i].x < packet.player.x + 6 &&
                projectiles[i].y >= packet.player.y && 
                projectiles[i].y < packet.player.y + 7) {
                
                projectiles[i].is_active = 0;
                
                // Trimite pachet pentru lovirea celuilalt jucător
                packet.type = PACKET_PVP_HIT;
                packet.player = *player;
                packet.data1 = arme_selectate[current_weapon].damage;
                send_packet(socket, &packet);
                
                player->score += 5;
            }
        }
    }
    
    // Verificăm dacă am fost lovit (trebuie implementat în funcție de pachetele primite)
    // Exemplu:
    if (packet.type == PACKET_PVP_HIT && packet.data1 > 0) {
        player->health -= packet.data1;
        
        // Efect vizual pentru lovitură
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(player->y - 1, player->x, "HIT!");
        attroff(COLOR_PAIR(1) | A_BOLD);
        
        if (player->health <= 0) {
            player->isalive = 0;
            
            // Trimite pachet pentru game over
            packet.type = PACKET_GAME_OVER;
            packet.player = *player;
            send_packet(socket, &packet);
            
            // Afișează mesaj de game over
            clear();
            attron(COLOR_PAIR(1) | A_BOLD);
            mvprintw(LINES/2, COLS/2 - 15, "DEFEAT! Your ship was destroyed!");
            attroff(COLOR_PAIR(1) | A_BOLD);
            refresh();
            napms(3000);
            
            return;
        }
    }
    
    // Trimite poziția actualizată a jucătorului la server
    packet.type = PACKET_PLAYER_POS;
    packet.player = *player;
    send_packet(socket, &packet);
    
    // Afișează informații despre arma curentă și sănătatea jucătorului
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(LINES - 2, 2, "Weapon: %s (Damage: %d, Ammo: %d) | Health: %d",
             arme_selectate[current_weapon].nume,
             arme_selectate[current_weapon].damage,
             arme_selectate[current_weapon].gloanțe_disp,
             player->health);
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    refresh();
    
    // Actualizează starea jucătorului
    player->ammo = arme_selectate[current_weapon].gloanțe_disp;
    
    // Pauză scurtă pentru a limita rata de cadre
    usleep(16666);  // ~60 FPS
}