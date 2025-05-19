#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <ncurses.h>
#include "aliens.h"
#include "arms.h"
#include "nava.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_PLAYERS 2

// Funcția principală multiplayer
void multiplayer_mode();

#endif