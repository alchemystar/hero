#ifndef HERO_BUFFER_UTIL_H
#define HERO_BUFFER_UTIL_H

typedef struct _packet_buffer{
    int pos;
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
char* read_string_with_null(packet_buffer* pb);
char* read_bytes_with_length(packet_buffer* pb,int* bytes_length);

void write_byte(packet_buffer* pb ,unsigned char c);
void write_UB2(packet_buffer* pb,int i);
void write_UB3(packet_buffer* pb, int i);
void write_UB4(packet_buffer* pb,int i);
void write_long(packet_buffer* pb,long l);
void write_length(packet_buffer* pb,long l);
void write_bytes(packet_buffer* pb ,unsigned char* src ,int length);
void write_with_null(packet_buffer* pb , unsigned char* src , int length);

packet_buffer* get_packet_buffer(int size);

void free_packet_buffer(packet_buffer* pb);   

int packet_has_remaining(packet_buffer* pb);

#endif