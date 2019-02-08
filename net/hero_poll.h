#ifndef HERO_POLL
#define HERO_POLL
#include "conn.h"
#include "basic.h"

#define EPOLL_MAX_EVENTS 1024
#define DEFAULT_EPOLL_WAIT_TIMEOUT 500

#ifdef HERO_USE_EPOLL
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

int init_reactor(int listen_fd,int worker_count);

#endif
#endif