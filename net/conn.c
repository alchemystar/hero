#include "conn.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "basic.h"

atomic_int front_conn_id = 0 ;
atomic_int back_conn_id = 0;

int init_conn(connection* conn,int sockfd,struct sockaddr_in* addr,mem_pool* pool) {
    conn->sockfd = sockfd;
    conn->addr = addr;
    conn->pool = pool;
    socklen_t sock_len = (socklen_t)sizeof(*addr);
    int sock_ret = getsockname(sockfd,addr,&sock_len);
    if(sock_ret != 0){
        printf("get sock name error,sock_ret=%d",sock_ret);
        goto error_process;
    }
    // Each call to inet_ntoa() in your application will override this area
    // so copy it
    char* ip = (char*)inet_ntoa((*addr).sin_addr);
    if(ip != NULL){
        snprintf((char*)conn->ip,48,"%s",ip);
    }else{
        conn->ip[0] = "\0";
    }
    conn->port = (*addr).sin_port;
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
    conn->query_count = 0;
    return TRUE;
error_process:
    if(close(sockfd) != 0){
        printf("close sockfd error,sockfd=%d",sockfd);
    }
    free_packet_buffer(conn->read_buffer);
    free_packet_buffer(conn->write_buffer);
    return FALSE;

}

// todo 在适当时机调整一下read_buffer,不至于过大
packet_buffer* get_conn_read_buffer(connection* conn,int size){
    packet_buffer* pb = conn->read_buffer;
    if(size > pb->length) {
        if(!expand(pb,size)){
            return NULL;
        }
    }
    return pb;
}


void free_front_conn(front_conn* front){
    free_packet_buffer(front->conn.read_buffer);
    free_packet_buffer(front->conn.write_buffer);
    // front_conn本身由mem_pool free
}
