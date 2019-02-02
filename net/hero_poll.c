#include "hero_poll.h"
#include <sys/epoll.h>
#include <sys/errno.h>
#include "hero_worker.h"

// reactor模式创建
// reactor的内存一直存在，暂时没必要删除,只有在出错时刻删除
// worker_count用linux proc文件读取获得
int init_reactor(int listen_fd,int worker_count){
    mem_pool* pool = (mem_pool*)mem_pool_create(DEFAULT_MEM_POOL_SIZE);
    Reactor *reactor = mem_alloc(sizeof(Reactor));
    reactor->events = mem_alloc(sizeof(struct epoll_event) * EPOLL_MAX_EVENTS);
    reactor->master_fd = epoll_create(EPOLL_MAX_EVENTS);
    if(reactor->master_fd == -1){
        goto error_process;
    }
    reactor->worker_fd_arrays = mem_alloc(sizeof(int) * worker_count);
    reactor->worker_count = worker_count;
    for(int i=0;i<worker_count;i++){
        reactor->worker_fd_arrays[i] = epoll_create(EPOLL_MAX_EVENTS);
        if(reactor->worker_fd_arrays[i] == -1){
           goto error_process;
        }
        if(FALSE == create_and_start_rw_thread(reactor->worker_fd_arrays[i],pool)){
            goto error_process;
        }
    }
    if(-1 == poll_add_event(reactor->master_fd,listen_fd,EPOLLIN,NULL)){
        goto error_process;
    }
    int current_worker=0;
    for(;;){
        int numevents = 0;
        int retval = epoll_wait(reactor->master_fd,reactor->events,EPOLL_MAX_EVENTS,500);     
        if(retval > 0){
            int j;
            numevents = retval;
            // 块内作用域会被回收
            struct epoll_event *e;
            for(j=0; j < numevents; j++){
                e = reactor->events+j;
                // accept主循环中只有一个fd
                if(e->data.fd == listen_fd){
                    int client_fd;
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    if (0 > (client_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &client_len))) {
                        continue;
                    }
                    connection *conn = init_conn_ and_mempoll(client_fd,&client_addr);
                    if(conn == NULL){
                        continue;
                    }
                    // 首先是触发可写事件,因为是三次握手之后，主动发起请求
                    if(-1 == poll_add_event(reactor->worker_fd_arrays[(current_worker++)%(reactor->worker_count)],conn->sockfd,EPOLLOUT,conn)){
                        // 添加失败，则close掉连接
                        release_conn_and_mempoll(conn);
                    }
                }
            }
        }else{
            if(retval == -1){
                // 对应信号打断需要做特殊处理
                if(errno == EINTR){
                    continue;
                }
            }else if(retval == 0){
                // 无可用数据
                continue;
            }
            printf("main accept epoll has some exception,error=%d\n",errno);
        }   
    }
    return FALSE;
error_process:
    mem_free(reactor->worker_fd_arrays);
    mem_free(reactor->events);
    mem_free(reactor);
    mem_pool_free(pool);
    return FALSE;
}

// todo epoll add event function
int poll_add_event(int epfd,int epifd,int mask,void* ptr){
    // 栈上变量，传递给kernel,会复制
    struct epoll_event event = {0};
    // 不需要加EPOLLHUP或EPOLLERR,因为kernel会自动加上
    event.events = mask;
    if(ptr == NULL){
        // 此种情况在listen fd时候出现
        event.data.fd = epifd;
    }else{
        event.data.ptr = ptr; 
    }
    printf("get one connection,epifd=%d\n",epifd);
    return epoll_ctl(epfd,EPOLL_CTL_ADD,epifd,&event);
}