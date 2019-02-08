#include <sys/socket.h>
#include <stdlib.h>
#include "proto/capacity.h"
#include "network.h"
#include "config.h"
#include "proto/packet_const.h"
#include "buffer_util.h"
#include <sys/errno.h>
#include <string.h>
#include "password.h"
#include "query.h"
#include "conn.h"

unsigned char FILLER_HAND_SHAKE[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

unsigned char AUTH_OKAY[] = {7, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0};

unsigned char FILLER[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


int handle_one_connection(int sockfd,struct sockaddr_in* sockaddr,mem_pool* pool){
    // 定义front conn
    front_conn* front = (front_conn*) mem_pool_alloc(sizeof(front_conn),pool);
    front->conn = (connection*) mem_pool_alloc(sizeof(connection),pool);
    init_conn(front->conn,sockfd,sockaddr,pool);
    // 发送hand_shake包
    hand_shake_packet* handshake;
    if(!send_handshake(front,&handshake)){
        return FALSE;
    }
    // 接收auth包
    auth_packet* auth;
    if(!read_auth(front,&auth)){
        return FALSE;
    }
    // 校验权限
    if(!check_auth(auth,handshake)){
        printf("check auth failed\n");
        // send error packet;
        return FALSE;
    }
    printf("check auth okay\n");
    front->auth_state = NOT_SEND_HANDSHAKE;
    front->schema = auth->database;
    front->user = auth->user;
    front->conn->charset_index = auth->charset_index;
    send_auth_okay(sockfd);
    while(TRUE){
        if(!handle_mysql_command(front)){
            break;
        }
    }
    free_front_conn(front);
    return TRUE;
}

int write_handshake_to_buffer(connection* conn){
    int sockfd = conn->sockfd;
    mem_pool* pool = conn->meta_pool;
    // 因为要保留，所以在池中分配
    hand_shake_packet* hand_shake  = get_handshake_packet(pool);
    conn->front->hand_shake_packet = hand_shake;
    if(hand_shake == NULL){
        return FALSE;
    }
    // 在write_buffer中分配，最后在write完的时候，write_buffer会重置
    packet_buffer* pb = get_conn_write_buffer_with_size(conn,caculate_handshake_size()+MYSQL_HEADER_LEN);
    if(pb == NULL){
        return FALSE;
    }
    // 写入 hand_shake buffer
    int handshake_size = caculate_handshake_size();
    // 由于这边申请的是确定的buffer size,所以下面应该不会进行expand=>无需进行内存分配检查
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
    return TRUE;
}

// here for handshake todo conn buffer
int send_handshake(front_conn* front,hand_shake_packet** result){
    int sockfd = front->conn->sockfd;
    mem_pool* pool = front->conn->meta_pool;
    hand_shake_packet* hand_shake  = get_handshake_packet(pool);
    if(hand_shake == NULL){
        return FALSE;
    }
    // 不在内存池中分配，所以需要手动释放内存
    packet_buffer* pb = get_conn_write_buffer_with_size(front->conn,caculate_handshake_size()+MYSQL_HEADER_LEN);
    if(pb == NULL){
        return FALSE;
    }
    // 写入 hand_shake buffer
    int handshake_size = caculate_handshake_size();
    // 由于这边申请的是确定的buffer size,所以下面应该不会进行expand=>无需进行内存分配检查
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
    writen(sockfd,pb->pos,pb->buffer);
    // release 对应的buffer
    reset_packet_buffer(pb);
    *result = hand_shake;
    return TRUE;
}

// 用完packet之后得重置buffer
// todo 用random读取很少的一段数据 来测试一下粘包问题！！！！！！！！
// 包装一些recv ^_^
int read_packet(connection* conn){
    int sockfd = conn->sockfd;
    int header_read_len = conn->header_read_len;
    int nread = 0;
    // 待读缓冲区
    unsigned char* read_buffer = conn->read_buffer->buffer + conn->read_buffer->read_limit;
    if(header_read_len < MYSQL_HEADER_LEN){
        nread = recv(sockfd,read_buffer,MYSQL_HEADER_LEN - header_read_len,0);
        if(nread < 0) {
            if(errno == EINTR || errno == EWOULDBLOCK){
                return READ_WAIT_FOR_EVENT;
            }
        }else{
            // read_buffer,read_limit往前移动
            read_buffer += nread;
            conn->read_buffer->read_limit += nread;
            header_read_len += nread;
        }
        // 再次检验是否达到4字节
        if(header_read_len < MYSQL_HEADER_LEN){
            return READ_WAIT_FOR_EVENT;
        }
        // 记录当前packet_id
        // 到此处已经达到了4字节
        // 开始读取包长度
        conn->packet_length = read_ub3(conn->read_buffer);
        printf("packet_length = %d\n",conn->packet_length);
        if(conn->packet_length == -1){
            return READ_PACKET_ERROR;
        }
        // 读取packet_id
        conn->packet_id = read_byte(conn->read_buffer);
        printf("packet_id = %d\n",conn->packet_id);
        if(conn->packet_id == -1){
            return READ_PACKET_ERROR;
        }
        if(conn->packet_length > MYSQL_PACKET_MAX_LENGTH){
            printf("packet_length more than max , packet_length=%d\n",conn->packet_length);
            return READ_PACKET_LENGTH_MORE_THAN_MAX;
        }
        // 如果包长度过大，则expand read_buffer,todo 需要考虑connection的buffer收敛
        int all_size = conn->packet_length + MYSQL_HEADER_LEN;
        if(all_size> conn->read_buffer->length){
            if(FALSE == expand(conn->read_buffer,all_size)){
                return MEMORY_EXHAUSTED;
            }
        }
    }
    // 读取body,由于上面expand了，所以这边不需要检查buffer大小
    if(conn->body_read_len < conn->packet_length ) {
        nread = recv(sockfd,read_buffer,conn->packet_length - conn->body_read_len,0);
        if(nread < 0) {
            if(errno == EINTR || errno == EWOULDBLOCK){
                return READ_WAIT_FOR_EVENT;
            }
        }else{
            // read_buffer,read_limit往前移动
            read_buffer += nread;
            conn->read_buffer->read_limit += nread;
            conn->body_read_len += nread;
        }    
        if(conn->body_read_len == conn->packet_length){
            return TRUE;
        }else{
            return READ_WAIT_FOR_EVENT;
        }   
    }else {
        return TRUE;
    }
}

int read_and_check_auth(connection* conn){
    packet_buffer* pb = conn->read_buffer;
    // 栈上变量分配
    auth_packet auth;
    // 由于buffer是之前分配好的，这边理论上不会出错
    auth.header.packet_length = conn->packet_length;
    auth.header.packet_id = conn->packet_id;
    auth.client_flags = read_ub4(pb);
    auth.max_packet_size = read_ub4(pb);
    auth.charset_index = read_byte(pb); 
    // 当前pos，如果遇到filler,则跳过这么多偏移
    int current = pb->pos;
    int len = read_length(pb);
    if(len >0 && len < FILLER_SIZE){
        unsigned char* extra = (unsigned char*)mem_pool_alloc(len,conn->request_pool);
        // 到position的位置copy
        memcpy(extra,pb->buffer+current,len);   
        auth.extra = extra; 
    } else{
        // NULL for mem free
        auth.extra = NULL;
    }
    // skip filler length
    pb->pos = current + FILLER_SIZE;
    // read user,user用meta_pool
    auth.user = read_string_with_null(pb,conn->meta_pool);
    // password不从meta_pool中获取，因为不存储此信息
    printf("auth user name=%s\n",auth.user);
    auth.password = read_bytes_with_length(pb,conn->request_pool,&(auth.password_length));
    printf("auth password length = %d\n",auth.password_length);
    if(auth.password == NULL){
        return FALSE;
    }
    int remaining = packet_has_read_remaining(pb);
    if(((auth.client_flags & CLIENT_CONNECT_WITH_DB)) !=0 && (remaining > 0)){
        auth.database = read_string_with_null(pb,conn->meta_pool);
    }else{
        auth.database = NULL;
    }
    if(FALSE == check_auth(&auth,conn->front->hand_shake_packet)){
        printf("check pass error\n");
        // 而后connection close后回收内存
        return FALSE;
    }
    printf("check auth okay\n");
    // 到这边认证成功
    conn->front->user = auth.user;
    conn->front->database = auth.database;
    // 留待最后清理
    return TRUE;     
} 


// here for auth
// auth也用元信息内存池
int read_auth(front_conn* front,auth_packet** result){
    int sockfd = front->conn->sockfd;
    mem_pool* pool = front->conn->meta_pool;    
    unsigned char* header_buffer = front->conn->header;
    // 读取4个字节(MySQL Header Len)
    if(!readn(sockfd,MYSQL_HEADER_LEN,header_buffer)){
        printf("read handshake error");
        return NULL;
    }
    // 读取长度
    int length = read_packet_length(header_buffer);
    if(length > MAX_FRAME_SIZE){
        printf("handshake size more than max size,length=%d",length);     
        return NULL;
    }
    // 读取packet_id
    int packet_id = header_buffer[PACKET_ID_POS];
    // 创建auth_buffer
    packet_buffer* pb = get_conn_read_buffer_with_size(front->conn,length);
    if(pb == NULL){
        return FALSE;
    }
    // 读取整个body
    if(!readn(sockfd,length,pb->buffer)){
        reset_packet_buffer(pb);
        return FALSE;
    }
    // 设置pb read limit
    set_packet_buffer_read_limit(pb,length);
    // 构建对应的auth结构体
    auth_packet* auth = (auth_packet*)mem_pool_alloc(sizeof(auth_packet),pool);
    auth->header.packet_length = length;
    auth->header.packet_id = packet_id;
    auth->client_flags = read_ub4(pb);
    auth->max_packet_size = read_ub4(pb);
    auth->charset_index = read_byte(pb);
    // 当前pos，如果遇到filler,则跳过这么多偏移
    int current = pb->pos;
    int len = read_length(pb);
    if(len >0 && len < FILLER_SIZE){
        unsigned char* extra = (unsigned char*)mem_pool_alloc(len,pool);
        // 到position的位置copy
        memcpy(extra,pb->buffer+current,len);   
        auth->extra = extra; 
    } else{
        // NULL for mem free
        auth->extra = NULL;
    }
    // skip filler length
    pb->pos = current + FILLER_SIZE;
    // read user
    auth->user = read_string_with_null(pb,pool);
    auth->password = read_bytes_with_length(pb,pool,&(auth->password_length));
    int remaining = packet_has_read_remaining(pb);
    if(((auth->client_flags & CLIENT_CONNECT_WITH_DB)) !=0 && (remaining > 0)){
        auth->database = read_string_with_null(pb,pool);
    }else{
        auth->database = NULL;
    }
    // 清理packet_buffer
    reset_packet_buffer(pb);
    *result = auth;
    return TRUE;
}

int check_auth(auth_packet* auth,hand_shake_packet* handshake){
    printf("user:%s:login\n" , auth->user);
    if(auth->password_length != SCRAMBLE_PASSWORD_LEN){
        printf("auth packet password_length not 20");
        return FALSE;
    }
    // 直接用栈上变量,无需申请内存
    unsigned char scramble_buffer[20];
    scramble(scramble_buffer,handshake->scramble,PASS_WORD);
    // 摘要认证
    for(int i=0 ; i < SCRAMBLE_PASSWORD_LEN ; i++){
        if(scramble_buffer[i] != auth->password[i]){
            return FALSE;
        }
    }
    return TRUE;
}

void send_auth_okay(int sockfd){
    writen(sockfd,AUTH_OKAY_SIZE,AUTH_OKAY);
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

