#ifndef HERO_BUFFER_UTIL_H
#define HERO_BUFFER_UTIL_H
#include "basic.h"

typedef struct _packet_buffer{
    int pos;
    int read_limit; // for read,write则无此限制
    int length;
    unsigned char* buffer;
}packet_buffer;

// 当前所有函数都是以buffer开头进行读取

int read_ub2(packet_buffer* pb);
int read_ub3(packet_buffer* pb);
int read_packet_length(unsigned char* buffer);
long read_ub4(packet_buffer* pb);
long read_long(packet_buffer* pb);
long read_length(packet_buffer* pb);
char* read_string(packet_buffer* pb,mem_pool* pool);
char* read_string_with_null(packet_buffer* pb,mem_pool* pool);
char* read_bytes_with_length(packet_buffer* pb,mem_pool* pool,int* bytes_length);

int write_byte(packet_buffer* pb ,unsigned char c);
int write_UB2(packet_buffer* pb,int i);
int write_UB3(packet_buffer* pb, int i);
int write_UB4(packet_buffer* pb,int i);
int write_long(packet_buffer* pb,long l);
int write_length(packet_buffer* pb,long l);
int write_bytes(packet_buffer* pb ,unsigned char* src ,int length);
int write_with_null(packet_buffer* pb , unsigned char* src , int length);
int write_with_length(packet_buffer* pb,unsigned char* src,int length);
int write_string_with_length_or_null(packet_buffer* pb ,char* src );

packet_buffer* get_packet_buffer(int size);

int get_length(long length);

int get_length_with_bytes(long length);

void free_packet_buffer(packet_buffer* pb);   

int packet_has_read_remaining(packet_buffer* pb);

int expand(packet_buffer* pb,int size);

#endif