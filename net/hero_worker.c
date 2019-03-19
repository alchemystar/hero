#include "hero_worker.h"
#include <pthread.h>
#include "basic.h"
#include "hero_poll.h"
#ifdef HERO_USE_EPOLL
#include <sys/epoll.h>
#include "network.h"
#include "query.h"

// 多进程方案待完善，当前先用多线程
// linux kernerl 2.6.32已经在epoll_wait里面加入了WQ_FLAG_EXCLUSIVE选项，同时，我们这边使用了reuseport
// 所以不必担心惊群问题，但是可能会出现进程负载不均衡问题,多进程待调研负载均衡

typedef struct _hero_thread_info {
    int worker_fd;
    void* priv;
}Hero_thread_info;

int handle_front_auth(connection* conn);

void handle_front_command(connection* conn);

void handle_front_conn(connection* conn);

back_conn* init_back_conn(connection* conn);

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
                    printf("connection fd=%d,epoll hull up\n",conn->sockfd);
                    // close fd后，kernel自动删除对应的event
                    release_conn_and_mempool(conn);
                    continue;
                }else if(event & EPOLLERR){
                    printf("connection fd=%d,epoll error,errno=%d\n",conn->sockfd,errno);
                    release_conn_and_mempool(conn);
                    continue;
                }else {
                    if(event & EPOLLIN){
                        handle_ready_read_connection(conn);
                        continue;
                    }
                    if(event & EPOLLOUT){
                        printf("handle epoll out and write\n");
                        hanlde_ready_write_connection(conn);
                        continue;
                    }
                }
            }
        }
    }
}

void handle_front_conn(connection* conn){
    int retval = read_packet(conn);
    switch(retval){
        case READ_WAIT_FOR_EVENT:
            // wait for next read event
            return;
        case READ_PACKET_SUCCESS:
            if(NOT_AUTHED == conn->front->auth_state){
                if(FALSE == handle_front_auth(conn)){
                    release_conn_and_mempool(conn);
                    return ;
                }
            }else{
                handle_front_command(conn);
            }
            return;
        case READ_PACKET_LENGTH_MORE_THAN_MAX:
        case MEMORY_EXHAUSTED:
        case READ_PACKET_ERROR:    
        default:
            release_conn_and_mempool(conn);
            return;
    }  
}

void handle_back_conn(connection* conn){
    if(NULL == conn->back){
        // 初始化front connection,注意这边并不是以0填充的
        back_conn* back = init_back_conn(conn);
        // 需设置read_limit为0
        conn->read_buffer->read_limit=0;
        if(NULL == back){
            goto error_process;
        }
    }
    int retval = read_packet(conn);
    switch(retval){
        case READ_WAIT_FOR_EVENT:
            // wait for next read event
            return;
        case READ_PACKET_SUCCESS:
            printf("got one complete packet\n");
            if(AUTHED != conn->back->auth_state){
                if(FALSE == handle_backend_auth(conn)){
                    release_conn_and_mempool(conn);
                    return ;
                }
            }else{
                handle_backend_command(conn);
            }
            return;
        case READ_PACKET_LENGTH_MORE_THAN_MAX:
        case MEMORY_EXHAUSTED:
        case READ_PACKET_ERROR:    
        default:
            release_conn_and_mempool(conn);
            return;
    }  
error_process:
    release_conn_and_mempool(conn);
    return;    
}

void handle_ready_read_connection(connection* conn){
    if(IS_FRONT_CONN == conn->is_front_or_back){
        handle_front_conn(conn);
    }else if(IS_BACK_CONN == conn->is_front_or_back){
        handle_back_conn(conn);
    }
}

void handle_front_command(connection* conn) {
    int command_type = read_byte(conn->read_buffer);
    // 当前仅处理query
    if(-1 == command_type){
        release_conn_and_mempool(conn);
        return;  
    }
    // printf("command_type=%d\n",command_type);
    switch(command_type){
        case COM_QUIT:
            release_conn_and_mempool(conn);
            break;
        case COM_QUERY:  
	        if(FALSE == handle_com_query(conn->front)){
	            release_conn_and_mempool(conn);
	            return;                        
	        }
            break;
        default:
            if(FALSE == write_okay(conn)){
                release_conn_and_mempool(conn);
	            return;
            }
            break;    
    }
}

// todo result set 分包以后再搞
int handle_backend_resultset(connection* conn){
    printf("now handling result set packet\n");
    if(RESULT_SET_FIELD_COUNT == conn->back->select_status){
        //
    }
    // reset_conn_from_one_request(conn);
    return TRUE;
}

int handle_backend_normal(connection* conn){
    return TRUE;
}

void handle_backend_command(connection* conn){
    printf("read_limit size = %d\n",conn->read_buffer->read_limit);
    // todo 有并发问题
    front_conn* front = conn->front;
    if(NULL == front){
        printf("front_conn is NULL\n");
        return ;
    }
    connection* to_conn = front->conn;
    write_bytes(to_conn->write_buffer,conn->read_buffer->buffer,conn->read_buffer->read_limit);
    // write之后则reset
    reset_conn_from_one_request(conn);
    write_nonblock(to_conn);
//    if(TRUE == conn->back->selecting){
//         if(FALSE == handle_backend_resultset(conn)){
//           	release_conn_and_mempool(conn);
// 	        return;    
//         }
//    }else{
//        if(FALSE == handle_backend_normal(conn)){
//           	release_conn_and_mempool(conn);
// 	        return;             
//        }
//    }
}


