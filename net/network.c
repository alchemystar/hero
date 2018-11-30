#include <sys/socket.h>
#include <stdlib.h>
#include "proto/capacity.h"
#include "network.h"
#include "config.h"
#include "proto/packet_const.h"
#include "buffer_util.h"
#include <sys/errno.h>

#define MAX_FRAME_SIZE RECV_MAX_BUFFER_SIZE-MYSQL_HEADER_LEN

#define AUTH_OKAY_SIZE 11

#define FILLER_SIZE 23

unsigned char FILLER_HAND_SHAKE[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

unsigned char AUTH_OKAY[] = {7, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0};

unsigned char FILLER[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void send_handshake(int sockfd);

auth_packet* read_auth(int sockfd);

void send_auth_okay(int sockfd);

int check_auth(auth_packet* auth,hand_shake_packet* handshake);

int get_server_capacities();

void handle_one_connection(int sockfd){
    // 发送hand_shake包
    send_handshake(sockfd);
    // 当前读出来do nothing,直接返回okay
    auth_packet* auth=read_auth(sockfd);

    send_auth_okay(sockfd);
}

void send_handshake(int sockfd){
    hand_shake_packet* hand_shake  = get_handshake_packet();
    packet_buffer* pb = get_handshake_buff();
    // 写入 hand_shake buffer
    int handshake_size = caculate_handshake_size();
    write_UB3(pb,handshake_size);
    write_byte(pb,hand_shake->header.packet_id);
    write_byte(pb,hand_shake->protol_version);
    // 因为server_version是个字符串，最后一位为0,所以可以如此操作
    write_with_null(pb,hand_shake->server_version,strlen(hand_shake->server_version));
    write_UB4(pb,hand_shake->thread_id);
    // seed 固定8字节
    write_with_null(pb,hand_shake->seed,8);
    write_UB2(pb,hand_shake->server_capabilities);
    write_byte(pb,hand_shake->server_charset_index);
    write_UB2(pb,hand_shake->server_status);
    write_bytes(pb,FILLER_HAND_SHAKE,13);
    // rest_of_scramble_buff 固定12字节
    write_with_null(pb,hand_shake->rest_of_scramble_buff,12);
    // 写入socket具体的数据
    writen(sockfd,pb->length,pb->buffer);
    // todo release对应的hand_shake,放到session里面，不应该释放
    free_handshake_packet(hand_shake);
    // release 对应的buffer
    free_packet_buffer(pb);
}

void send_auth_okay(int sockfd){
    writen(sockfd,AUTH_OKAY_SIZE,AUTH_OKAY);
}

int check_auth(auth_packet* auth,hand_shake_packet* handshake){

}

// here for handshake
auth_packet* read_auth(int sockfd){
    unsigned char* header_buffer = mem_alloc(4);
    // 读取4个字节(MySQL Header Len)
    if(!readn(sockfd,MYSQL_HEADER_LEN,header_buffer)){
        printf("read handshake error");
        return;
    }
    // 读取长度
    int length = read_packet_length(header_buffer);
    if(length > MAX_FRAME_SIZE){
        printf("handshake size more than max size,length=%d",length);
        return;
    }
    // 读取packet_id
    int packet_id = header_buffer[PACKET_ID_POS];
    // 清理header buffer
    mem_free(header_buffer);
    // 创建auth_buffer
    packet_buffer* pb = get_packet_buffer(length);
    // 读取整个body
    readn(sockfd,length,pb->buffer);
    // 构建对应的auth结构体
    auth_packet* auth = mem_alloc(sizeof(auth_packet));
    auth->header.packet_length = length;
    auth->header.packet_id = packet_id;
    auth->client_flags = read_ub4(pb);
    auth->max_packet_size = read_ub4(pb);
    auth->charset_index = read_byte(pb);
    // 当前pos，如果遇到filler,则跳过这么多偏移
    int current = pb->pos;
    int len = read_length(pb);
    if(len >0 && len < FILLER_SIZE){
        unsigned char* extra = mem_alloc(len);
        // 到position的位置copy
        memcpy(extra,pb->buffer+current,len);   
        auth->extra = extra; 
    }
    // skip filler length
    pb->pos = current + FILLER_SIZE;
    // read user
    auth->user = read_string_with_null(pb);
    auth->password = read_bytes_with_length(pb,&(auth->password_length));
    int remaining = packet_has_remaining(pb);
    if(((auth->client_flags & CLIENT_CONNECT_WITH_DB)) !=0 && (remaining > 0)){
        auth->database = read_string_with_null(pb);
    }
    // 清理packet_buffer
    free_packet_buffer(pb);
    return auth;
}

// 返回读到的length
int readn(int sockfd,int size,unsigned char* recv_buff){
    size_t nleft=size;
    size_t nread=0;
    const unsigned char* ptr = recv_buff;
    while(nleft > 0){
    again:
        nread = recv(sockfd,ptr,nleft,0);
        // 处理被中断打断导致again的情况
        if(nread < 0) {
            if(errno == EINTR || errno == EWOULDBLOCK){
                goto again;
            }else{
                printf("read connection error");
                return FALSE;
            }
        }
        nleft-=nread;
        ptr +=nread;
    }
    return TRUE;
}


int writen(int sockfd,int size,unsigned char* write_buff){
    size_t nleft=size;
    ssize_t nwritten=0;
    unsigned char* ptr = write_buff;
    while(nleft > 0){
        if((nwritten = write(sockfd,ptr,nleft))<=0){
            if(nwritten <0 && (errno == EINTR || errno == EWOULDBLOCK)){
                nwritten = 0;
            }else{
                printf("write connection error");
                return FALSE;
            }
        }
        nleft -=nwritten;
        ptr +=nwritten;
    }
    return TRUE;
}

int get_server_capacities(){
    int flag = 0;
    flag |= CLIENT_LONG_PASSWORD;
    flag |= CLIENT_FOUND_ROWS;
    flag |= CLIENT_LONG_FLAG;
    flag |= CLIENT_CONNECT_WITH_DB;
    flag |= CLIENT_ODBC;
    flag |= CLIENT_IGNORE_SPACE;
    flag |= CLIENT_PROTOCOL_41;
    flag |= CLIENT_INTERACTIVE;
    flag |= CLIENT_IGNORE_SIGPIPE;
    flag |= CLIENT_TRANSACTIONS;
    flag |= CLIENT_SECURE_CONNECTION;
    return flag;
}

