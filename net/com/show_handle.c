#include "show_handle.h"
#include "../config.h"
#include "../server_parse.h"
#include "../proto/packet.h"
#include "handle_util.h"
#include "../datasource.h"

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

int show_collate_check(char* sql,int offset,int length);

int show_database_check(char* sql,int offset,int length);

int show_tables_check(char* sql,int offset,int length);

int write_databases(front_conn* front);

int  write_tables(front_conn* front);

int write_collation(front_conn* front);

int write_show_variables(front_conn* front);

int get_one_variable_row(mem_pool* pool,int packet_id,char* key,char* value);

int handle_show(front_conn* front,char* sql,int offset){
    int type = show_parse_sql(sql,offset,strlen(sql));
    switch(type){
        case DATABASES:
            // printf("it's show databases \n");
            return write_databases(front);
        case SHOWTABLES:
            // printf("it's show tables\n");
            return write_tables(front);
        case COLLATION:
            // printf("it's show collation\n");
            return write_collation(front);
        default:
            // todo 改掉 默认这边write_databases
            return default_execute(front,sql,TRUE);
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
            case 'C':
            case 'c':
                return show_collate_check(sql,offset,length);
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

int show_collate_check(char* sql,int offset,int length){
    if(length > offset + strlen("collation")){
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
		if((c1 == 'c' || c1 =='C') && 
		   (c2 == 'o' || c2 =='O') && 
		   (c3 == 'l' || c3 =='L') && 
		   (c4 == 'l' || c4 =='L') && 
		   (c5 == 'a' || c5 =='A') && 
		   (c6 == 't' || c6 =='T') && 
		   (c7 == 'i' || c7 =='I') && 
		   (c8 == 'o' || c8 =='O') && 
		   (c9 == 'n' || c9 =='N') &&
		   (is_char_eof(c10))
		) {
			return COLLATION;
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

int write_databases(front_conn* front){
    int sockfd = front->conn->sockfd;
    mem_pool* pool = front->conn->request_pool;
    packet_buffer* pb = get_conn_write_buffer(front->conn);
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
    write_nonblock(front->conn);
    return TRUE;
error_process:
    reset_packet_buffer(pb);
    return FALSE;    
}

int write_tables(front_conn* front){
    int sockfd = front->conn->sockfd;
    mem_pool* pool = front->conn->request_pool;
    packet_buffer* pb = get_conn_write_buffer(front->conn);
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
    write_nonblock(front->conn);
    return TRUE;
error_process:
    reset_packet_buffer(pb);
    return FALSE;    
}

// todo 压测的话，需要解决jdbc连接的问题！所以先搞background吧！
int write_show_variables(front_conn* front){
    int sockfd = front->conn->sockfd;
    mem_pool* pool = front->conn->request_pool;
    packet_buffer* pb = get_conn_write_buffer(front->conn);
    if(pb == NULL){
        return NULL;
    }
    int packet_id = 1;
    // 发送result set header
    result_set_header* header = get_result_set_header(pool);   
    if(header == NULL){
        return NULL;
    }      
    header->field_count = 2;
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
    field->name = "Variable_name";
    field->header.packet_length = caculate_field_size(field);
    field->header.packet_id = packet_id++;
    if(!write_field(pb,field)){
        goto error_process;
    }
    field->name = "Value";
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
    // row_packet* row = get_row_packet(pool);
    // row->field_count=1;
    // add_field_value_to_row(pool,row,"4096",strlen("4096"));
    // row->header.packet_length = caculate_row_size(row);
    // row->header.packet_id = packet_id++;
    // if(!write_row(pb,row)){
    //     goto error_process;
    // }
    row_packet* row = get_one_variable_row(pool,packet_id++,"character_set_client", "gb2312");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"character_set_connection", "gb2312");
     if(!write_row(pb,row)){
        goto error_process;
    }   
    row = get_one_variable_row(pool,packet_id++,"character_set_results", "gb2312");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"character_set_server", "latin1");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"init_connect", "");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"interactive_timeout", "28800");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"lower_case_table_names", "2");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"max_allowed_packet", "4194304");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"net_buffer_length", "16384");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"net_write_timeout", "60");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"query_cache_size", "1048576");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"query_cache_type", "OFF");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"sql_mode", "STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"system_time_zone", "CST");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"time_zone", "SYSTEM");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"tx_isolation", "REPEATABLE-READ");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_variable_row(pool,packet_id++,"wait_timeout", "28800");
    if(!write_row(pb,row)){
        goto error_process;
    }
        // 发送last eof,只需要修改下packet_id
    eof->header.packet_id = packet_id++;
    if(!write_eof(pb,eof)){
        goto error_process;
    }
    write_nonblock(front->conn);
    return TRUE;
