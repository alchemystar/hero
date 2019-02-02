#include "hero_worker.h"
#include <pthread.h>
#include "basic.h"
#include "hero_poll.h"
#include <sys/epoll.h>

// 多进程方案待完善，当前先用多线程

typedef struct _hero_thread_info {
    int worker_fd;
    void* priv;
}Hero_thread_info;

static void* rw_thread_func(void* arg){
    printf("rw thread start\n");
    // 忽略信号
    init_signal_handlers();
    Hero_thread_info* thread_info = (Hero_thread_info*)arg;
    int epfd = thread_info->worker_fd;
    struct epoll_event* events = (struct epoll_event*)mem_alloc(sizeof(struct epoll_event) * EPOLL_MAX_EVENTS);
    for(;;){
        int numevents = 0;
        int retval = epoll_wait(epfd,events,EPOLL_MAX_EVENTS,500);
        if(retval > 0){
            int j;
            numevents = retval;
            // 块内作用域会被回收
            struct epoll_event *e;
            int event;
            for(j=0; j < numevents; j++){
                e = events+j;
                event = e->events;
                connection* conn = (connection*)(e->data.ptr);
                if(event & EPOLLHUP){
                    prinft("connection fd=%d,epoll hull up\n",conn->sockfd);
                    release_conn_and_mempoll(conn);
                    continue;
                }else if(event & EPOLLERR){
                    prinft("connection fd=%d,epoll error,errno=%d\n",conn->sockfd,errno);
                    release_conn_and_mempoll(conn);
                    continue;
                }else {
                    hanlde_ready_connection(conn);
                }
            }
        }
    }
}
void hanlde_ready_connection(connection* conn){
    front_conn* front = (front_conn*) mem_pool_alloc(sizeof(front_conn),conn->pool);
    front->conn = conn;
    hand_shake_packet* handshake;
    if(!send_handshake(front,&handshake)){
        return FALSE;
    }
}

// pool 用来统一管理thread_info内存
int create_and_start_rw_thread(int worker_fd,mem_pool* pool){
    Hero_thread_info* thread_info = (Hero_thread_info*)mem_pool_alloc(sizeof(Hero_thread_info),pool);
    thread_info->worker_fd = worker_fd;
    pthread_t* t1 = (pthread_t*)mem_pool_alloc_ignore_check(sizeof(pthread_t),pool);
    if(t1 == NULL){
        return FALSE;
    }
    pthread_attr_t* attr = (pthread_attr_t*)mem_pool_alloc_ignore_check(sizeof(pthread_attr_t),pool);
    if(attr == NULL){
        return FALSE;
    }
    int retval = pthread_attr_setdetachstate(attr,PTHREAD_CREATE_DETACHED);
    if (0 != retval) { 
        return FALSE;
    }
    retval=pthread_create(t1,attr,rw_thread_func,thread_info);
    if (0 != retval){
        printf("create pthread failed\n");
        return FALSE;
    }
    if(0 != pthread_attr_destroy(attr)) {
        printf("destroy pthread attr error\n");
    }
    return TRUE;
}