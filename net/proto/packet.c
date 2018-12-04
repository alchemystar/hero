#include "packet.h"
#include "packet_const.h"
#include <string.h>
#include "../network.h"
#include "../basic.h"
#include "../buffer_util.h"
#include <stdatomic.h>
#include <stdlib.h>
#include "../config.h"
#include "string.h"

// for vscode 
#define TRUE 1
#define FALSE 0
#define EOF_PACKET_SIZE 5

atomic_int g_client_id=0;

unsigned char FIELD_FILLER[2] = {0,0};

unsigned char* random_seed(int size,mem_pool* pool);

void packet(){
    printf("this is packet\n");
}

int caculate_handshake_size(){
    int size = 1;
    size += strlen(SERVER_VERSION);
    size +=5; // 1+4;
    size += 8; // for seed 
    size += 19; // 1+2+1+2+13
    size += 12; // for rest of scrambleBuff
    size +=1;
    return size;
}

hand_shake_packet* get_handshake_packet(mem_pool* pool){
    hand_shake_packet* hand_shake = (hand_shake_packet*)mem_pool_alloc(sizeof(hand_shake_packet),pool);
    if(hand_shake == NULL){
        return NULL;
    }
    hand_shake->header.packet_id=0;
    hand_shake->protol_version=10;
    hand_shake->server_version=SERVER_VERSION;
    hand_shake->thread_id=g_client_id++;
    hand_shake->server_capabilities=get_server_capacities();
    hand_shake->server_charset_index = UTF8_CHAR_INDEX;
    hand_shake->server_status=2;
    // mem_pool内部分配的buffer无需手动释放
    unsigned char* scramble_buffer = random_seed(20,pool);
    if(scramble_buffer == NULL){
        return NULL;
    }
    hand_shake->seed= scramble_buffer;
    hand_shake->rest_of_scramble_buff= scramble_buffer+8;
    hand_shake->scramble = scramble_buffer;
    return hand_shake;
}

ok_packet* get_ok_packet(mem_pool* pool){
    ok_packet* okay = (ok_packet*)mem_pool_alloc(sizeof(ok_packet),pool);
    if(okay == NULL){
        return NULL;
    }
    okay->affected_rows = 0;
    // todo;
    return okay;
}

result_set_header* get_result_set_header(mem_pool* pool){
    result_set_header* header = (result_set_header*)mem_pool_alloc(sizeof(result_set_header),pool);
    if(header == NULL){
        return NULL;
    }
    header->header.packet_length = 0;
    header->header.packet_id = 0;
    header->field_count = 0;
    header->extra = 0;
    return header;
}

unsigned char* random_seed(int size,mem_pool* pool){
    unsigned char* buffer = (unsigned char*)mem_pool_alloc(size,pool);
    if(buffer == NULL){
        return NULL;
    }
    for(int i=0 ;i < size ;i++){
      //  buffer[i] = rand()%256;
      // for debug
        buffer[i] = 1;
    }
    return buffer;
}

field_packet* get_field_packet(mem_pool* pool){
    field_packet* field = (field_packet*)mem_pool_alloc(sizeof(field_packet),pool);
    if(field == NULL){
        return NULL;
    }
	field->catalog="def";
	field->db=NULL;
	field->table=NULL;
	field->org_table=NULL;
	field->name=NULL;
	field->org_name=NULL;
    field->charset_index=0;
    field->length=0;
    field->type=0;
    field->flags=0;
    field->decimals=0;
    field->definition=NULL;
    return field;
}


eof_packet* get_eof_packet(mem_pool* pool){
    eof_packet* eof = (eof_packet*)mem_pool_alloc(sizeof(eof_packet),pool);
    if(eof == NULL){
        return NULL;
    }
    eof->header.packet_length=0;
    eof->header.packet_id=0;
    eof->field_count=(0xfe);
    eof->warning_count=0;
    eof->status=2;
    return eof;
}

row_packet* get_row_packet(mem_pool* pool){
    row_packet* row = (row_packet*)mem_pool_alloc(sizeof(row_packet),pool);
    if(row == NULL){
        return NULL;
    }
    row->header.packet_length=0;
    row->header.packet_id=0; 
    row->field_count = 0;
    row->value_list = NULL;
    return row;
}

error_packet* get_error_packet(mem_pool* pool){
    error_packet* error = (error_packet*)mem_pool_alloc(sizeof(error_packet),pool);
    if(error == NULL){
        return NULL;
    } 
    error->header.packet_length=0;
    error->header.packet_id = 0;
	error->field_count=0xff;
	error->error=0;
	error->mark='#';
	error->sql_state="HY000";
	error->message=NULL;
    return error;
}

packet_buffer* get_handshake_buff(){
    // 还需要加上头长度
    return get_packet_buffer(caculate_handshake_size()+MYSQL_HEADER_LEN);
}

int caculate_result_set_header_size(result_set_header* ptr){
    int size = get_length(ptr->field_count);
    if(ptr->extra > 0){
        size += get_length(ptr->extra);
    }
    return size;
}

int caculate_field_size(field_packet* ptr){
        int size = (ptr->catalog == NULL ? 1 : get_length_with_bytes(strlen(ptr->catalog)));
        size += (ptr->db == NULL ? 1 : get_length_with_bytes(strlen(ptr->db)));
        size += (ptr->table == NULL ? 1 : get_length_with_bytes(strlen(ptr->table)));
        size += (ptr->org_table == NULL ? 1 : get_length_with_bytes(strlen(ptr->org_table)));
        size += (ptr->name == NULL ? 1 : get_length_with_bytes(strlen(ptr->name)));
        size += (ptr->org_name == NULL ? 1 : get_length_with_bytes(strlen(ptr->org_name)));
        size += 13;// 1+2+4+1+2+1+2
        if (ptr->definition != NULL) {
            size += get_length_with_bytes(strlen(ptr->definition));
        }
        return size;
}

