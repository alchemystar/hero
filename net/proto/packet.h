#ifndef HERO_PACKET_H
#define HERO_PACKET_H

#include <stdio.h>
#include "list.h"
#include "../buffer_util.h"
#define FIELD_COUNT 0xfe
#define SQL_STATE_MARKER '#'

#define MYSQL_HEADER_LEN 4
#define MYSQL_HEADER_END_POS 3
#define PACKET_ID_POS 3
#define UTF8_CHAR_INDEX 83
#define SCRAMBLE_PASSWORD_LEN 20

#define COM_QUIT 1
#define COM_INIT_DB  2
#define COM_QUERY 3

#define DEFAULT_PB_SIZE 512

// 定义to_string的函数指针
typedef char*(*to_string)();
// 定义读取内存的操作
typedef void(*read)(unsigned char* data);

// todo以后考虑对齐(align)问题
// 这边typedef就可以不用写成struct mysql_packet的形式
typedef struct {
    // 3 字节
    int packet_length;
    // 1 字节
    char packet_id;
}mysql_packet;

typedef struct _auth_packet{
    // mysql通用头部
    mysql_packet header;
    // client标识
    long client_flags;
    // 最大packet大小
    long max_packet_size;
    // 对应字符集
    int charset_index;
    unsigned char* extra;
    char* user;
    unsigned char* password;
    // 表示password对应byte长度,不参与buffer的写入
    int password_length;
    char* database;
}auth_packet;

typedef struct _command_packet{
   // mysql通用头部
   mysql_packet header; 
   char* command;
   unsigned char* args;
}command_packet;

typedef struct _eof_packet{
    mysql_packet header;
    unsigned char field_count;
    unsigned char warning_count;
    unsigned char status;
}eof_packet;

typedef struct _error_packet{
    mysql_packet header;
    unsigned char field_count;
    int error;
    unsigned char mark;
    char* sql_state;
    char* message;
}error_packet;

typedef struct _field_packet{
   mysql_packet header; 
   unsigned char* catalog;
   unsigned char* db;
   unsigned char* table;
   unsigned char* org_table;
   unsigned char* name;
   unsigned char* org_name;
   int charset_index;
   long length;
   int type;
   int flags;
   unsigned char decimals;
   unsigned char* definition;
}field_packet;

typedef struct _hand_shake_packet{
    mysql_packet header;
    unsigned char protol_version;
    unsigned char* server_version;
    long thread_id;
    unsigned char* seed;
    int server_capabilities;
    unsigned char server_charset_index;
    int server_status;
    unsigned char* rest_of_scramble_buff;
    // 用作auth解密用，不参与网络传输
    unsigned char* scramble;
}hand_shake_packet;

typedef struct  _ok_packet{
    mysql_packet header;
    unsigned char field_count;
    long affected_rows;
    long insert_id;
    int server_status;
    int warning_count;
    unsigned char* message;
}ok_packet;

typedef struct _result_set_header{
    mysql_packet header;
    int field_count;
    long extra;
}result_set_header;

typedef struct _field_value{
    unsigned char* value;
    int length;
    void *next;
}field_value;

typedef struct _row_packet{
    mysql_packet header;
    int field_count;
    // 通用链表设计，对应的value链表
    field_value* value_list;
}row_packet;

// 代表了整个result结构体
typedef struct _result_set{
    result_set_header* header;
    struct list_head fields;
    eof_packet* eof_packet;
    row_packet* data;
    eof_packet* last_eof;
}result_set;


hand_shake_packet* get_handshake_packet(mem_pool* pool);
ok_packet* get_ok_packet(mem_pool* pool);
result_set_header* get_result_set_header(mem_pool* pool);
field_packet* get_field_packet(mem_pool* pool);
eof_packet* get_eof_packet(mem_pool* pool);
row_packet* get_row_packet(mem_pool* pool);
error_packet* get_error_packet(mem_pool* pool);

int add_field_value_to_row(mem_pool* pool,row_packet* row,unsigned char* buffer,int length);

int caculate_handshake_size();
int caculate_result_set_header_size(result_set_header* ptr);
int caculate_field_size(field_packet* ptr);
int caculate_eof_size();
int caculate_row_size(row_packet* row);
int caculate_error_packet_size(error_packet* error);

void free_packet_buffer(packet_buffer* pb);

int write_result_set_header(packet_buffer*pb,result_set_header* header);
int write_field(packet_buffer* pb,field_packet* field);
int write_eof(packet_buffer* pb,eof_packet* eof);
int write_row(packet_buffer* pb,row_packet* row);
int write_error(packet_buffer* pb,error_packet* error);

#endif