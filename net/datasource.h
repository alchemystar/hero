#ifndef HERO_DATASOURCE
#define HERO_DATASOURCE
#include "config.h"
#include "conn.h"
#include "hero_poll.h"

typedef struct _Datasource{
    // 互斥锁
    pthread_mutex_t mutex;
    // db address
    struct sockaddr_in db_addr;
    // datasource对应的内存池
    mem_pool* pool;
    // 当前conn的position
    int current_conn_pos;
    // 连接池size
    int conn_pool_size;
    // 连接池数组
    connection** conn_array;
    // 对应的反应堆模型
    Reactor* reactor;
}Datasource;

int init_datasource(Reactor* reactor);

int put_conn_to_datasource(connection* conn,Datasource* datasource);
connection* get_conn_from_datasource(Datasource* datasource);

#endif