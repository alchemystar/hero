#include "query.h"
#include "config.h"
#include "buffer_util.h"

int handle_com_query(int sockfd,packet_buffer* pb , mem_pool* pool);

int handle_command(int sockfd,mem_pool* pool){
    unsigned char header_buffer[4];
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
    // 创建整个包的buffer
    packet_buffer* pb = get_packet_buffer(length);
    // 读取整个body
    readn(sockfd,length,pb->buffer);
    // 首字节为type
    int type = read_byte(pb);
    switch(type){
        case COM_QUERY:
            handle_com_query(sockfd,pb,pool);
            break;
        default:
            break;
    }
    // 清理packet buffer
    free_packet_buffer(pb);
    return TRUE;
}

int handle_com_query(int sockfd,packet_buffer* pb , mem_pool* pool){
    char* sql = read_string(pb,pool);

}