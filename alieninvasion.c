#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "aliens.h"
#include "arms.h"
#include "nava.h"

#define TOTAL_ALIENS_TO_KILL 50
#define ENTER 10
#define NUM_ARME_DISPONIBILE 5
#define MAX_ARME_PER_SELECTIE 20
#define MAX_TOTAL_ARME_SELECTATE 80
#define MAX_PROJECTILE 100
#define ARME_PER_SELECTIE 4
#define PORT 8080
#define BUFFER_SIZE 1024
typedef struct {
    int x, y;
    int score;
    int current_weapon;
    int is_alive;
} player_t;
// Implementarea funcției multiplayer_host
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
    
    // Setarea socket-ului pentru reutilizarea adresei
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        mvprintw(LINES / 2, COLS / 2 - 15, "Eroare la setarea opțiunilor socket");
        refresh();
        getch();
        close(server_fd);
        return;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Atașarea socket-ului la portul specificat
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        mvprintw(LINES / 2, COLS / 2 - 15, "Eroare la binding");
        refresh();
        getch();
        close(server_fd);
        return;
    }
    
    // Ascultarea pentru conexiuni
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
    
    // Setăm socket-ul ca non-blocking pentru a permite anularea
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Așteptăm conexiunea sau anularea
    fd_set read_fds;
    struct timeval tv;
    int activity;
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        
        // Timeout de 0.1 secunde
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
        
        // Verificăm dacă a fost apăsată o tastă
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            getch(); // Consumăm tasta
            mvprintw(LINES / 2 + 2, COLS / 2 - 15, "Anulat de utilizator");
            refresh();
            getch();
            close(server_fd);
            return;
        }
        
        // Verificăm dacă avem o conexiune nouă
        if (FD_ISSET(server_fd, &read_fds)) {
            socket_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (socket_fd < 0) {
                mvprintw(LINES / 2 + 2, COLS / 2 - 15, "Eroare la accept");
                refresh();
                getch();
                close(server_fd);
                return;
            }
            
            // Am acceptat o conexiune, putem ieși din buclă
            break;
        }
    }
    
    // Revenim la modul blocking pentru socket-ul client
    flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags & ~O_NONBLOCK);
    
    // Am stabilit conexiunea, putem închide socket-ul server
    close(server_fd);
    
    // Inițiem thread-ul pentru comunicarea în rețea
    is_host = 1;
    is_connected = 1;
    pthread_create(&network_thread, NULL, network_thread_function, NULL);
    
    // Afișăm un mesaj de succes
    clear();
    mvprintw(LINES / 2, COLS / 2 - 15, "Jucător conectat! Începe jocul...");
    refresh();
    napms(2000);
    
    // Inițializăm jocul
    start_multiplayer_game();
    
    // Curățare
    is_connected = 0;
    pthread_join(network_thread, NULL);
    close(socket_fd);
}

// Implementarea funcției multiplayer_join
void multiplayer_join() {
    struct sockaddr_in serv_addr;
    char ip_address[16] = {0};
    
    // Solicită adresa IP
    clear();
    echo();
    mvprintw(LINES / 2, COLS / 2 - 15, "Introdu adresa IP a host-ului: ");
    getstr(ip_address);
    noecho();
    
    // Crearea socket-ului
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        mvprintw(LINES / 2 + 2, COLS / 2 - 15, "Eroare la crearea socket-ului");
        refresh();
        getch();
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convertirea adresei IP din text în binar
    if (inet_pton(AF_INET, ip_address, &serv_addr.sin_addr) <= 0) {
        mvprintw(LINES / 2 + 2, COLS / 2 - 15, "Adresă IP invalidă");
        refresh();
        getch();
        close(socket_fd);
        return;
    }
    
    // Conectarea la server
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
    
    // Inițiem thread-ul pentru comunicarea în rețea
    is_host = 0;
    is_connected = 1;
    pthread_create(&network_thread, NULL, network_thread_function, NULL);
    
    // Afișăm un mesaj de succes
    clear();
    mvprintw(LINES / 2, COLS / 2 - 15, "Conectat la server! Începe jocul...");
    refresh();
    napms(2000);
    
    // Inițializăm jocul
    start_multiplayer_game();
    
    // Curățare
    is_connected = 0;
    pthread_join(network_thread, NULL);
    close(socket_fd);
}

// Structura pachetului pentru comunicare în rețea
typedef struct {
    int type;  // Tipul pachetului (1=poziție, 2=fază, 3=lovitură)
    int x, y;  // Poziția jucătorului
    int score; // Scorul jucătorului
    int health; // Viața jucătorului/boss-ului
    int is_alive; // Starea jucătorului (viu/mort)
    int phase;  // Faza jocului (0=aliens, 1=boss, 2=pvp)
    int alien_count; // Numărul de extratereștri rămași
    int data;  // Date adiționale
} network_packet_t;

