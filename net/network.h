#ifndef HERO_NET_WORK_H
#define HERO_NET_WORK_H
#include "proto/packet.h"
#include "basic.h"
#include <netinet/in.h>
#include "conn.h"

// 返回码
#define READ_PACKET_SUCCESS 1
#define READ_PACKET_ERROR 2
#define READ_WAIT_FOR_EVENT 3
#define READ_PACKET_LENGTH_MORE_THAN_MAX 4
#define MEMORY_EXHAUSTED 5

#define AUTH_OKAY_SIZE 11
#define OKAY_SIZE 11
#define FILLER_SIZE 23

// 全局变量extern
extern unsigned char FILLER_HAND_SHAKE[];
extern unsigned char AUTH_OKAY[];
extern unsigned char FILLER[];
extern unsigned char OKAY[];

// 这边由于需要修改指针类型的result，所以需要双重指针
int send_handshake(front_conn* front,hand_shake_packet** result);

int read_auth(front_conn* front,auth_packet** result);

int read_error_packet(connection* conn);

void send_auth_okay(int sockfd);

int check_auth(auth_packet* auth,hand_shake_packet* handshake);

int get_server_capacities();

int handle_one_connection(int sockfd,struct sockaddr_in* sockaddr_ptr, mem_pool* pool);

int readn(int sockfd,int size,unsigned char* recv_buff);

int writen(int sockfd,int size ,unsigned char* write_buff);

void read_query(int sockfd, mem_pool* pool);

int read_packet(connection* conn);

int read_and_check_auth(connection* conn);

#endif