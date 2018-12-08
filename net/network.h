#ifndef HERO_NET_WORK_H
#include "proto/packet.h"
#include "basic.h"
#include <netinet/in.h>
#include "conn.h";

int handle_one_connection(int sockfd,struct sockaddr_in* sockaddr_ptr, mem_pool* pool);

int readn(int sockfd,int size,unsigned char* recv_buff);

int writen(int sockfd,int size ,unsigned char* write_buff);

void read_query(int sockfd, mem_pool* pool);

#endif