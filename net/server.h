#ifndef HERO_SERVER_H
#define HERO_SERVER_H 

#include <stdio.h>
#include "config.h"
#include "proto/packet.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include "hero_poll.h"

void start_server();

#endif