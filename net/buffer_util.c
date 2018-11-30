#include "buffer_util.h"
#include <string.h>
#include "basic.h"

int read_byte(packet_buffer* pb){
    unsigned char* buffer = pb->buffer + pb->pos;
    if(pb->pos + 1 > pb->length ){
        printf("error write_bytes more than max len");
        return -1;
    }
    int i = buffer[0];
    pb->pos +=1;
    return i;    
}

int read_ub2(packet_buffer* pb){
    unsigned char* buffer = pb->buffer + pb->pos;
    if(pb->pos + 2 > pb->length ){
        printf("error write_bytes more than max len");
        return -1;
    }
    int i = buffer[0] | (buffer[1] << 8);
    pb->pos +=2;
    return i;
}
int read_ub3(packet_buffer* pb){
    unsigned char* buffer = pb->buffer + pb->pos;
    if(pb->pos + 3 > pb->length){
        printf("error write_bytes more than max len");
        return -1;
    }    
    int i = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16);
    pb->pos +=3;
    return i;
}

int read_packet_length(unsigned char* buffer){
    return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16);
}

long read_ub4(packet_buffer* pb){
    unsigned char* buffer = pb->buffer + pb->pos;
    if(pb->pos + 4 > pb->length ){
        printf("error write_bytes more than max len");
        return -1;
    }    
    int i = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    pb->pos +=4;
    return i;
}
long read_long(packet_buffer* pb){
    unsigned char* buffer = pb->buffer + pb->pos;
    if(pb->pos + 8 > pb->length ){
        printf("error write_bytes more than max len");
        return -1;
    }    
    long l = buffer[0];
    l = l | buffer[1] << 8;
    l = l | buffer[2] << 16;
    l = l | buffer[3] << 24;
    l = l | buffer[4] << 32;
    l = l | buffer[5] << 40;
    l = l | buffer[6] << 48;
    l = l | buffer[7] << 56;
    pb->pos += 8;
    return l;
}
// notice 防止数组越界
long read_length(packet_buffer* pb){
    unsigned char* buffer = pb->buffer + pb->pos;
     if(pb->pos + 1 > pb->length ){
        printf("error write_bytes more than max len");
        return -1;
    }   
    int length = buffer[0];
    pb->pos++;
    switch(length) {
        case 251:
            return -1;
        case 252:
            return read_ub2(buffer);
        case 253:
            return read_ub3(buffer);   
        case 254:
            return read_ub4(buffer);    
        default:
            return length;
    }
}

char* read_string_with_null(packet_buffer* pb){
    if(pb->pos >= pb->length){
        return NULL;
    }
    int offset = -1;
    for(int i=pb->pos;i<pb->length;i++){
        if(pb->buffer[i] == 0){
            offset = i;
            break;
        }
    }
    if(offset == -1){
      // 1 for char 0  
      int string_len = pb->length - pb->pos + 1;
      int content_len = string_len - 1;
      char* s = mem_alloc(string_len);
      memcpy(s,pb->buffer+pb->pos,content_len);
      s[content_len] = 0; 
      pb->pos = pb->length;  
      return s;
    }
    if(offset > pb->pos){
        // 1 for char 0  
        int string_len = offset - pb->pos + 1;
        int content_len = string_len - 1;
        char* s = mem_alloc(string_len);
        memcpy(s,pb->buffer+pb->pos,content_len);
        s[string_len]=0;
        pb->pos = offset+1;
        return s;
    }else{
        pb->pos++;
        return NULL;
    }
}

// 同时还返回字节数量
char* read_bytes_with_length(packet_buffer* pb,int* bytes_length){
    int length = read_length(pb);
    if(length <= 0 ){
        return NULL;
    }
    unsigned char* bytes = mem_alloc(length);
    *bytes_length = length;
    pb->pos +=length;
    return bytes;
}

void write_byte(packet_buffer* pb ,unsigned char c){
    if(pb->pos + 1 > pb->length ){
        printf("error write_bytes more than max len");
        return;
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    write_buff[0] = c;
    pb->pos++;
}

void write_UB2(packet_buffer* pb, int i){
    if(pb->pos + 2 > pb->length ){
        printf("error write_bytes more than max len");
        return;
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    write_buff[0] = (unsigned char)(i&0xff);
    write_buff[1] = (unsigned char)(i>>8);
    pb->pos += 2;
}

void write_UB3(packet_buffer* pb, int i){
    if(pb->pos + 3 > pb->length ){
        printf("error write_bytes more than max len");
        return;
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    write_buff[0] = (unsigned char)(i&0xff);
    write_buff[1] = (unsigned char)(i>>8);
    write_buff[2] = (unsigned char)(i>>16);
    pb->pos += 3;
}

void write_UB4(packet_buffer* pb,int i){
    if(pb->pos + 4 > pb->length ){
        printf("error write_bytes more than max len");
        return;
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    write_buff[0] = (unsigned char)(i&0xff);
    write_buff[1] = (unsigned char)(i>>8);
    write_buff[2] = (unsigned char)(i>>16);
    write_buff[3] = (unsigned char)(i>>24);
    pb->pos += 4;
}

void write_long(packet_buffer* pb,long l){
    if(pb->pos + 8 > pb->length ){
        printf("error write_bytes more than max len");
        return;
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    write_buff[0] = (unsigned char)(l&0xff);
    write_buff[1] = (unsigned char)(l>8);
    write_buff[2] = (unsigned char)(l>>16);
    write_buff[3] = (unsigned char)(l>>24);
    write_buff[4] = (unsigned char)(l>32);
    write_buff[5] = (unsigned char)(l>>40);
    write_buff[6] = (unsigned char)(l>>48);
    write_buff[7] = (unsigned char)(l>>56);
    pb->pos +=8;
}

void write_length(packet_buffer* pb,long l){
     unsigned char* write_buff = pb->buffer + pb->pos;
    if(l < 251){
        write_byte(pb,l);
        return;
    }else if(l < 0x10000L){
        write_byte(pb,253);
        write_UB3(pb,l);
        return;
    }else{
        write_byte(pb,254);
        write_long(pb,l);
        return;
    }
}

void write_bytes(packet_buffer* pb ,unsigned char* src ,int length){
    if(pb->pos + length > pb->length ){
        printf("error write_bytes more than max len");
        return;
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    memcpy(write_buff,src,length);
    // for(int i=0 ; i < length ;i++){
    //     char a = src[i];
    //     write_byte(pb,src[i]);
    // }
    pb->pos += length;
}

void write_with_null(packet_buffer* pb , unsigned char* src , int length){

    write_bytes(pb,src,length);
    write_byte(pb,0);
}


packet_buffer* get_packet_buffer(int size){
    unsigned char* buff = mem_alloc(size);
    int size_packet_buffer = sizeof(packet_buffer);
    packet_buffer* pb = (packet_buffer*)mem_alloc(sizeof(packet_buffer));
    pb->pos = 0 ;
    pb->length = size;
    pb->buffer = buff;
    return pb;
}

void free_packet_buffer(packet_buffer* pb){
    mem_free(pb->buffer);
    mem_free(pb);
}

int packet_has_remaining(packet_buffer* pb){
    return pb->length - pb->pos;
}