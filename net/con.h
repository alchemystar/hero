#ifndef HERO_CON_H
#define HERO_CON_H
#include "basic.h"
#include "proto/packet.h"
#include "stdatomic.h"

typedef int (*handle_fun)(front_connection* conn,char* sql);

// connection [front,end]
typedef struct connection{
    long id;
    // ip v6最高46位，取整48
    char ip[48];
    // 端口号
    int port;
    // 对应的sock_fd
    int sockfd;
    // 对应的内存池
    mem_pool* pool;
    // 读写buffer
    packet_buffer* read_buffer;
    packet_buffer* write_buffer;
    // header char,用作decode
    unsigned char header[4];
    // 标识当前packet_id
    // 状态信息=>atomic
    atomic_int is_closed;
    // 统计信息
    int start_time;
    int last_read_time;
    int last_write_time;
    int query_times;
}front_connection;

#endif