int handle_backend_auth(connection* conn){
    if(AUTHED == conn->back->auth_state){
        printf("auth_state not right ,auth_state = %d\n",conn->back->auth_state);
        return FALSE;
    }
    if(NOT_GET_HANDSHAKE == conn->back->auth_state){
        if(!read_handshake(conn)){
            return FALSE;
        }else{
            conn->back->auth_state = NOT_AUTHED;
            if(!send_auth_packet(conn)){
               return FALSE; 
            }else{
                return write_nonblock(conn);
            }
        }
    }
    if(NOT_AUTHED == conn->back->auth_state){
        int okay = read_byte(conn->read_buffer);
        printf("okay_type = %d\n",okay);
        switch(okay){
            case OKAY_PACKET_FIELD_COUNT:
                printf("got okay packet\n");
                conn->back->auth_state = AUTHED;
                put_conn_to_datasource(conn,conn->datasource);
                break;
            case ERROR_PACKET_FIELD_COUNT:
                printf("got error packet\n");
                read_error_packet(conn);
                break;
        }
    }
    return TRUE;
}

int handle_front_auth(connection* conn){
    printf("handle front auth\n");
    if(conn->front->auth_state != NOT_AUTHED){
        printf("auth_state not right ,auth_state = %d\n",conn->front->auth_state);
        return FALSE;
    }
    if(FALSE == read_and_check_auth(conn)){
       return FALSE;
    }
    // 推进auth状态
    conn->front->auth_state = AUTHED;
    // 对buffer中写入auth_okay的数据
    write_bytes(conn->write_buffer,AUTH_OKAY,AUTH_OKAY_SIZE);
    // 开启可写事件，并关闭可读
    // mem_pool在一个请求结束的时候才reset
    return write_nonblock(conn);
}

int write_auth_okay(connection* conn){
    write_bytes(conn->write_buffer,AUTH_OKAY,AUTH_OKAY_SIZE);
    return write_nonblock(conn);
}

int write_okay(connection* conn){
    write_bytes(conn->write_buffer,OKAY,OKAY_SIZE);
    return write_nonblock(conn);
}

void hanlde_ready_write_connection(connection* conn){
    if(conn->is_front_or_back == IS_FRONT_CONN){
        if(conn->front == NULL){
            // 初始化front connection,注意这边并不是以0填充的
            front_conn* front = (front_conn*) mem_pool_alloc(sizeof(front_conn),conn->meta_pool);
            front->conn = conn;
            conn->front = front;
            if(front->auth_state == NOT_SEND_HANDSHAKE){
                // 推进状态
                front->auth_state = NOT_AUTHED;
                printf("not send handshake\n");
                if(FALSE == write_handshake_to_buffer(conn)){
                    printf("write handshake null\n");
                    // 无需删除对应的front_conn,因为是在conn内存池中分配的
                    release_conn_and_mempool(conn);
                }else{
                    if(FALSE == write_nonblock(conn)){
                        release_conn_and_mempool(conn);
                    }
                }
            }else{
                printf("state is %d \n",front->auth_state);
            }
        }else{
           printf("write something \n");
           if(FALSE == write_nonblock(conn)){
               release_conn_and_mempool(conn);
           }
        }
    }else if(conn->is_front_or_back == IS_BACK_CONN){
        printf("it's a backend connection writable\n");
        printf("write something \n");
        if(FALSE == write_nonblock(conn)){
            release_conn_and_mempool(conn);
        }
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

// 由于write packet时候已经expand了write_buffer,所以无需考虑buffer不够问题
int write_nonblock(connection* conn){
    int epfd = conn->epfd;
    // 剩余数据量
    size_t nleft=conn->write_buffer->pos - conn->write_buffer->write_index;
    ssize_t nwritten=0;
    unsigned char* ptr = conn->write_buffer->buffer;
    if((nwritten = write(conn->sockfd,ptr,nleft)) <= 0){
        if(nwritten <0 && (errno == EINTR || errno == EWOULDBLOCK)){
            nwritten = 0;
        }else{
            printf("write connection error,sockfd=%d,errno=%d\n",conn->sockfd,errno);
            return FALSE;
        }        
    }
    printf("write bytes = %d\n",nwritten);
    conn->write_buffer->write_index += nwritten;
    nleft -= nwritten;
    printf("nleft = %d\n",nleft);
    if(nleft == 0){
        // 重置conn,到这表示一个请求结束了，重置buffer
        reset_conn_from_one_request(conn);
        // 取消写事件,并启动读事件
        return enable_conn_read_and_disable_write(conn);
    }else{
        // 否则打开写事件，取消读事件
        return enable_conn_write_and_disable_read(conn);
    }
}

back_conn* init_back_conn(connection* conn){
     back_conn* back = (back_conn*) mem_pool_alloc(sizeof(back_conn),conn->meta_pool);
     if(NULL == back){
         return NULL;
     }
     back->conn = conn;
     conn->back = back;
     back->user = BACKEND_USER_NAME;
     back->schema = NULL;
     back->password = BACKEND_PASS_WORD;
     back->auth_state = NOT_GET_HANDSHAKE;
     back->selecting = TRUE;
     back->select_status = RESULT_SET_FIELD_COUNT;
     back->front = NULL;
     // 需要设置为conn reading
     conn->reading_or_writing = CONN_READING;
     return back;
}

#endif