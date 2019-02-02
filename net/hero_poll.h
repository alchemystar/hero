#ifndef HERO_POLL
#define HERO_POLL
#include "conn.h"
#include "basic.h"

#define EPOLL_MAX_EVENTS 1024
#define DEFAULT_EPOLL_WAIT_TIMEOUT 500

typedef struct Reactor{
    // accept fd
    int master_fd;
    // worker数组
    int *worker_fd_arrays;
    // worker数量
    int worker_count;
    // 主循环epoll数组
    struct epoll_event *events;
}Reactor;

connection* init_conn_and_mempoll(int sockfd,struct sockaddr_in* addr);
int init_reactor(int listen_fd,int worker_count);
int poll_add_event(int epfd,int epifd,int mask,void* ptr);
#endif