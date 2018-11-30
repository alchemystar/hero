#ifndef HERO_NET_WORK_H
#include "proto/packet.h"
#include "basic.h"

void handle_one_connection(int sockfd);

int readn(int sockfd,int size,unsigned char* recv_buff);

int writen(int sockfd,int size ,unsigned char* write_buff);

#endif