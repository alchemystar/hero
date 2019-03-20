#include "conn.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "basic.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

atomic_int front_conn_id = 0 ;
atomic_int back_conn_id = 0;

connection* init_conn_and_mempool(int sockfd,struct sockaddr_in* addr,int front_or_back){
    if(FALSE == set_fd_flags(sockfd)){
        return NULL;
    }
    mem_pool* pool = mem_pool_create(DEFAULT_MEM_POOL_SIZE);
    if(pool == NULL){
        return NULL;
    }
    connection *conn_addr = (connection*)mem_pool_alloc(sizeof(connection),pool);
    if(conn_addr == NULL){
        goto error_process;
    }
    // 对backend做处理，因为其addr是公用的server addr
    if(FALSE == init_conn(conn_addr,sockfd,addr,pool)){
        goto error_process;
    }
    conn_addr->is_front_or_back = front_or_back;
    return conn_addr;
error_process:
    mem_pool_free(pool);
    return NULL;
}

// release conn 直接就release其对应的内存池以及不从mem_poll分配的buffer
void release_conn_and_mempool(connection* conn){
    printf("connection close,sockfd=%d\n",conn->sockfd);
    close(conn->sockfd);
    free_packet_buffer(conn->read_buffer);
    free_packet_buffer(conn->write_buffer);
    if(conn->request_pool != NULL){
        mem_pool_free(conn->request_pool);
    }
    // meta_pool必须在最后释放
    if(conn->meta_pool != NULL){
        mem_pool_free(conn->meta_pool);
    }
}

int init_conn(connection* conn,int sockfd,struct sockaddr_in* addr,mem_pool* pool) {
    conn->sockfd = sockfd;
    conn->addr = addr;
    conn->meta_pool = pool;
    // 请求内存池，在每次请求过后，重置
    conn->request_pool = mem_pool_create(DEFAULT_MEM_POOL_SIZE);
    pthread_mutex_init(&(conn->mutex),NULL);
    if(conn->request_pool == NULL){
        return FALSE;
    }
    // addr must be initialized
    // Each call to inet_ntoa() in your application will override this area
    // so copy it
    char* ip = (char*)inet_ntoa((*addr).sin_addr);
    if(ip != NULL){
        snprintf((char*)conn->ip,48,"%s",ip);
    }else{
        conn->ip[0] = "\0";
    }
    // here must use htons
    conn->port = htons((*addr).sin_port);
    // 初始化读buffer
    conn->read_buffer = get_packet_buffer(DEFAULT_PB_SIZE);
    if(conn->read_buffer == NULL){
        goto error_process;
    }
    // 初始化写buffer
    conn->write_buffer = get_packet_buffer(DEFAULT_PB_SIZE);
    if(conn->write_buffer == NULL){
        goto error_process;
    }
    conn->is_closed = FALSE;
    // 统计信息
    if(gettimeofday(&conn->start_time,0) != 0){
        printf("get time of day error");
        goto error_process;
    }
    if(gettimeofday(&conn->last_read_time,0) != 0){
        printf("get time of day error");
        goto error_process;
    }
     if(gettimeofday(&conn->last_write_time,0) != 0){
        printf("get time of day error");
        goto error_process;
    }   
    conn->packet_id = 0;
    conn->packet_length = 0;
    conn->query_count = 0;
    conn->front = NULL;
    conn->back = NULL;
    conn->datasource = NULL;
    conn->header_read_len = 0;
    conn->body_read_len = 0;
    conn->epfd = 0;
    conn->reading_or_writing = CONN_WRITING;
    return TRUE;
error_process:
    if(close(sockfd) != 0){
        printf("close sockfd error,sockfd=%d",sockfd);
    }
    mem_pool_free(conn->request_pool); 
    free_packet_buffer(conn->read_buffer);
    free_packet_buffer(conn->write_buffer);
    return FALSE;

}

// todo 在适当时机调整一下read_buffer,不至于过大
packet_buffer* get_conn_read_buffer_with_size(connection* conn,int size){
    packet_buffer* pb = conn->read_buffer;
    if(size > pb->length) {
        if(!expand(pb,size)){
            return NULL;
        }
    }
    return pb;
}

// todo 在适当时机调整一下write_buffer,不至于过大
packet_buffer* get_conn_write_buffer_with_size(connection* conn,int size){
    packet_buffer* pb = conn->write_buffer;
    if(size > pb->length) {
        if(!expand(pb,size)){
            return NULL;
        }
    }
    return pb;
}


void free_front_conn(front_conn* front){
    free_packet_buffer(front->conn->read_buffer);
    free_packet_buffer(front->conn->write_buffer);
    // front_conn本身由mem_pool free
}


void reset_conn_from_one_request(connection* conn){
    // 重置connection的request_pool
    mem_pool_reset(conn->request_pool);
    // 重置connection的read_buffer
    reset_packet_buffer(conn->read_buffer);
    // 重置connection的write_buffer
    reset_packet_buffer(conn->write_buffer);
    // 重置connection的读取属性
    conn->packet_id = 0;
    conn->packet_length = 0;
    conn->header_read_len = 0;
    conn->body_read_len = 0;
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
        // ptr 其实是connection
        ((connection*)ptr)->epfd = epfd;
        event.data.ptr = ptr; 
    }
    return epoll_ctl(epfd,EPOLL_CTL_ADD,epifd,&event);
}

int poll_mod_event(int epfd,int epifd,int mask,void* ptr){
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
    return epoll_ctl(epfd,EPOLL_CTL_MOD,epifd,&event);
}

int enable_conn_write_and_disable_read(connection* conn){
    if(CONN_WRITING == conn->reading_or_writing){
        return TRUE;
    }
    printf("enable_conn_write_and_disable_read\n");
    if(-1 == poll_mod_event(conn->epfd,conn->sockfd,EPOLLOUT,conn)){
        return FALSE;
    }
    conn->reading_or_writing = CONN_WRITING;
    return TRUE;
}

int enable_conn_read_and_disable_write(connection* conn){
    if(CONN_READING == conn->reading_or_writing){
        return TRUE;
    }
    printf("enable_conn_read_and_disable_write\n");
    if(-1 == poll_mod_event(conn->epfd,conn->sockfd,EPOLLIN,conn)){
        return FALSE;
    }
    conn->reading_or_writing = CONN_READING;
    return TRUE;    
}

int is_need_lock(connection* conn){
    if(IS_BACK_CONN == conn->is_front_or_back){
        if(TRUE == conn->back->selecting){
            return TRUE;
        }else{
            return FALSE;
        }
    }else{
        front_conn* front = conn->front;
        if(NULL == front->back){
            printf("front->back NULL\n");
            return FALSE;
        }
        if(FALSE == front->back->selecting){
            printf("front->back selecting FALSE\n");
            return FALSE;
        }
        // 只有在处理结果集的时候才会有并发现象
        // backend才可能并发往front里面写
        return TRUE;
    }
}