error_process:
    reset_packet_buffer(pb);
    return FALSE;    
}
// collation
// 这边为了简单，全部以string代替，jdbc会自动转换
int write_collation(front_conn* front){
   int sockfd = front->conn->sockfd;
    mem_pool* pool = front->conn->request_pool;
    packet_buffer* pb = get_conn_write_buffer(front->conn);
    if(pb == NULL){
        return NULL;
    }
    int packet_id = 1;
    // 发送result set header
    result_set_header* header = get_result_set_header(pool);   
    if(header == NULL){
        return NULL;
    }      
    header->field_count = 6;
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
    field->name = "Collation";
    field->header.packet_length = caculate_field_size(field);
    field->header.packet_id = packet_id++;
    field->type=FIELD_TYPE_VAR_STRING;
    if(!write_field(pb,field)){
        goto error_process;
    }
    field->name = "Charset";
    field->header.packet_length = caculate_field_size(field);
    field->header.packet_id = packet_id++;
    field->type=FIELD_TYPE_VAR_STRING;
    if(!write_field(pb,field)){
        goto error_process;
    }
    field->name = "Id";
    field->header.packet_length = caculate_field_size(field);
    field->header.packet_id = packet_id++;
    field->type=FIELD_TYPE_LONGLONG;
    if(!write_field(pb,field)){
        goto error_process;
    }
    field->name = "Default";
    field->header.packet_length = caculate_field_size(field);
    field->header.packet_id = packet_id++;
    field->type=FIELD_TYPE_VAR_STRING;
    if(!write_field(pb,field)){
        goto error_process;
    }
    field->name = "Compiled";
    field->header.packet_length = caculate_field_size(field);
    field->header.packet_id = packet_id++;
    field->type=FIELD_TYPE_VAR_STRING;
    if(!write_field(pb,field)){
        goto error_process;
    }
    field->name = "Sortlen";
    field->header.packet_length = caculate_field_size(field);
    field->header.packet_id = packet_id++;
    field->type=FIELD_TYPE_VAR_STRING;
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
    row_packet* row = get_one_collation_row(pool,packet_id++,"gb2312_chinese_ci", "gb2312","28");
    if(!write_row(pb,row)){
        goto error_process;
    }
    row = get_one_collation_row(pool,packet_id++,"utf8_general_ci", "utf8","33");
     if(!write_row(pb,row)){
        goto error_process;
    }  
    // 发送last eof,只需要修改下packet_id
    eof->header.packet_id = packet_id++;
    if(!write_eof(pb,eof)){
        goto error_process;
    }
    write_nonblock(front->conn);
    return TRUE;
error_process:
    reset_packet_buffer(pb);
    return FALSE;         
}

int get_one_variable_row(mem_pool* pool,int packet_id,char* key,char* value){
    row_packet* row = get_row_packet(pool);
    row->field_count=2;
    add_field_value_to_row(pool,row,key,strlen(key));
    add_field_value_to_row(pool,row,value,strlen(value));
    row->header.packet_length = caculate_row_size(row);
    row->header.packet_id = packet_id;
    return row;
}

int get_one_collation_row(mem_pool* pool,int packet_id,char* key,char* value,char* index){
    row_packet* row = get_row_packet(pool);
    row->field_count=6;
    add_field_value_to_row(pool,row,key,strlen(key));
    add_field_value_to_row(pool,row,value,strlen(value));
    add_field_value_to_row(pool,row,index,strlen(index));
    add_field_value_to_row(pool,row,"YES",strlen("YES")); 
    add_field_value_to_row(pool,row,"YES",strlen("YES")); 
    add_field_value_to_row(pool,row,"1",strlen("1"));   
    row->header.packet_length = caculate_row_size(row);
    row->header.packet_id = packet_id;
    return row;    
}