int caculate_eof_size(){
    return EOF_PACKET_SIZE;
}

int caculate_row_size(row_packet* row){
    int size = 0;
    field_value* entry = row->value_list;
    while(entry != NULL){
        size += get_length_with_bytes(entry->length);
        entry = entry->next;
    }
    return size;
}

int caculate_error_packet_size(error_packet* error){
    int size = 9; // 1+2+1+5
    if(error->message != NULL){
        size += strlen(error->message);
    }   
    return size;
}

int write_result_set_header(packet_buffer*pb,result_set_header* header){
    if(!write_UB3(pb,header->header.packet_length)){
        return FALSE;
    }
    if(!write_byte(pb,header->header.packet_id)){
        return FALSE;
    }
    if(!write_length(pb,header->field_count)){
        return FALSE;
    }
    if(header->extra > 0){
        if(!write_length(pb,header->extra)){
            return FALSE;     
        }
    }
    return TRUE;
}

int write_field(packet_buffer* pb,field_packet* field){
    // buffer到上层释放
    if(!write_UB3(pb,field->header.packet_length)){
        return FALSE;
    }
    if(!write_byte(pb,field->header.packet_id)){
        return FALSE;
    }
    if(!write_string_with_length_or_null(pb, field->catalog)){
        return FALSE;
    }
    if(!write_string_with_length_or_null(pb, field->db)){
        return FALSE;
    }
    if(!write_string_with_length_or_null(pb, field->table)){
        return FALSE;
    }
    if(!write_string_with_length_or_null(pb, field->org_table)){
        return FALSE;
    }
    if(!write_string_with_length_or_null(pb, field->name)){
        return FALSE;
    }
    if(!write_string_with_length_or_null(pb, field->org_name)){
        return FALSE;
    }
    if(!write_byte(pb,(unsigned char) 0x0C)){
        return FALSE;
    }
    if(!write_UB2(pb, field->charset_index)){
        return FALSE;
    }
    if(!write_UB4(pb, field->length)){
        return FALSE;
    }
    if(!write_byte(pb,(unsigned char) (field->type & 0xff))){
        return FALSE;
    }
    if(!write_UB2(pb, field->flags)){
        return FALSE;
    }
    if(!write_byte(pb,field->decimals)){
        return FALSE;
    }
    if(!write_bytes(pb,FIELD_FILLER,2)){
        return FALSE;
    }
    if (field->definition != NULL) {
        if(!write_with_length(pb, field->definition,strlen(field->definition))){
            return FALSE;
        }
    }    
    return TRUE;
}

int write_eof(packet_buffer* pb,eof_packet* eof){
    if(!write_UB3(pb,eof->header.packet_length)){
        return FALSE;
    }
    if(!write_byte(pb,eof->header.packet_id)){
        return FALSE;
    }
    if(!write_byte(pb,eof->field_count)){
        return FALSE;
    }
    if(!write_UB2(pb,eof->warning_count)){
        return FALSE;
    }
    if(!write_UB2(pb,eof->status)){
        return FALSE;
    }
    return TRUE;
}

int write_row(packet_buffer* pb,row_packet* row){
    if(!write_UB3(pb,row->header.packet_length)){
        return FALSE;
    }
    if(!write_byte(pb,row->header.packet_id)){
        return FALSE;
    }
    field_value* entry = row->value_list;
    while(entry != NULL){
        if(entry->length && entry->value != NULL){
            if(!write_length(pb,entry->length)){
                return FALSE;
            }
            if(!write_bytes(pb,entry->value,entry->length)){
                return FALSE;
            }
        }else{
            if(!write_byte(pb,251)){
                return FALSE;
            }
        }
        entry = entry->next;
    }
    return TRUE;
}

int write_error(packet_buffer* pb,error_packet* error){
    if(!write_UB3(pb,error->header.packet_length)){
        return FALSE;
    }
    if(!write_byte(pb,error->header.packet_id)){
        return FALSE;
    }
    if(!write_byte(pb,error->field_count)){
        return FALSE;
    }
    if(!write_UB2(pb,error->error)){
        return FALSE;
    }
    if(!write_byte(pb,error->mark)){
        return FALSE;
    }
    if(!write_bytes(pb,error->sql_state,strlen(error->sql_state))){
        return FALSE;
    }
    if(error->message != NULL){
        if(!write_bytes(pb,error->message,strlen(error->message))){
            return FALSE;
        }
    }
    return TRUE;
}

field_value* get_field_value(mem_pool* pool,unsigned char* buffer,int length){
    field_value* value = (field_value*)mem_pool_alloc(sizeof(field_value),pool);
    if(value == NULL){
        return NULL;
    }
    value->value = buffer;
    value->length = length;
    value->next = NULL;
    return value;   
}

int add_field_value_to_row(mem_pool* pool,row_packet* row,unsigned char* buffer,int length){
    field_value* new_value = get_field_value(pool,buffer,length);
    if(new_value == NULL){
        return FALSE;
    }
    if(row->value_list == NULL){
        row->value_list = new_value;
        return TRUE;
    }
    field_value* prev = row->value_list;
    // 找到链入的前一个
    while(prev->next != NULL){
        prev = prev->next;
    }
    prev->next = new_value;
    return TRUE;
    
}