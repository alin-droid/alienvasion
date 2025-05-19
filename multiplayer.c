#include "multiplayer.h"

// Variabile globale pentru multiplayer
static int socket_fd = -1;
static int is_host = 0;
static int is_connected = 0;
static int game_phase = 0;
static pthread_t network_thread;
static int remote_player_x = 0;
static int remote_player_y = 0;
static int remote_player_score = 0;
static int remote_player_alive = 1;

// Structură pentru pachetul de rețea
typedef struct {
    int type;  // 1=poziție, 2=fază, 3=lovitură
    int x, y;
    int score;
    int health;
    int is_alive;
    int phase;
    int alien_count;
    int data;
} network_packet_t;

// Declarații forward
void multiplayer_host();
void multiplayer_join();
void start_multiplayer_game();
void *network_thread_function(void *arg);
void draw_warning_pillar(int x, int y, int color);

// Funcția pentru meniul multiplayer
void multiplayer_mode() {
    int choice = 0;
    int key;
    
    while (1) {
        clear();
        mvprintw(10, 10, "Multiplayer Mode:");
        
        if (choice == 0) attron(A_REVERSE);
        mvprintw(12, 15, "Host Game");
        if (choice == 0) attroff(A_REVERSE);
        
        if (choice == 1) attron(A_REVERSE);
        mvprintw(14, 15, "Join Game");
        if (choice == 1) attroff(A_REVERSE);
        
        if (choice == 2) attron(A_REVERSE);
        mvprintw(16, 15, "Back");
        if (choice == 2) attroff(A_REVERSE);
        
        refresh();
        
        key = getch();
        if (key == KEY_UP) choice = (choice - 1 + 3) % 3;
        if (key == KEY_DOWN) choice = (choice + 1) % 3;
        if (key == 10) {  // Enter key
            if (choice == 0) {
                multiplayer_host();
            }
            else if (choice == 1) {
                multiplayer_join();
            }
            else if (choice == 2) break;
        }
    }
}

// Funcția pentru a găzdui un joc (host)
void multiplayer_host() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Crearea socket-ului
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        mvprintw(LINES / 2, COLS / 2 - 15, "Eroare la crearea socket-ului");
        refresh();
        getch();
        return;
    }
    
    // Setarea socket-ului pentru reutilizare
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        mvprintw(LINES / 2, COLS / 2 - 15, "Eroare la setarea socket-ului");
        refresh();
        getch();
        close(server_fd);
        return;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Binding
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        mvprintw(LINES / 2, COLS / 2 - 15, "Eroare la binding");
        refresh();
        getch();
        close(server_fd);
        return;
    }
    
    // Ascultare pentru conexiuni
    if (listen(server_fd, 1) < 0) {
        mvprintw(LINES / 2, COLS / 2 - 15, "Eroare la listen");
        refresh();
        getch();
        close(server_fd);
        return;
    }
    
    clear();
    mvprintw(LINES / 2, COLS / 2 - 15, "Așteptare pentru conexiune pe portul %d...", PORT);
    mvprintw(LINES / 2 + 1, COLS / 2 - 15, "Apasă orice tastă pentru a anula");
    refresh();
    
    // Non-blocking 
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
    
    fd_set read_fds;
    struct timeval tv;
    int activity;
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        
        activity = select(server_fd + 1, &read_fds, NULL, NULL, &tv);
        
        if (activity < 0) {
            mvprintw(LINES / 2 + 2, COLS / 2 - 15, "Eroare la select");
            refresh();
            getch();
            close(server_fd);
            return;
        }
        
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            getch();
            mvprintw(LINES / 2 + 2, COLS / 2 - 15, "Anulat de utilizator");
            refresh();
            getch();
            close(server_fd);
            return;
        }
        
        if (FD_ISSET(server_fd, &read_fds)) {
            socket_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (socket_fd < 0) {
                mvprintw(LINES / 2 + 2, COLS / 2 - 15, "Eroare la accept");
                refresh();
                getch();
                close(server_fd);
                return;
            }
            break;
        }
    }
    
    // Înapoi la blocking
    flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags & ~O_NONBLOCK);
    
    close(server_fd);
    
    // Pregătire joc
    is_host = 1;
    is_connected = 1;
    pthread_create(&network_thread, NULL, network_thread_function, NULL);
    
    clear();
    mvprintw(LINES / 2, COLS / 2 - 15, "Jucător conectat! Începe jocul...");
    refresh();
    napms(2000);
    
    start_multiplayer_game();
    
    is_connected = 0;
    pthread_join(network_thread, NULL);
    close(socket_fd);
}

