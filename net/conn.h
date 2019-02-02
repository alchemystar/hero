#ifndef HERO_CON_H
#define HERO_CON_H
#include "basic.h"
#include "proto/packet.h"
#include "stdatomic.h"
#include <sys/socket.h>
#include <netinet/in.h>

// for function pointer
struct _front_conn;
struct _back_conn;

typedef int (*handle_fun)(struct _front_conn* conn,char* sql);

typedef int (*resp_fun)(struct _front_conn* conn,unsigned char* buffer);

// connection [front,end]
typedef struct _connection{
    // 对应的sock_fd
    int sockfd;   
    // sockaddr
    struct sockaddr_in* addr;    
    // 对应的内存池
    mem_pool* pool;    
    // ip v6最高46位，取整48
    char ip[48];
    // 端口号
    int port;
    // 字符集
    int charset_index;
    // 读写buffer
    packet_buffer* read_buffer;
    // todo 设置 rw buffer最大限制
    packet_buffer* write_buffer;
    // header char,用作decode
    unsigned char header[4];
    // 记录当前packet_id,由于是calloc=>0
    int packet_id;
    // 标识当前packet_id
    // 状态信息=>atomic
    atomic_int is_closed;
    // 统计信息
    struct timeval start_time;
    struct timeval last_read_time;
    struct timeval last_write_time;
    int query_count;
}connection;

// 前端连接
typedef struct _front_connn{
    connection conn;
    // 请求函数
    handle_fun handle;
    // 是否已经auth处理
    int is_authed;
    // 用户名
    char* user;
    // schema
    char* schema;
    // 当前正在处理的sql
    char* sql;
}front_conn;

typedef struct _back_conn{
    connection conn;
    long client_flags;
    // 对应的后端user
    char* user;
    // 对应的后端schema
    char* schema;
    // 密码
    char* password;
    // 是否正在使用
    volatile int borrowed;
    // 
    resp_fun handle;
    // 数据源 todo 
}back_conn;

int init_conn(connection* conn,int sockfd,struct sockaddr_in* addr,mem_pool* pool);

packet_buffer* get_conn_read_buffer(connection* conn,int size);

//简单函数 直接内联
inline static packet_buffer * get_conn_write_buffer(connection* conn){
    return conn->write_buffer;
}

void free_front_conn(front_conn* front);

void release_conn_and_mempoll(connection* conn);

#endif