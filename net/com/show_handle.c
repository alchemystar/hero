#include "show_handle.h"
#include "../config.h"
#include "../server_parse.h"
#include "../proto/packet.h"
#include "../network.h"
#include "handle_util.h"

#define OTHER -1
#define DATABASES 1
#define DATASOURCES 2
#define COLLATION 3
#define FULL_TABLES 4
#define FULL_COLUMNS 5
#define KEYS 6
#define VARIABLES 7
#define SHOWTABLES 8
#define SHOW_CREATE_TABLE 9

#define DEFAULT_DATABASE "hero";
#define SHOW_DATA_BASE_FIELDS_LENGTH 1


int show_parse_sql(char* sql,int offset,int length);

int show_database_check(char* sql,int offset,int length);

int show_tables_check(char* sql,int offset,int length);

int write_databases(int sockfd,mem_pool* pool);

int  write_tables(int sockfd,mem_pool* pool);

int handle_show(int sockfd,char* sql,int offset,mem_pool* pool){
    int type = show_parse_sql(sql,offset,strlen(sql));
    switch(type){
        case DATABASES:
            printf("it's show databases \n");
            return write_databases(sockfd,pool);
        case SHOWTABLES:
            printf("it's show tables\n");
            return write_tables(sockfd,pool);
        default:
            return TRUE;   
    }
    return TRUE;
}

int show_parse_sql(char* sql,int offset,int length){
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
            case 'D':
            case 'd':
                return show_database_check(sql,offset,length);
            case 't':
            case 'T':
                return show_tables_check(sql,offset,length);
            default:
                return OTHER;
        }
    }
    return OTHER;
}

int show_database_check(char* sql,int offset,int length){
    if(length > offset + strlen("databases")){
        char c1 = sql[++offset];
        char c2 = sql[++offset];
        char c3 = sql[++offset];
        char c4 = sql[++offset];
        char c5 = sql[++offset];
        char c6 = sql[++offset];
        char c7 = sql[++offset];
        char c8 = sql[++offset];  
        char c9 = sql[++offset];
        // for eof
        char c10 = sql[++offset];
		if((c1 == 'd' || c1 =='D') && 
		   (c2 == 'a' || c2 =='A') && 
		   (c3 == 't' || c3 =='T') && 
		   (c4 == 'a' || c4 =='A') && 
		   (c5 == 'b' || c5 =='B') && 
		   (c6 == 'a' || c6 =='A') && 
		   (c7 == 's' || c7 =='S') && 
		   (c8 == 'e' || c8 =='E') && 
		   (c9 == 's' || c9 =='S') &&
		   (is_char_eof(c10))
		) {
			return DATABASES;
		}
    }
    return OTHER;
}

int show_tables_check(char* sql,int offset,int length){
    if(length > offset + strlen("tables")){
        char c1 = sql[++offset];
        char c2 = sql[++offset];
        char c3 = sql[++offset];
        char c4 = sql[++offset];
        char c5 = sql[++offset];
        char c6 = sql[++offset];
        // for eof
        char c7 = sql[++offset];
		if((c1 == 't' || c1 =='T') && 
		   (c2 == 'a' || c2 =='A') && 
		   (c3 == 'b' || c3 =='B') && 
		   (c4 == 'l' || c4 =='L') && 
		   (c5 == 'e' || c5 =='E') && 
		   (c6 == 's' || c6 =='S') && 
		   (is_char_eof(c7))
		) {
			return SHOWTABLES;
		}
    }
    return OTHER;
}

int write_databases(int sockfd,mem_pool* pool){
    // 一开始申请一块512字节内存
    packet_buffer* pb = get_packet_buffer(DEFAULT_PB_SIZE);
    if(pb == NULL){
        return NULL;
    }
    int packet_id = 1;
    // 发送result set header
    result_set_header* header = get_result_set_header(pool);   
    if(header == NULL){
        return NULL;
    }      
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
    field->name = "DATABASE";
    field->header.packet_length = caculate_field_size(field);
    field->header.packet_id = packet_id++;
    if(!write_field(pb,field)){
        goto error_process;
    }
    // 发送 eof
    eof_packet* eof = get_eof_packet(pool);
    if(eof == NULL){
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
    add_field_value_to_row(pool,row,"hero",strlen("hero"));
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

int write_tables(int sockfd,mem_pool* pool){
    // 一开始申请一块512字节内存
    // todo 8 for debug
    packet_buffer* pb = get_packet_buffer(DEFAULT_PB_SIZE);
    if(pb == NULL){
        return NULL;
    }
    int packet_id = 1;
    // 发送result set header
    result_set_header* header = get_result_set_header(pool); 
    if(header == NULL){
        return NULL;
    }    
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
    field->name = "TABLES";
    field->header.packet_length = caculate_field_size(field);
    field->header.packet_id = packet_id++;
    if(!write_field(pb,field)){
        goto error_process;
    } 
    // 发送 eof
    eof_packet* eof = get_eof_packet(pool);  
    if(eof == NULL){
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
    add_field_value_to_row(pool,row,"hero_table",strlen("hero_table"));
    
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