// Funcția pentru a se alătura unui joc
void multiplayer_join() {
    struct sockaddr_in serv_addr;
    char ip_address[16] = {0};
    
    clear();
    echo();
    mvprintw(LINES / 2, COLS / 2 - 15, "Introdu adresa IP a host-ului: ");
    getstr(ip_address);
    noecho();
    
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        mvprintw(LINES / 2 + 2, COLS / 2 - 15, "Eroare la crearea socket-ului");
        refresh();
        getch();
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, ip_address, &serv_addr.sin_addr) <= 0) {
        mvprintw(LINES / 2 + 2, COLS / 2 - 15, "Adresă IP invalidă");
        refresh();
        getch();
        close(socket_fd);
        return;
    }
    
    clear();
    mvprintw(LINES / 2, COLS / 2 - 15, "Se conectează la %s...", ip_address);
    refresh();
    
    if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        mvprintw(LINES / 2 + 2, COLS / 2 - 15, "Conexiune eșuată");
        refresh();
        getch();
        close(socket_fd);
        return;
    }
    
    is_host = 0;
    is_connected = 1;
    pthread_create(&network_thread, NULL, network_thread_function, NULL);
    
    clear();
    mvprintw(LINES / 2, COLS / 2 - 15, "Conectat la server! Începe jocul...");
    refresh();
    napms(2000);
    
    start_multiplayer_game();
    
    is_connected = 0;
    pthread_join(network_thread, NULL);
    close(socket_fd);
}

// Funcția pentru thread-ul de rețea
void *network_thread_function(void *arg) {
    network_packet_t packet;
    
    while (is_connected) {
        int bytes_received = recv(socket_fd, &packet, sizeof(network_packet_t), 0);
        
        if (bytes_received <= 0) {
            is_connected = 0;
            break;
        }
        
        switch (packet.type) {
            case 1:  // Actualizare poziție
                remote_player_x = packet.x;
                remote_player_y = packet.y;
                remote_player_score = packet.score;
                remote_player_alive = packet.is_alive;
                
                if (!is_host) {
                    game_phase = packet.phase;
                }
                break;
                
            case 2:  // Actualizare fază
                game_phase = packet.phase;
                break;
                
            case 3:  // Lovitură jucător
                if (game_phase == 2) {
                    remote_player_alive = 0;
                }
                break;
        }
        
        napms(1);
    }
    
    return NULL;
}

// Funcția pentru desenarea stâlpului de avertizare
void draw_warning_pillar(int x, int y, int color) {
    attron(COLOR_PAIR(color));
    mvprintw(y, x,     "|===|");
    mvprintw(y + 1, x, "| ! |");
    mvprintw(y + 2, x, "| ! |");
    mvprintw(y + 3, x, "|===|");
    mvprintw(y + 4, x, "| | |");
    mvprintw(y + 5, x, "| | |");
    mvprintw(y + 6, x, "|___|");
    attroff(COLOR_PAIR(color));
}

