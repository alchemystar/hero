#ifndef HERO_SERVER_H
#include <stdio.h>
#include "config.h"
#include "proto/packet.h"

void start_server();

void process_connection(int sockfd);

#endif