// Funcția thread-ului de rețea pentru primirea pachetelor
void *network_thread_function(void *arg) {
    network_packet_t packet;
    
    while (is_connected) {
        // Primim pachete de la celălalt jucător
        int bytes_received = recv(socket_fd, &packet, sizeof(network_packet_t), 0);
        
        if (bytes_received <= 0) {
            // Conexiunea a fost închisă sau a apărut o eroare
            is_connected = 0;
            break;
        }
        
        // Procesăm pachetul în funcție de tipul său
        switch (packet.type) {
            case 1:  // Actualizare poziție
                remote_player_x = packet.x;
                remote_player_y = packet.y;
                remote_player_score = packet.score;
                remote_player_alive = packet.is_alive;
                
                if (!is_host) {
                    // Clientul primește informații despre starea jocului de la host
                    game_phase = packet.phase;
                    // Pentru alte informații despre starea jocului (alien_count, boss health, etc.)
                }
                break;
                
            case 2:  // Actualizare fază
                game_phase = packet.phase;
                break;
                
            case 3:  // Lovitură jucător (în faza PvP)
                if (game_phase == 2) {
                    remote_player_alive = 0;  // Am fost lovit, suntem morți
                }
                break;
        }
        
        // Adăugăm o mică pauză pentru a nu consuma prea multe resurse ale CPU-ului
        napms(1);
    }
    
    return NULL;
}

