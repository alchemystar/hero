#include "query.h"
#include "config.h"
#include "buffer_util.h"
#include "server_parse.h"
#include "com/show_handle.h"
#include "com/select_handle.h"
#include "network.h"
#include "sql_error_code.h"

int handle_mysql_command(front_conn* front){
    unsigned char* header_buffer = front->conn->header;
    int sockfd = front->conn->sockfd;
    mem_pool* pool = front->conn->request_pool;
    // 读取4个字节(MySQL Header Len)
    if(!readn(sockfd,MYSQL_HEADER_LEN,header_buffer)){
        printf("read handshake error");
        return FALSE;
    }
    // 读取长度
    int length = read_packet_length(header_buffer);
    printf("length = %d\n",length);
    if(length > MAX_FRAME_SIZE){
        printf("handshake size more than max size,length=%d",length);     
        return FALSE;
    }
    // 读取packet_id
    front->conn->packet_id = header_buffer[PACKET_ID_POS];
    // 使用front conn的buffer
    packet_buffer* pb = get_conn_read_buffer_with_size(front->conn,length);
    if( pb == NULL) {
        return FALSE;
    }
    // 读取整个body
    if(!readn(sockfd,length,pb->buffer)){
        reset_packet_buffer(pb);
        return FALSE;
    }
    // 设置read_limit
    set_packet_buffer_read_limit(pb,length);
    // 首字节为type
    int type = read_byte(pb);
    int result = FALSE;
    switch(type){
        case COM_QUERY:
            result = handle_com_query(front);
            break;
        default:
            result = write_unkown_error_message(front);
            break;
    }
    // 重置packet_buffer   
    reset_packet_buffer(pb);
    return result;
}

int handle_com_query(front_conn* front){
    char* sql = read_string(front->conn->read_buffer,front->conn->request_pool);
    int rs = server_parse_sql(sql);
    switch(rs & 0xff){
        case SHOW:
            return handle_show(front,sql,rs >> 8);
        case SELECT:
            printf("it's select\n");
            return handle_select(front,sql,rs >> 8);    
        case KILL_QUERY:
            printf("it's kill\n");
            // todo kill backend的连接
            // return false,上层关闭连接
            return FALSE;       
        default:
            printf("default return okay");
            return write_okay(front->conn);
    }
    return TRUE;
}

// todo default error packet 不再n申请内存
int write_unkown_error_message(front_conn* front){
    int sockfd = front->conn->sockfd;
    mem_pool* pool = front->conn->request_pool;
    printf("unknown command\n");
    error_packet* error = get_error_packet(pool);
    error->message = "Unknown command";
    error->header.packet_length=caculate_error_packet_size(error);
    error->header.packet_id = front->conn->packet_id+1;
    error->error = ER_UNKNOWN_COM_ERROR;
    packet_buffer* pb = get_conn_write_buffer(front->conn);
    if(pb == NULL){
        return NULL;
    }
    if(!write_error(pb,error)){
        goto error_process;
    }
    if(!writen(sockfd,pb->pos,pb->buffer)){
        goto error_process;
    }
    reset_packet_buffer(pb);
    return TRUE;
error_process:
    reset_packet_buffer(pb);
    return FALSE;
}
