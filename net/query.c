#include "query.h"
#include "config.h"
#include "buffer_util.h"
#include "server_parse.h"
#include "com/show_handle.h"
#include "com/select_handle.h"
#include "network.h"
#include "sql_error_code.h"

int handle_com_query(int sockfd,packet_buffer* pb , mem_pool* pool);

int write_unkown_error_message(int sockfd,mem_pool* pool);

int handle_command(int sockfd,mem_pool* pool){
    unsigned char header_buffer[4];
    // 读取4个字节(MySQL Header Len)
    if(!readn(sockfd,MYSQL_HEADER_LEN,header_buffer)){
        printf("read handshake error");
        return FALSE;
    }
    // 读取长度
    int length = read_packet_length(header_buffer);
    if(length > MAX_FRAME_SIZE){
        printf("handshake size more than max size,length=%d",length);     
        return FALSE;
    }
    // 读取packet_id
    int packet_id = header_buffer[PACKET_ID_POS];
    // 创建整个包的buffer
    packet_buffer* pb = get_packet_buffer(length);
    if(pb == NULL){
        return FALSE;
    }
    // 读取整个body
    readn(sockfd,length,pb->buffer);
    // 首字节为type
    int type = read_byte(pb);
    switch(type){
        case COM_QUERY:
            return handle_com_query(sockfd,pb,pool);
        default:
            return write_unkown_error_message(sockfd,pool);
    }
    // 清理packet buffer
    free_packet_buffer(pb);
    return TRUE;
}

int handle_com_query(int sockfd,packet_buffer* pb , mem_pool* pool){
    char* sql = read_string(pb,pool);
    int rs = server_parse_sql(sql);
    switch(rs & 0xff){
        case SHOW:
            return handle_show(sockfd,sql,rs >> 8,pool);
        case SELECT:
            printf("it's select\n");
            return handle_select(sockfd,sql,rs >> 8,pool);    
        default:
            break;    
    }
    return TRUE;
}

// todo default error packet 不再n申请内存
int write_unkown_error_message(int sockfd,mem_pool* pool){
    printf("unknown command\n");
    error_packet* error = get_error_packet(pool);
    error->message = "Unknown command";
    error->header.packet_length=caculate_error_packet_size(error);
    error->header.packet_id = 1;
    error->error = ER_UNKNOWN_COM_ERROR;
    packet_buffer* pb = get_packet_buffer(error->header.packet_length + MYSQL_HEADER_LEN);
    if(pb == NULL){
        return NULL;
    }
    if(!write_error(pb,error)){
        goto error_process;
    }
    if(!writen(sockfd,pb->pos,pb->buffer)){
        goto error_process;
    }
    free_packet_buffer(pb);
    return TRUE;
error_process:
    free_packet_buffer(pb);
    return FALSE;
}
