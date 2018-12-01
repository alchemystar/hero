#ifndef HERO_NET_WORK_H
#include "proto/packet.h"
#include "basic.h"

int handle_one_connection(int sockfd,mem_pool* pool);

int readn(int sockfd,int size,unsigned char* recv_buff);

int writen(int sockfd,int size ,unsigned char* write_buff);

void read_query(int sockfd, mem_pool* pool);

#endif