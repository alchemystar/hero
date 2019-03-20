#ifndef HERO_CON_H
#define HERO_CON_H
#include "basic.h"
#include "proto/packet.h"
#include "stdatomic.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <memory.h>
#include <sys/epoll.h>

#define IS_FRONT_CONN 1
#define IS_BACK_CONN 0
// front status
#define NOT_SEND_HANDSHAKE 0
#define NOT_GET_HANDSHAKE 0
#define NOT_AUTHED 1
#define AUTHED 2

#define RESULT_SET_FIELD_COUNT 1
#define RESULT_SET_FIELDS 2
#define RESULT_SET_EOF 3
#define RESULT_SET_ROW 4
#define RESULT_SET_LAST_EOF 5

#define CONN_WRITING 0
#define CONN_READING 1


// for function pointer
struct _front_conn;
struct _back_conn;
struct _Datasource;

typedef int (*handle_fun)(struct _front_conn* conn,char* sql);

typedef int (*resp_fun)(struct _front_conn* conn,unsigned char* buffer);

// connection [front,end]
typedef struct _connection{
    pthread_mutex_t mutex;
    // 对应的sock_fd
    int sockfd;   
    // sockaddr
    struct sockaddr_in* addr;    
    // connection元信息内存池
    mem_pool* meta_pool;
    // 单次请求的内存池
    mem_pool* request_pool;    
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
    // header char,用作decode todo 删除
    unsigned char header[4];
    // 记录当前packet_id,由于是calloc=>0
    int packet_id;
    // 记录当前packet_length的长度
    int packet_length;
    // 标识当前packet_id
    // 状态信息=>atomic
    atomic_int is_closed;
    // 如果是backend,则指向其datasource
    struct _Datasource* datasource;
    // 统计信息
    struct timeval start_time;
    struct timeval last_read_time;
    struct timeval last_write_time;
    int query_count;
    // 其关联的是前端还是后端连接
    int is_front_or_back;
    struct _front_conn* front;
    struct _back_conn* back;
    // 当前读到的报文头长度
    int header_read_len;
    // 当前读到的body长度
    int body_read_len;
    // connection对应的epoll fd
    int epfd;
    int reading_or_writing;
}connection;

// 前端连接
typedef struct _front_conn{
    connection* conn;
    // 请求函数
    handle_fun handle;
    // 是否已经auth处理
    int auth_state;
    // 用户名
    char* user;
    // schema
    char* schema;
    // 当前正在处理的sql
    char* sql;
    // 当前的database
    char* database;
    // 对应的后端连接
    struct _back_conn* back;
    // 对应的握手包
    hand_shake_packet* hand_shake;
}front_conn;

typedef struct _back_conn{
    connection* conn;
    long client_flags;
    // 对应的后端user
    char* user;
    // 对应的后端schema
    char* schema;
    // 密码
    char* password;
    // 后端认证状态
    int auth_state;
    // 当前sql是否使用select
    int selecting;
    // select状态
    int select_status;
    // 是否正在使用
    volatile int borrowed;
    // 对应的前端连接
    struct _front_conn* front;
    // 对应的处理器
    resp_fun handle;
    // 对端发过来的hand_shake_packet
    hand_shake_packet* hand_shake;

}back_conn;

int init_conn(connection* conn,int sockfd,struct sockaddr_in* addr,mem_pool* pool);

packet_buffer* get_conn_read_buffer_with_size(connection* conn,int size);

packet_buffer* get_conn_write_buffer_with_size(connection* conn,int size);

//简单函数 直接内联
inline static packet_buffer * get_conn_write_buffer(connection* conn){
    return conn->write_buffer;
}

inline static void reset_conn_read_buffer(connection* conn){
    reset_packet_buffer(conn->read_buffer);
}

inline static void reset_conn_write_buffer(connection* conn){
    reset_packet_buffer(conn->write_buffer);
}

void free_front_conn(front_conn* front);

void reset_conn_from_one_request(connection* conn);

connection* init_conn_and_mempool(int sockfd,struct sockaddr_in* addr,int front_or_back);

void release_conn_and_mempool(connection* conn);

inline static int set_fd_flags(int fd) {

    if (fd < 0) {
        return FALSE;
    }
    int opts;
    if (0 > (opts = fcntl(fd, F_GETFL))) {
        return FALSE;
    }
    // 这边置为非阻塞
    opts = opts | O_NONBLOCK;
    if (0 > fcntl(fd, F_SETFL, opts)) {
        return FALSE;
    }
    struct linger li;
    memset(&li, 0, sizeof(li));
    // close时候丢弃buffer,同时发送rst,避免time_wait状态
    li.l_onoff = 1;
    li.l_linger = 0;
    int snd_size = 1024 * 16 * 2;  
    int recv_size = 1024 * 16 * 2; 
    // 设置读写buffer
    if (0 != setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &snd_size, sizeof(snd_size))){
        return FALSE;
    }
    if(0 !=  setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &recv_size, sizeof(recv_size))){
        return FALSE;
    }
    if (0 != setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char*) &li, sizeof(li))) {
        return FALSE;
    }
    int var = 1;
    if (0 != setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &var, sizeof(var))) {
        return FALSE;
    }
    return TRUE;
}

int poll_add_event(int epfd,int epifd,int mask,void* ptr);
int poll_mod_event(int epfd,int epifd,int mask,void* ptr);
int enable_conn_write_and_disable_read(connection* conn);
int enable_conn_read_and_disable_write(connection* conn);

int is_need_lock(connection* conn);

#endif