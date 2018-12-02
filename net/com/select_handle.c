#include "show_handle.h"
#include "../config.h"
#include "../server_parse.h"
#include "../proto/packet.h"
#include "../network.h"
#include "../proto/packet_const.h"

#define OTHER -1
#define VERSION_COMMENT 1
#define DATABASE 2
#define USER 3
#define LAST_INSERT_ID 4
#define IDENTITY 5
#define VERSION 6
#define TX_ISOLATION 7
#define AUTO_INCREMENT 8


int select_parse_sql(char* sql,int offset,int length);

int verion_comment_check(char* sql,int offset,int length);

int select_2_check(char* sql,int offset,int length);

int write_version_comment(int sockfd,mem_pool* pool);

int handle_select(int sockfd,char* sql,int offset,mem_pool* pool){
    int type = select_parse_sql(sql,offset,strlen(sql));
    switch(type){
        case VERSION_COMMENT:
            printf("it's version_comment\n");
            return write_version_comment(sockfd,pool);
        default:
            return TRUE;   
    }
    return TRUE;
}


int select_parse_sql(char* sql,int offset,int length){
    int i = offset;
    for(;i<length;i++){
        switch(sql[i]){
            case ' ':
            case '\t':
                continue;
            case '/':
            case '#':
                // i = parse comment
                return OTHER;
            case '@':
                return select_2_check(sql,i,length);
            default:
                return OTHER;
        }
    }
    return OTHER;    
}

int select_2_check(char* sql,int offset,int length){
    if(length > ++offset && sql[offset] == '@'){
        if(length > ++offset){
            printf("%c\n",sql[offset]);
            switch(sql[offset]){
                case 'V':
                case 'v':
                    return verion_comment_check(sql,offset,length);
                case 'I':
                case 'i':
                    // identify check
                    return OTHER;    
                default:
                    return OTHER;
            }
        }
    }
    return OTHER;
}

int verion_comment_check(char* sql,int offset,int length){
     if(length > offset + strlen("ersion_comment")){
        char c1 = sql[++offset];
        char c2 = sql[++offset];
        char c3 = sql[++offset];
        char c4 = sql[++offset];
        char c5 = sql[++offset];
        char c6 = sql[++offset];
        char c7 = sql[++offset];
        char c8 = sql[++offset];  
        char c9 = sql[++offset];
        char c10 = sql[++offset];
        char c11 = sql[++offset];
        char c12 = sql[++offset];
        char c13 = sql[++offset];
        char c14 = sql[++offset]; 
        // for eof
        char c15 = sql[++offset];       

		if((c1 == 'e' || c2 =='E') && 
		   (c2 == 'r' || c3 =='R') && 
		   (c3 == 's' || c4 =='S') && 
		   (c4 == 'i' || c5 =='I') && 
		   (c5 == 'o' || c6 =='O') && 
		   (c6 == 'n' || c7 =='N') && 
		   (c7 == '_' || c8 =='_') && 
		   (c8 == 'c' || c9 =='C') &&
 		   (c9 == 'o' || c10 =='O') && 
		   (c10 == 'm' || c11 =='M') && 
		   (c11 == 'm' || c12 =='M') && 
		   (c12 == 'e' || c13 =='E') && 
		   (c13 == 'n' || c14 =='N') && 
		   (c14 == 't' || c15 =='T') &&          
		   (c15 == '\0' || c15 == ' ' || c15 == '\n' || c15 == '\r' || c15 == '\n' || c15==';')
		) {
			return VERSION_COMMENT;
		}
    }
    return OTHER;   
}


int write_version_comment(int sockfd,mem_pool* pool){
    // 一开始申请一块512字节内存
    packet_buffer* pb = get_packet_buffer(DEFAULT_PB_SIZE);
    if(pb == NULL){
        return NULL;
    }
    int packet_id = 1;
    // 发送result set header
    result_set_header* header = get_result_set_header(pool);     
    header->field_count = 1;
    header->extra = 0;
    int size = caculate_result_set_header_size(header);
    header->header.packet_length = size;
    header->header.packet_id = packet_id++;
    // 参照kernel的错误处理方法
    if(!write_result_set_header(pb,header)){
        goto error_process;
    }
    // 发送fields
    field_packet* field = get_field_packet(pool);
    if(field == NULL){
        goto error_process;
    }
    field->name = "@@version_comment";
    field->header.packet_length = caculate_field_size(field);
    field->header.packet_id = packet_id++;
    if(!write_field(pb,field)){
        goto error_process;
    }
    // 发送 eof
    eof_packet* eof = get_eof_packet(pool);
    if(field == NULL){
        goto error_process;
    }
    eof->header.packet_length = caculate_eof_size();
    eof->header.packet_id = packet_id++;
    if(!write_eof(pb,eof)){
        goto error_process;
    }
    // 发送 row
    row_packet* row = get_row_packet(pool);
    row->field_count=1;
    add_field_value_to_row(pool,row,SERVER_VERSION,strlen(SERVER_VERSION));
    row->header.packet_length = caculate_row_size(row);
    row->header.packet_id = packet_id++;
    if(!write_row(pb,row)){
        goto error_process;
    }
    // 发送last eof,只需要修改下packet_id
    eof->header.packet_id = packet_id++;
    if(!write_eof(pb,eof)){
        goto error_process;
    }
    writen(sockfd,pb->pos,pb->buffer);
    free_packet_buffer(pb);
    return TRUE;
error_process:
    free_packet_buffer(pb);
    return FALSE;    
}