// Funcția pentru jocul multiplayer
void start_multiplayer_game() {
    // Variabile pentru jocul multiplayer
    alien_t *aliens = malloc(NUMBER_OF_ALIENS * sizeof(alien_t));
    if (!aliens) {
        endwin();
        printf("Eroare alocare memorie!\n");
        exit(1);
    }
    
    initialize_random();
    createAliens(aliens);
    init_projectiles();
    
    // Inițializare arme
    arma_t arme[NUM_ARME_DISPONIBILE];
    arma_t arme_selectate[MAX_TOTAL_ARME_SELECTATE];
    int num_arme_selectate;
    int projectile_count = 0;
    
    init_arme(arme);
    select_arme(arme, arme_selectate, &num_arme_selectate);
    
    // Inițializare jucător și variabile de joc
    int local_x = is_host ? COLS / 3 : 2 * COLS / 3;
    int local_y = LINES - 8;
    int local_score = 0;
    int local_current_weapon = 0;
    int local_is_alive = 1;
    
    int alien_count = NUMBER_OF_ALIENS;
    int boss_phase = 0;  // 0 = nu a început, 1 = activ
    boss_t boss;
    
    // Variabilele pentru sincronizare
    time_t last_alien_move = time(NULL);
    time_t last_alien_spawn = time(NULL);
    time_t last_network_update = time(NULL);
    
    // Intrăm în bucla principală a jocului
    int game_over = 0;
    
    while (!game_over && is_connected) {
        clear();
        
        // Actualizare stare joc
        time_t current_time = time(NULL);
        
        // Mișcarea aliens și regenerare
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
        
        // Mișcarea proiectilelor
        move_projectiles();
        
        // Verificarea coliziunilor cu proiectilele
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
                        
                        // Verificăm dacă boss-ul a fost învins
                        if (result == 2 && is_host) {
                            game_phase = 2;  // Trecem la faza PvP
                            
                            // Trimite actualizare de fază
                            network_packet_t packet;
                            memset(&packet, 0, sizeof(network_packet_t));
                            packet.type = 2;  // Actualizare fază
                            packet.phase = 2;  // Faza PvP
                            send(socket_fd, &packet, sizeof(network_packet_t), 0);
                        }
                    }
                } else if (game_phase == 2) {
                    // Verificăm coliziunile între jucători în faza PvP
                    if (projectiles[i].x >= remote_player_x && 
                        projectiles[i].x < remote_player_x + 6 &&
                        projectiles[i].y >= remote_player_y && 
                        projectiles[i].y < remote_player_y + 7) {
                        
                        projectiles[i].is_active = 0;
                        
                        // Trimitem informații despre lovitură
                        network_packet_t packet;
                        memset(&packet, 0, sizeof(network_packet_t));
                        packet.type = 3;  // Lovitură jucător
                        packet.data = arme_selectate[local_current_weapon].damage;
                        send(socket_fd, &packet, sizeof(network_packet_t), 0);
                    }
                }
            }
        }
        
        // Desenăm elementele de joc
        if (game_phase <= 1) {
            // Desenăm stâlpul de avertizare
            draw_warning_pillar(COLS - 10, LINES - 8, game_phase == 0 ? 1 : 2);
            
            // Desenăm aliens sau boss-ul
            if (game_phase == 0) {
                printAliens(aliens);
            } else {
                printBoss(&boss);
            }
        } else {
            // În faza PvP desenăm doar un fundal stelar
            for (int i = 0; i < 50; i++) {
                mvaddch(rand() % LINES, rand() % COLS, '*');
            }
        }
        
        // Desenăm jucătorii
        draw_spaceship(local_x, local_y);
        draw_spaceship(remote_player_x, remote_player_y);
        
        // Desenăm proiectilele
        draw_projectiles();
        
        // Afișăm informații despre joc
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
        
        // Procesăm input-ul utilizatorului
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
                
            case ' ':  // Spațiu pentru tragere
                if (local_current_weapon < num_arme_selectate && 
                    arme_selectate[local_current_weapon].gloanțe_disp > 0) {
                    
                    arme_selectate[local_current_weapon].gloanțe_disp--;
                    
                    if (game_phase < 2) {
                        // Faza cooperativă și boss - tragem în sus
                        shoot_projectile(
                            local_x + 3,
                            local_y - 1,
                            arme_selectate[local_current_weapon].damage,
                            2,  // Direcția - sus
                            arme_selectate[local_current_weapon].damage % 4 + 1,
                            &projectile_count,
                            &arme_selectate[local_current_weapon]
                        );
                    } else {
                        // Faza PvP - direcția către celălalt jucător
                        int dx = remote_player_x - local_x;
                        int dy = remote_player_y - local_y;
                        int direction;
                        
                        if (abs(dx) > abs(dy)) {
                            direction = (dx > 0) ? 1 : 0;  // Dreapta sau stânga
                        } else {
                            direction = (dy > 0) ? 3 : 2;  // Jos sau sus
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
                
            case '\n':  // Enter pentru schimbarea armei
                if (num_arme_selectate > 0) {
                    local_current_weapon = (local_current_weapon + 1) % num_arme_selectate;
                }
                break;
                
            case 'q':
            case 'Q':
                game_over = 1;
                break;
        }
        
        // Verificăm coliziunile cu extratereștrii sau boss-ul
        if (game_phase == 0) {
            if (check_alien_collision(aliens, local_x, local_y)) {
                local_is_alive = 0;
                game_over = 1;
            }
            
            // Verificăm dacă am eliminat toți extratereștrii
            if (alien_count <= 0 && is_host) {
                game_phase = 1;
                createBoss(&boss);
                
                // Reîncărcăm muniția pentru lupta cu boss-ul
                for (int i = 0; i < num_arme_selectate; i++) {
                    arme_selectate[i].gloanțe_disp = arme[i].gloanțe_disp;
                }
                
                // Trimite actualizare de fază
                network_packet_t packet;
                memset(&packet, 0, sizeof(network_packet_t));
                packet.type = 2;  // Actualizare fază
                packet.phase = 1;  // Faza boss
                send(socket_fd, &packet, sizeof(network_packet_t), 0);
                
                // Afișăm mesaj de tranziție
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
        
        // Trimitem actualizările poziției și stării locale
        if (current_time - last_network_update >= 0.05) {  // 50ms între actualizări
            network_packet_t packet;
            memset(&packet, 0, sizeof(network_packet_t));
            packet.type = 1;  // Actualizare poziție
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
        
        // Verificăm dacă jocul s-a terminat
        if (!local_is_alive || !remote_player_alive) {
            // Afișăm mesajul de final
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
        
        // Verificăm dacă conexiunea s-a închis
        if (!is_connected) {
            clear();
            mvprintw(LINES / 2, COLS / 2 - 15, "Conexiunea cu celălalt jucător a fost pierdută.");
            refresh();
            napms(3000);
            game_over = 1;
        }
        
        // Adăugăm o mică pauză pentru a limita rata de cadre
        napms(16);  // ~60 FPS
    }
    
    // Eliberăm memoria alocată
    free(aliens);
}
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
void draw_table(int y, int x, int height, int width) {
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + width - 1, ACS_URCORNER);
    mvaddch(y + height - 1, x, ACS_LLCORNER);
    mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
    
    for (int i = 1; i < width - 1; i++) {
        mvaddch(y, x + i, ACS_HLINE);
        mvaddch(y + height - 1, x + i, ACS_HLINE);
    }
    
    for (int i = 1; i < height - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + width - 1, ACS_VLINE);
    }
}

void draw_table_divider(int y, int x, int width) {
    mvaddch(y, x, ACS_LTEE);
    for (int i = 1; i < width - 1; i++) {
        mvaddch(y, x + i, ACS_HLINE);
    }
    mvaddch(y, x + width - 1, ACS_RTEE);
}

void get_weapon_description(arma_t weapon, char* desc_buffer, int buffer_size) {
    if (strstr(weapon.nume, "Laser") != NULL) {
        strncpy(desc_buffer, "High precision weapon with moderate damage", buffer_size - 1);
    } else if (strstr(weapon.nume, "Plasma") != NULL) {
        strncpy(desc_buffer, "Powerful energy weapon with high damage", buffer_size - 1);
    } else if (strstr(weapon.nume, "Rocket") != NULL) {
        strncpy(desc_buffer, "Explosive projectile with area damage", buffer_size - 1);
    } else if (strstr(weapon.nume, "Machine") != NULL) {
        strncpy(desc_buffer, "Rapid fire weapon with low cooldown", buffer_size - 1);
    } else if (strstr(weapon.nume, "Crossbow") != NULL) {
        strncpy(desc_buffer, "Silent weapon with good accuracy", buffer_size - 1);
    } else {
        strncpy(desc_buffer, "Standard alien hunting weapon", buffer_size - 1);
    }
    desc_buffer[buffer_size - 1] = '\0';
}

void init_ncurses() {
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    timeout(-1);
}

void close_ncurses() {
    endwin();
}

void print_title() {
    int title_y = 2;
    int title_x = 10;
    int title_height = 6;
    int title_width = 50;
    
    attron(COLOR_PAIR(4));
    draw_table(title_y-1, title_x-2, title_height+2, title_width);
    attroff(COLOR_PAIR(4));
    
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(2, 10, "  /\\     | |     | |   | _____|  | |\\   | |");
    mvprintw(3, 10, " /  \\    | |     | |   | |____   | | \\  | |");
    mvprintw(4, 10, "/ /\\ \\   | |     | |   | |____|  | |  \\ | |");
    mvprintw(5, 10, "/ ____ \\  | |___  | |   | |____   | |   \\| |");
    mvprintw(6, 10,"/_/    \\_\\ |_____| |_|   |_______|  |_|    \\| |");
    attroff(A_BOLD | COLOR_PAIR(4));
}

void print_menu(int highlight) {
    char *choices[] = {"Single Player", "Multiplayer", "Exit"};
    int n_choices = sizeof(choices) / sizeof(choices[0]);
    
    int menu_y = 10;
    int menu_x = 10;
    int menu_width = 40;
    int menu_height = n_choices * 2 + 4;
    
    attron(COLOR_PAIR(2));
    draw_table(menu_y-1, menu_x-1, menu_height, menu_width);
    attroff(COLOR_PAIR(2));
    
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(menu_y, menu_x, "Welcome to Alien Invasion!");
    mvprintw(menu_y+1, menu_x, "Please select a game mode:");
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    attron(COLOR_PAIR(2));
    draw_table_divider(menu_y+2, menu_x-1, menu_width);
    attroff(COLOR_PAIR(2));

    for (int i = 0; i < n_choices; i++) {
        if (i == highlight) attron(A_REVERSE);
        mvprintw(menu_y + 3 + i * 2, menu_x + 10, "%s", choices[i]);
        attroff(A_REVERSE);
    }
}

void select_arme(arma_t *arme, arma_t *arme_selectate, int *num_arme_selectate) {
    int highlight = 0;
    *num_arme_selectate = 0;
    int total_weapons_to_select = 5;
    clear();

    int menu_height = NUM_ARME_DISPONIBILE + 8;
    int menu_width = 70;
    int start_y = (LINES - menu_height) / 2;
    int start_x = (COLS - menu_width) / 2;

    while (*num_arme_selectate < total_weapons_to_select) {
        clear();
        
        attron(COLOR_PAIR(5));
        draw_table(start_y, start_x, menu_height, menu_width);
        attroff(COLOR_PAIR(5));
        
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(start_y + 1, start_x + (menu_width - 14) / 2, "WEAPON SELECTION");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        mvprintw(start_y + 3, start_x + 2, "Use UP/DOWN to navigate, ENTER to select");
        mvprintw(start_y + 4, start_x + 2, "Weapons selected: %d/%d", *num_arme_selectate, total_weapons_to_select);
        
        attron(COLOR_PAIR(5));
        draw_table_divider(start_y + 5, start_x, menu_width);
        attroff(COLOR_PAIR(5));
        
        attron(A_BOLD);
        mvprintw(start_y + 6, start_x + 2, "%-15s | %-10s | %s", "WEAPON", "DAMAGE", "DESCRIPTION");
        attroff(A_BOLD);
        
        attron(COLOR_PAIR(5));
        draw_table_divider(start_y + 7, start_x, menu_width);
        attroff(COLOR_PAIR(5));
        
        for (int i = 0; i < NUM_ARME_DISPONIBILE; i++) {
            char desc[40];
            get_weapon_description(arme[i], desc, sizeof(desc));
            
            if (i == highlight) 
                attron(A_REVERSE);
            
            mvprintw(start_y + 8 + i, start_x + 2, "%-15s | %-10d | %s", 
                     arme[i].nume, 
                     arme[i].damage,
                     desc);
                     
            if (i == highlight)
                attroff(A_REVERSE);
        }
        
        refresh();

        int key = getch();
        if (key == KEY_UP) highlight = (highlight - 1 + NUM_ARME_DISPONIBILE) % NUM_ARME_DISPONIBILE;
        if (key == KEY_DOWN) highlight = (highlight + 1) % NUM_ARME_DISPONIBILE;
        if (key == ENTER) {
            if (*num_arme_selectate < total_weapons_to_select) {
                arme_selectate[*num_arme_selectate] = arme[highlight];
                (*num_arme_selectate)++;
                
                attron(COLOR_PAIR(5) | A_BOLD);
                mvprintw(start_y + menu_height - 2, start_x + 2, "Added %s! (%d/%d)", 
                        arme[highlight].nume, *num_arme_selectate, total_weapons_to_select);
                attroff(COLOR_PAIR(5) | A_BOLD);
                refresh();
                napms(500);
            }
        }
        
        if (*num_arme_selectate >= total_weapons_to_select) {
            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(start_y + menu_height - 2, start_x + 2, "All weapons selected! Press any key to start.");
            attroff(COLOR_PAIR(3) | A_BOLD);
            refresh();
            getch();
        }
    }

    clear();
}

void draw_weapon_panel(arma_t *arme_selectate, int num_arme_selectate, int current_weapon) {

    int panel_width = 25;
    int panel_height = 6;
    int start_x = 2;
    int start_y = 1;
    
    attron(COLOR_PAIR(6));
    draw_table(start_y, start_x, panel_height, panel_width);
    attroff(COLOR_PAIR(6));
    
    attron(COLOR_PAIR(6) | A_BOLD);
    mvprintw(start_y + 1, start_x + 2, "CURRENT WEAPON");
    
    draw_table_divider(start_y + 2, start_x, panel_width);
    attroff(COLOR_PAIR(6) | A_BOLD);
    
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(start_y + 3, start_x + 2, "%-15s", arme_selectate[current_weapon].nume);
    attroff(COLOR_PAIR(3) | A_BOLD);
    
    mvprintw(start_y + 4, start_x + 2, "Damage: %d", arme_selectate[current_weapon].damage);
    mvprintw(start_y + 5, start_x + 2, "Ammo: %d", arme_selectate[current_weapon].gloanțe_disp);
}

void draw_game_status(int aliens_killed, int alien_count) {
    int status_width = 50;
    int status_height = 5;
    int start_x = COLS - status_width - 2;
    int start_y = 1;
    
    attron(COLOR_PAIR(5));
    draw_table(start_y, start_x, status_height, status_width);
    attroff(COLOR_PAIR(5));
    
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(start_y + 1, start_x + (status_width - 11)/2, "GAME STATUS");
    
    draw_table_divider(start_y + 2, start_x, status_width);
    attroff(COLOR_PAIR(5) | A_BOLD);
    
    attron(COLOR_PAIR(5));
    mvprintw(start_y + 3, start_x + 5, "ALIENS KILLED: %d/%d    ALIENS ACTIVE: %d", 
             aliens_killed, TOTAL_ALIENS_TO_KILL, alien_count);
    attroff(COLOR_PAIR(5));
    
    int progress_width = status_width - 10;
    int filled = (aliens_killed * progress_width) / TOTAL_ALIENS_TO_KILL;
    
    mvprintw(start_y + 4, start_x + 5, "[");
    for (int i = 0; i < progress_width; i++) {
        if (i < filled)
            addch('|');
        else
            addch(' ');
    }
    printw("]");
}

void draw_controls_panel() {
    int controls_width = 70;
    int start_x = (COLS - controls_width) / 2;
    int start_y = LINES - 2;
    
    attron(A_DIM);
    mvprintw(start_y, start_x, "ENTER: change weapon | SPACE: fire | ←→: move ship | Q: quit");
    attroff(A_DIM);
}
void single_player_mode() {
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);      // Roșu pentru stâlp de avertizare și unele extratereștre
    init_pair(2, COLOR_BLUE, COLOR_BLACK);     // Albastru pentru stâlp după eliminarea extratereștrilor
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);   // Galben pentru unele extratereștre
    init_pair(4, COLOR_CYAN, COLOR_BLACK);     // Cyan pentru unele extratereștre
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);  // Magenta pentru boss
    timeout(20);

    clear();
    refresh();

    arma_t arme[NUM_ARME_DISPONIBILE];
    arma_t arme_selectate[MAX_TOTAL_ARME_SELECTATE];
    int num_arme_selectate;
    int projectile_count = 0;

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

    int ship_x = COLS / 2 - 3;
    int ship_y = LINES - 8;
    int current_weapon = 0;
    int alien_count = NUMBER_OF_ALIENS;
    int game_over = 0;
    int boss_phase = 0;  // 0 = faza cu aliens, 1 = faza cu boss
    boss_t boss;

    time_t last_alien_move = time(NULL);
    time_t last_alien_spawn = time(NULL);

    while (!game_over) {
        clear();

        time_t current_time = time(NULL);
        if (current_time - last_alien_move >= ((double)ALIEN_MOVE_DELAY / 1000.0)) {
            if (boss_phase == 0) {
                moveAliens(aliens);
            } else if (boss_phase == 1) {
                moveBoss(&boss);
            }
            last_alien_move = current_time;
        }

        if (current_time - last_alien_spawn >= ((double)ALIEN_SPAWN_INTERVAL / 1000.0) && boss_phase == 0) {
            regenerate_aliens(aliens, &alien_count);
            last_alien_spawn = current_time;
        }

        move_projectiles();

        for (int i = 0; i < MAX_PROJECTILE; i++) {
            if (projectiles[i].is_active) {
                if (boss_phase == 0) {
                    if (check_projectile_collision(projectiles[i].x, projectiles[i].y, aliens, &alien_count)) {
                        projectiles[i].is_active = 0;
                    }
                } else if (boss_phase == 1) {
                    int result = check_projectile_boss_collision(projectiles[i].x, projectiles[i].y, &boss);
                    if (result > 0) {
                        projectiles[i].is_active = 0;
                    }
                }
            }
        }

        // Desenare stâlp de avertizare
        draw_warning_pillar(COLS - 10, LINES - 8, boss_phase == 0 ? 1 : 2);  // Roșu în timpul invaziei, albastru după
        
        // Desenare navă și proiectile
        draw_spaceship(ship_x, ship_y);
        draw_projectiles();
        
        // Desenare aliens sau boss
        if (boss_phase == 0) {
            printAliens(aliens);
        } else {
            printBoss(&boss);
        }

        mvprintw(LINES - 2, 0, "Arma curentă: %s (Gloanțe: %d)",
                 arme_selectate[current_weapon].nume,
                 arme_selectate[current_weapon].gloanțe_disp);
        
        if (boss_phase == 0) {
            mvprintw(LINES - 1, 0, "Extratereștri rămași: %d", alien_count);
        } else {
            mvprintw(LINES - 1, 0, "Viață Boss: %d", boss.health);
        }

        refresh();

        usleep(16666);

        int ch = getch();
        switch (ch) {
            case KEY_LEFT:
                if (ship_x > 0) ship_x -= 1;
                break;
            case KEY_RIGHT:
                if (ship_x < COLS - 7) ship_x += 1;
                break;
            case ' ':
                if (current_weapon < num_arme_selectate && arme_selectate[current_weapon].gloanțe_disp > 0) {
                    arme_selectate[current_weapon].gloanțe_disp--;
                    shoot_projectile(
                        ship_x + 3,
                        ship_y - 1,
                        arme_selectate[current_weapon].damage,
                        2,
                        arme_selectate[current_weapon].damage % 4 + 1,
                        &projectile_count,
                        &arme_selectate[current_weapon]
                    );
                }
                break;
            case '1': case '2': case '3': case '4': case '5':
                {
                    int new_weapon = ch - '1';
                    if (new_weapon < num_arme_selectate) {
                        current_weapon = new_weapon;
                        mvprintw(LINES - 3, 0, "Schimbat pe arma %s", arme_selectate[current_weapon].nume);
                    }
                }
                break;
            case ENTER:
                if (num_arme_selectate > 0) {
                    current_weapon = (current_weapon + 1) % num_arme_selectate;
                    mvprintw(LINES - 3, 0, "Schimbat pe arma %s", arme_selectate[current_weapon].nume);
                }
                break;
            case 'q':
                game_over = 1;
                break;
        }

        // Verificare coliziuni cu nava
        if (boss_phase == 0) {
            if (check_alien_collision(aliens, ship_x, ship_y)) {
                game_over = 1;
                mvprintw(LINES / 2, COLS / 2 - 10, "Game Over! Ai fost lovit de un extraterestru.");
                refresh();
                nodelay(stdscr, FALSE);
                getch();
            }
            
            // Verifică trecerea la faza cu boss
            if (alien_count <= 0) {
                boss_phase = 1;
                createBoss(&boss);
                
                // Reîncarcă muniția pentru lupta cu boss-ul
                for (int i = 0; i < num_arme_selectate; i++) {
                    arme_selectate[i].gloanțe_disp = arme[i].gloanțe_disp;
                }
                
                mvprintw(LINES / 2, COLS / 2 - 15, "Toți extratereștrii au fost eliminați! Se apropie BOSS-ul!");
                refresh();
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
            }
        } else {
            if (check_boss_collision(&boss, ship_x, ship_y)) {
                game_over = 1;
                mvprintw(LINES / 2, COLS / 2 - 10, "Game Over! Ai fost lovit de BOSS.");
                refresh();
                nodelay(stdscr, FALSE);
                getch();
            }
            
            // Verifică dacă boss-ul a fost învins
            if (boss.health <= 0) {
                game_over = 1;
                mvprintw(LINES / 2, COLS / 2 - 10, "Victorie! Ai învins BOSS-ul și ai salvat planeta!");
                refresh();
                nodelay(stdscr, FALSE);
                getch();
            }
        }
    }

    nodelay(stdscr, FALSE);
    free(aliens);
    timeout(-1);
}
void multiplayer_game(int socket_fd, int is_host) {
    // Setează socket-ul ca non-blocking
    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Inițializarea jucătorilor
    player_t player1, player2;
    player1.x = COLS / 3;
    player1.y = LINES - 8;
    player1.score = 0;
    player1.current_weapon = 0;
    player1.is_alive = 1;
    
    player2.x = 2 * COLS / 3;
    player2.y = LINES - 8;
    player2.score = 0;
    player2.current_weapon = 0;
    player2.is_alive = 1;
    
    // Inițializare aliens și proiectile
    alien_t *aliens = malloc(NUMBER_OF_ALIENS * sizeof(alien_t));
    if (!aliens) {
        endwin();
        printf("Eroare alocare memorie!\n");
        exit(1);
    }
    
    initialize_random();
    createAliens(aliens);
    init_projectiles();
    
    boss_t boss;
    int boss_phase = 0; // 0 = faza cu aliens, 1 = faza cu boss, 2 = pvp
    int alien_count = NUMBER_OF_ALIENS;
    
    // Inițializare arme
    arma_t arme[NUM_ARME_DISPONIBILE];
    arma_t arme_selectate[MAX_TOTAL_ARME_SELECTATE];
    int num_arme_selectate;
    int projectile_count = 0;
    
    init_arme(arme);
    select_arme(arme, arme_selectate, &num_arme_selectate);
    
    int game_over = 0;
    char send_buffer[BUFFER_SIZE] = {0};
    char recv_buffer[BUFFER_SIZE] = {0};
    int bytes_received;
    
    time_t last_alien_move = time(NULL);
    time_t last_alien_spawn = time(NULL);
    
    // Bucla de joc
    while (!game_over) {
        clear();
        
        // Mișcarea aliens și regenerare
        time_t current_time = time(NULL);
        if (current_time - last_alien_move >= ((double)ALIEN_MOVE_DELAY / 1000.0)) {
            moveAliens(aliens);
            last_alien_move = current_time;
        }
        
        if (current_time - last_alien_spawn >= ((double)ALIEN_SPAWN_INTERVAL / 1000.0) && boss_phase == 0) {
            regenerate_aliens(aliens, &alien_count);
            last_alien_spawn = current_time;
        }
        
        // Mișcarea proiectilelor și verificarea coliziunilor
        move_projectiles();
        
        for (int i = 0; i < MAX_PROJECTILE; i++) {
            if (projectiles[i].is_active) {
                if (boss_phase == 0) {
                    if (check_projectile_collision(projectiles[i].x, projectiles[i].y, aliens, &alien_count)) {
                        projectiles[i].is_active = 0;
                        if (is_host) player1.score += 10;
                        else player2.score += 10;
                    }
                } else if (boss_phase == 1) {
                    int result = check_projectile_boss_collision(projectiles[i].x, projectiles[i].y, &boss);
                    if (result > 0) {
                        projectiles[i].is_active = 0;
                        if (is_host) player1.score += 20;
                        else player2.score += 20;
                        
                        if (result == 2) {
                            boss_phase = 2; // Trece la faza PvP
                        }
                    }
                } else if (boss_phase == 2) {
                    // Verificare lovituri între jucători
                    if (is_host) {
                        // Verifică dacă proiectilul jucătorului 1 a lovit jucătorul 2
                        if (projectiles[i].x >= player2.x && projectiles[i].x < player2.x + 6 &&
                            projectiles[i].y >= player2.y && projectiles[i].y < player2.y + 7) {
                            projectiles[i].is_active = 0;
                            player2.is_alive = 0;
                            game_over = 1;
                        }
                    } else {
                        // Verifică dacă proiectilul jucătorului 2 a lovit jucătorul 1
                        if (projectiles[i].x >= player1.x && projectiles[i].x < player1.x + 6 &&
                            projectiles[i].y >= player1.y && projectiles[i].y < player1.y + 7) {
                            projectiles[i].is_active = 0;
                            player1.is_alive = 0;
                            game_over = 1;
                        }
                    }
                }
            }
        }
        
        // Desenarea elementelor de joc
        if (boss_phase == 0) {
            // Desenează stâlpul de avertizare (roșu în timpul invaziei aliens)
            draw_warning_pillar(COLS - 10, LINES - 8, 1);
            printAliens(aliens);
        } else if (boss_phase == 1) {
            // Desenează stâlpul albastru după eliminarea tuturor aliens
            draw_warning_pillar(COLS - 10, LINES - 8, 2);
            printBoss(&boss);
        }
        
        // Desenează jucătorii și proiectilele
        draw_spaceship(player1.x, player1.y);
        draw_spaceship(player2.x, player2.y);
        draw_projectiles();
        
        // Afișează informații despre joc
        mvprintw(LINES - 2, 0, "Jucător %d: Scor %d | Arma: %s (Gloanțe: %d)",
                 is_host ? 1 : 2,
                 is_host ? player1.score : player2.score,
                 arme_selectate[is_host ? player1.current_weapon : player2.current_weapon].nume,
                 arme_selectate[is_host ? player1.current_weapon : player2.current_weapon].gloanțe_disp);
                 
        mvprintw(LINES - 1, 0, "Extratereștri rămași: %d | Faza: %s",
                 alien_count,
                 boss_phase == 0 ? "Invazie" : (boss_phase == 1 ? "Boss" : "PvP"));
        
        refresh();
        
        // Verifică tranziția între faze
        if (boss_phase == 0 && alien_count <= 0) {
            boss_phase = 1;
            createBoss(&boss);
        }
        
        // Procesează input de la tastatură
        int ch = getch();
        if (is_host) {
            // Control jucător 1 (host)
            switch (ch) {
                case KEY_LEFT:
                    if (player1.x > 0) player1.x -= 1;
                    break;
                case KEY_RIGHT:
                    if (player1.x < COLS - 7) player1.x += 1;
                    break;
                case ' ':
                    if (player1.current_weapon < num_arme_selectate && 
                        arme_selectate[player1.current_weapon].gloanțe_disp > 0) {
                        arme_selectate[player1.current_weapon].gloanțe_disp--;
                        shoot_projectile(
                            player1.x + 3,
                            player1.y - 1,
                            arme_selectate[player1.current_weapon].damage,
                            2,
                            arme_selectate[player1.current_weapon].damage % 4 + 1,
                            &projectile_count,
                            &arme_selectate[player1.current_weapon]
                        );
                    }
                    break;
                case ENTER:
                    if (num_arme_selectate > 0) {
                        player1.current_weapon = (player1.current_weapon + 1) % num_arme_selectate;
                    }
                    break;
                case 'q':
                    game_over = 1;
                    break;
            }
            
            // Trimite datele jucătorului 1 către jucătorul 2
            sprintf(send_buffer, "%d,%d,%d,%d", player1.x, player1.y, player1.score, player1.current_weapon);
            send(socket_fd, send_buffer, strlen(send_buffer), 0);
            
            // Primește datele jucătorului 2
            memset(recv_buffer, 0, BUFFER_SIZE);
            bytes_received = recv(socket_fd, recv_buffer, BUFFER_SIZE, 0);
            if (bytes_received > 0) {
                sscanf(recv_buffer, "%d,%d,%d,%d", &player2.x, &player2.y, &player2.score, &player2.current_weapon);
            }
        } else {
            // Control jucător 2 (client)
            switch (ch) {
                case KEY_LEFT:
                    if (player2.x > 0) player2.x -= 1;
                    break;
                case KEY_RIGHT:
                    if (player2.x < COLS - 7) player2.x += 1;
                    break;
                case ' ':
                    if (player2.current_weapon < num_arme_selectate && 
                        arme_selectate[player2.current_weapon].gloanțe_disp > 0) {
                        arme_selectate[player2.current_weapon].gloanțe_disp--;
                        shoot_projectile(
                            player2.x + 3,
                            player2.y - 1,
                            arme_selectate[player2.current_weapon].damage,
                            2,
                            arme_selectate[player2.current_weapon].damage % 4 + 1,
                            &projectile_count,
                            &arme_selectate[player2.current_weapon]
                        );
                    }
                    break;
                case ENTER:
                    if (num_arme_selectate > 0) {
                        player2.current_weapon = (player2.current_weapon + 1) % num_arme_selectate;
                    }
                    break;
                case 'q':
                    game_over = 1;
                    break;
            }
            
            // Trimite datele jucătorului 2 către jucătorul 1
            sprintf(send_buffer, "%d,%d,%d,%d", player2.x, player2.y, player2.score, player2.current_weapon);
            send(socket_fd, send_buffer, strlen(send_buffer), 0);
            
            // Primește datele jucătorului 1
            memset(recv_buffer, 0, BUFFER_SIZE);
            bytes_received = recv(socket_fd, recv_buffer, BUFFER_SIZE, 0);
            if (bytes_received > 0) {
                sscanf(recv_buffer, "%d,%d,%d,%d", &player1.x, &player1.y, &player1.score, &player1.current_weapon);
            }
        }
        
        usleep(16666); // ~60 FPS
    }
    
    // Afișare rezultat
    clear();
    if (boss_phase == 2) {
        if ((is_host && player2.is_alive == 0) || (!is_host && player1.is_alive == 0)) {
            mvprintw(LINES / 2, COLS / 2 - 10, "Ai câștigat duelul!");
        } else {
            mvprintw(LINES / 2, COLS / 2 - 10, "Ai pierdut duelul!");
        }
    } else {
        mvprintw(LINES / 2, COLS / 2 - 10, "Joc terminat. Scor final: %d", is_host ? player1.score : player2.score);
    }
    
    refresh();
    getch();
    
    free(aliens);
}

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
        if (key == ENTER) {
            if (choice == 0) multiplayer_host();
            else if (choice == 1) multiplayer_join();
            else if (choice == 2) break;
        }
    }
}

int main() {
    int choice = 0;
    int key;

    init_ncurses();

    while (1) {
        clear();
        print_title();
        print_menu(choice);
        refresh();

        key = getch();
        if (key == KEY_UP) choice = (choice - 1 + 3) % 3;
        if (key == KEY_DOWN) choice = (choice + 1) % 3;
        if (key == ENTER) {
            if (choice == 2) break;
            else if (choice == 0) single_player_mode();
            else if (choice == 1) multiplayer_mode(); // Acum apelăm funcția multiplayer_mode()
        }
    }

    close_ncurses();
    return 0;
}