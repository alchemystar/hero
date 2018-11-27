#include <stdio.h>
#include "list.h"

#ifndef HERO_PACKET_H
#define HERO_PACKET_H

#define FIELD_COUNT 0xfe
#define SQL_STATE_MARKER '#'

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
    unsigned char* sql_state;
    unsigned char* message;
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
    long threadId;
    unsigned char* seed;
    int server_capabilities;
    unsigned char server_charset_index;
    int server_status;
    unsigned char* rest_of_scramble_buff;
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
    // struct _field_value* next;
    unsigned char* value;
    struct list_head list;
}field_value;

typedef struct _row_data{
    mysql_packet header;
    int field_count;
    // 通用链表设计，对应的value链表
    field_value value_list;
}row_data;

// 代表了整个result结构体
typedef struct _result_set{
    result_set_header* header;
    struct list_head fields;
    eof_packet* eof_packet;
    row_data data;
    eof_packet* last_eof;
}result_set;

#endif