// Funcția pentru jocul multiplayer propriu-zis
void start_multiplayer_game() {
    alien_t *aliens = malloc(NUMBER_OF_ALIENS * sizeof(alien_t));
    if (!aliens) {
        endwin();
        printf("Eroare alocare memorie!\n");
        exit(1);
    }
    
    initialize_random();
    createAliens(aliens);
    init_projectiles();
    
    arma_t arme[NUM_ARME_DISPONIBILE];
    arma_t arme_selectate[MAX_TOTAL_ARME_SELECTATE];
    int num_arme_selectate;
    int projectile_count = 0;
    
    init_arme(arme);
    
    // Selectarea armelor
    int highlight = 0;
    num_arme_selectate = 0;
    int counts[NUM_ARME_DISPONIBILE] = {0};
    
    while (num_arme_selectate < MAX_TOTAL_ARME_SELECTATE) {
        clear();
        mvprintw(10, 10, "Selectează armele (SUS/JOS navighează, ENTER adaugă %d, 'q' termină)", ARME_PER_SELECTIE);
        for (int i = 0; i < NUM_ARME_DISPONIBILE; i++) {
            if (i == highlight) attron(A_REVERSE);
            mvprintw(13 + i * 2, 15, "%s (%d/%d)", arme[i].nume, counts[i], MAX_ARME_PER_SELECTIE);
            if (i == highlight) attroff(A_REVERSE);
        }
        refresh();

        int key = getch();
        if (key == 'q') break;
        if (key == KEY_UP) highlight = (highlight - 1 + NUM_ARME_DISPONIBILE) % NUM_ARME_DISPONIBILE;
        if (key == KEY_DOWN) highlight = (highlight + 1) % NUM_ARME_DISPONIBILE;
        if (key == 10) {  // Enter
            int to_add = ARME_PER_SELECTIE;
            if (counts[highlight] + to_add > MAX_ARME_PER_SELECTIE) {
                to_add = MAX_ARME_PER_SELECTIE - counts[highlight];
            }
            if (num_arme_selectate + to_add > MAX_TOTAL_ARME_SELECTATE) {
                to_add = MAX_TOTAL_ARME_SELECTATE - num_arme_selectate;
            }
            for (int i = 0; i < to_add; i++) {
                arme_selectate[num_arme_selectate] = arme[highlight];
                num_arme_selectate++;
            }
            counts[highlight] += to_add;
        }
    }
    
    int local_x = is_host ? COLS / 3 : 2 * COLS / 3;
    int local_y = LINES - 8;
    int local_score = 0;
    int local_current_weapon = 0;
    int local_is_alive = 1;
    
    int alien_count = NUMBER_OF_ALIENS;
    boss_t boss;
    
    time_t last_alien_move = time(NULL);
    time_t last_alien_spawn = time(NULL);
    time_t last_network_update = time(NULL);
    
    int game_over = 0;
    
    while (!game_over && is_connected) {
        clear();
        
        time_t current_time = time(NULL);
        
        if (is_host && current_time - last_alien_move >= ((double)ALIEN_MOVE_DELAY / 1000.0)) {
            if (game_phase == 0) {
                moveAliens(aliens);
            } else if (game_phase == 1) {
                moveBoss(&boss);
            }
            last_alien_move = current_time;
        }
        
        if (is_host && current_time - last_alien_spawn >= ((double)ALIEN_SPAWN_INTERVAL / 1000.0) && game_phase == 0) {
            regenerate_aliens(aliens, &alien_count);
            last_alien_spawn = current_time;
        }
        
        move_projectiles();
        
        for (int i = 0; i < MAX_PROJECTILE; i++) {
            if (projectiles[i].is_active) {
                if (game_phase == 0) {
                    if (check_projectile_collision(projectiles[i].x, projectiles[i].y, aliens, &alien_count)) {
                        projectiles[i].is_active = 0;
                        local_score += 10;
                    }
                } else if (game_phase == 1) {
                    int result = check_projectile_boss_collision(projectiles[i].x, projectiles[i].y, &boss);
                    if (result > 0) {
                        projectiles[i].is_active = 0;
                        local_score += 20;
                        
                        if (result == 2 && is_host) {
                            game_phase = 2;
                            
                            network_packet_t packet;
                            memset(&packet, 0, sizeof(network_packet_t));
                            packet.type = 2;
                            packet.phase = 2;
                            send(socket_fd, &packet, sizeof(network_packet_t), 0);
                        }
                    }
                } else if (game_phase == 2) {
                    if (projectiles[i].x >= remote_player_x && 
                        projectiles[i].x < remote_player_x + 6 &&
                        projectiles[i].y >= remote_player_y && 
                        projectiles[i].y < remote_player_y + 7) {
                        
                        projectiles[i].is_active = 0;
                        
                        network_packet_t packet;
                        memset(&packet, 0, sizeof(network_packet_t));
                        packet.type = 3;
                        packet.data = arme_selectate[local_current_weapon].damage;
                        send(socket_fd, &packet, sizeof(network_packet_t), 0);
                    }
                }
            }
        }
        
        if (game_phase <= 1) {
            draw_warning_pillar(COLS - 10, LINES - 8, game_phase == 0 ? 1 : 2);
            
            if (game_phase == 0) {
                printAliens(aliens);
            } else {
                printBoss(&boss);
            }
        } else {
            for (int i = 0; i < 50; i++) {
                mvaddch(rand() % LINES, rand() % COLS, '*');
            }
        }
        
        draw_spaceship(local_x, local_y);
        draw_spaceship(remote_player_x, remote_player_y);
        
        draw_projectiles();
        
        mvprintw(LINES - 2, 0, "Scor: %d | Arma: %s (Gloanțe: %d)",
                 local_score,
                 arme_selectate[local_current_weapon].nume,
                 arme_selectate[local_current_weapon].gloanțe_disp);
        
        if (game_phase == 0) {
            mvprintw(LINES - 1, 0, "Extratereștri rămași: %d | Faza: Cooperativă", alien_count);
        } else if (game_phase == 1) {
            mvprintw(LINES - 1, 0, "Viață Boss: %d | Faza: Boss", boss.health);
        } else {
            mvprintw(LINES - 1, 0, "Faza: PvP | Elimină adversarul!");
        }
        
        refresh();
        
        int ch = getch();
        switch (ch) {
            case KEY_LEFT:
                if (local_x > 0) local_x -= 1;
                break;
                
            case KEY_RIGHT:
                if (local_x < COLS - 7) local_x += 1;
                break;
                
            case KEY_UP:
                if (game_phase == 2 && local_y > 1) local_y -= 1;
                break;
                
            case KEY_DOWN:
                if (game_phase == 2 && local_y < LINES - 8) local_y += 1;
                break;
                
            case ' ':
                if (local_current_weapon < num_arme_selectate && 
                    arme_selectate[local_current_weapon].gloanțe_disp > 0) {
                    
                    arme_selectate[local_current_weapon].gloanțe_disp--;
                    
                    if (game_phase < 2) {
                        shoot_projectile(
                            local_x + 3,
                            local_y - 1,
                            arme_selectate[local_current_weapon].damage,
                            2,
                            arme_selectate[local_current_weapon].damage % 4 + 1,
                            &projectile_count,
                            &arme_selectate[local_current_weapon]
                        );
                    } else {
                        int dx = remote_player_x - local_x;
                        int dy = remote_player_y - local_y;
                        int direction;
                        
                        if (abs(dx) > abs(dy)) {
                            direction = (dx > 0) ? 1 : 0;
                        } else {
                            direction = (dy > 0) ? 3 : 2;
                        }
                        
                        shoot_projectile(
                            local_x + 3,
                            local_y + ((direction == 3) ? 6 : -1),
                            arme_selectate[local_current_weapon].damage,
                            direction,
                            arme_selectate[local_current_weapon].damage % 4 + 1,
                            &projectile_count,
                            &arme_selectate[local_current_weapon]
                        );
                    }
                }
                break;
                
            case 10:  // Enter
                if (num_arme_selectate > 0) {
                    local_current_weapon = (local_current_weapon + 1) % num_arme_selectate;
                }
                break;
                
            case 'q':
            case 'Q':
                game_over = 1;
                break;
        }
        
        if (game_phase == 0) {
            if (check_alien_collision(aliens, local_x, local_y)) {
                local_is_alive = 0;
                game_over = 1;
            }
            
            if (alien_count <= 0 && is_host) {
                game_phase = 1;
                createBoss(&boss);
                
                for (int i = 0; i < num_arme_selectate; i++) {
                    arme_selectate[i].gloanțe_disp = arme[i].gloanțe_disp;
                }
                
                network_packet_t packet;
                memset(&packet, 0, sizeof(network_packet_t));
                packet.type = 2;
                packet.phase = 1;
                send(socket_fd, &packet, sizeof(network_packet_t), 0);
                
                clear();
                mvprintw(LINES / 2, COLS / 2 - 20, "Toți extratereștrii au fost eliminați! Se apropie BOSS-ul!");
                refresh();
                napms(3000);
            }
        } else if (game_phase == 1) {
            if (check_boss_collision(&boss, local_x, local_y)) {
                local_is_alive = 0;
                game_over = 1;
            }
        }
        
        if (current_time - last_network_update >= 0.05) {
            network_packet_t packet;
            memset(&packet, 0, sizeof(network_packet_t));
            packet.type = 1;
            packet.x = local_x;
            packet.y = local_y;
            packet.score = local_score;
            packet.is_alive = local_is_alive;
            packet.phase = game_phase;
            
            if (is_host) {
                packet.alien_count = alien_count;
                if (game_phase == 1) {
                    packet.health = boss.health;
                }
            }
            
            send(socket_fd, &packet, sizeof(network_packet_t), 0);
            last_network_update = current_time;
        }
        
        if (!local_is_alive || !remote_player_alive) {
            clear();
            if (!local_is_alive) {
                mvprintw(LINES / 2, COLS / 2 - 10, "Ai fost învins!");
            } else {
                mvprintw(LINES / 2, COLS / 2 - 10, "Ai câștigat!");
            }
            refresh();
            napms(3000);
            game_over = 1;
        }
        
        if (!is_connected) {
            clear();
            mvprintw(LINES / 2, COLS / 2 - 15, "Conexiunea cu celălalt jucător a fost pierdută.");
            refresh();
            napms(3000);
            game_over = 1;
        }
        
        napms(16);
    }
    
    free(aliens);
}