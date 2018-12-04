#include "buffer_util.h"
#include <string.h>
#include "config.h"
#include <stdio.h>
#include "basic.h"

// todo buffer_util read的error改造
int read_byte(packet_buffer* pb){
    unsigned char* buffer = pb->buffer + pb->pos;
    if(pb->pos + 1 > pb->read_limit ){
        printf("error write_bytes more than max len");
        return -1;
    }
    int i = buffer[0];
    pb->pos +=1;
    return i;    
}

int read_ub2(packet_buffer* pb){
    unsigned char* buffer = pb->buffer + pb->pos;
    if(pb->pos + 2 > pb->read_limit ){
        printf("error write_bytes more than max len");
        return -1;
    }
    int i = buffer[0] | (buffer[1] << 8);
    pb->pos +=2;
    return i;
}
int read_ub3(packet_buffer* pb){
    unsigned char* buffer = pb->buffer + pb->pos;
    if(pb->pos + 3 > pb->read_limit){
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
    if(pb->pos + 4 > pb->read_limit ){
        printf("error write_bytes more than max len");
        return -1;
    }    
    int i = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    pb->pos +=4;
    return i;
}
long read_long(packet_buffer* pb){
    unsigned char* buffer = pb->buffer + pb->pos;
    if(pb->pos + 8 > pb->read_limit ){
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
     if(pb->pos + 1 > pb->read_limit ){
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

 char* read_string(packet_buffer* pb,mem_pool* pool){
    if(pb->pos >= pb->read_limit){
        return NULL;
    }  
    int string_len = pb->read_limit - pb->pos + 1;
    int content_len = string_len - 1;
    char* s = (char*)mem_pool_alloc(string_len,pool);
    memcpy(s,pb->buffer+pb->pos,content_len);
    s[content_len] = 0; 
    pb->pos = pb->read_limit;
    return s;

}

char* read_string_with_null(packet_buffer* pb,mem_pool* pool){
    if(pb->pos >= pb->read_limit){
        return NULL;
    }
    int offset = -1;
    for(int i=pb->pos;i<pb->read_limit;i++){
        if(pb->buffer[i] == 0){
            offset = i;
            break;
        }
    }
    if(offset == -1){
      // 1 for char 0  
      int string_len = pb->read_limit - pb->pos + 1;
      int content_len = string_len - 1;
      char* s = (char*)mem_pool_alloc(string_len,pool);
      memcpy(s,pb->buffer+pb->pos,content_len);
      s[content_len] = 0; 
      pb->pos = pb->read_limit;  
      return s;
    }
    if(offset > pb->pos){
        // 1 for char 0  
        int string_len = offset - pb->pos + 1;
        int content_len = string_len - 1;
        char* s = (char*)mem_pool_alloc(string_len,pool);
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
char* read_bytes_with_length(packet_buffer* pb,mem_pool* pool,int* bytes_length){
    int length = read_length(pb);
    if(length <= 0 ){
        return NULL;
    }
    if(pb->pos + length >= pb->read_limit){
        return NULL;
    }   
    unsigned char* bytes = (unsigned char*)mem_pool_alloc(length,pool);
    *bytes_length = length;
    memcpy(bytes,pb->buffer+pb->pos,length);
    pb->pos +=length;
    return bytes;
}

int write_byte(packet_buffer* pb ,unsigned char c){
    if(pb->pos + 1 > pb->length ){
       if(!expand(pb,1)){
          return FALSE;
       }
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    write_buff[0] = c;
    pb->pos++;
    return TRUE;
}

int write_UB2(packet_buffer* pb, int i){
    if(pb->pos + 2 > pb->length ){
        if(!expand(pb,2)){
           return FALSE;
        }
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    write_buff[0] = (unsigned char)(i&0xff);
    write_buff[1] = (unsigned char)(i>>8);
    pb->pos += 2;
    return TRUE;
}

int write_UB3(packet_buffer* pb, int i){
    if(pb->pos + 3 > pb->length ){
        if(!expand(pb,3)){
           return FALSE;
        }
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    write_buff[0] = (unsigned char)(i&0xff);
    write_buff[1] = (unsigned char)(i>>8);
    write_buff[2] = (unsigned char)(i>>16);
    pb->pos += 3;
    return TRUE;
}

int write_UB4(packet_buffer* pb,int i){
    if(pb->pos + 4 > pb->length ){
        if(!expand(pb,4)){
           return FALSE;
        }
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    write_buff[0] = (unsigned char)(i&0xff);
    write_buff[1] = (unsigned char)(i>>8);
    write_buff[2] = (unsigned char)(i>>16);
    write_buff[3] = (unsigned char)(i>>24);
    pb->pos += 4;
    return TRUE;
}

int write_long(packet_buffer* pb,long l){
    if(pb->pos + 8 > pb->length ){
        if(!expand(pb,8)){
           return FALSE;
        }
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
    return TRUE;
}

int write_length(packet_buffer* pb,long l){
    unsigned char* write_buff = pb->buffer + pb->pos;
    if(l < 251){
        return write_byte(pb,l);
    }else if(l < 0x10000L){
        if(!write_byte(pb,253)){
            return FALSE;
        }
        return write_UB3(pb,l);
    }else{
        if(!write_byte(pb,254)){
            return FALSE;
        }
        return write_long(pb,l);
    }
}

int write_bytes(packet_buffer* pb ,unsigned char* src ,int length){
    if(pb->pos + length > pb->length ){
        if(!expand(pb,length)){
           return FALSE;
        }
    }
    unsigned char* write_buff = pb->buffer + pb->pos;
    memcpy(write_buff,src,length);
    pb->pos += length;
    return TRUE;
}

int write_with_null(packet_buffer* pb , unsigned char* src , int length){

    if(!write_bytes(pb,src,length)){
        return FALSE;
    }
    return write_byte(pb,0);
}

int write_with_length(packet_buffer* pb,unsigned char* src,int length){
    if(!write_length(pb,length)){
        return FALSE;
    }
    return write_bytes(pb,src,length);
}

int write_string_with_length_or_null(packet_buffer* pb ,char* src){
    if(src == NULL){
       return write_byte(pb,0);
    }else{
       return write_with_length(pb,src,strlen(src));
    }
}

int get_length(long length){
    if (length < 251) {
        return 1;
    } else if (length < 0x10000L) {
        return 3;
    } else if (length < 0x1000000L) {
        return 4;
    } else {
        return 9;
    }  
}

int get_length_with_bytes(long length){
     if (length < 251) {
        return 1+length;
    } else if (length < 0x10000L) {
        return 3+length;
    } else if (length < 0x1000000L) {
        return 4+length;
    } else {
        return 9+length;
    }     
}

packet_buffer* get_packet_buffer(int size){
    // for read string ,不然会读到多分配的buffer
    int read_limit = size;
    int actual_size = 1;
    while(actual_size < size){
        actual_size <<= 1;
    }
    // 校正为向上取整的buffer size
    size = actual_size;
    unsigned char* buff = (char*)mem_alloc(size);
    if(buff == NULL){
        return NULL;
    }
    packet_buffer* pb = (packet_buffer*)mem_alloc(sizeof(packet_buffer));
    if(pb == NULL){
        // 需要free掉之前申请的buff
        free(buff);
        return NULL;
    }
    pb->pos = 0 ;
    pb->length = size;
    pb->buffer = buff;
    pb->read_limit = read_limit;
    return pb;
}

// for debug
void printf_packet_buffer(packet_buffer* pb){
    for(int i=0 ; i< pb->pos ; i++){
        printf("%d ",pb->buffer[i]);
    }
    printf("\n");
}

void free_packet_buffer(packet_buffer* pb){
    mem_free(pb->buffer);
    mem_free(pb);
}

int packet_has_read_remaining(packet_buffer* pb){
    return pb->read_limit - pb->pos;
}

int expand(packet_buffer* pb,int size){
    // 向上2幂次取整
    int to_expand_size = pb->length + size;
    int new_size = 1;
    while(new_size < to_expand_size){
        new_size <<= 1;
    }
    void* p = mem_realloc(pb->buffer,new_size);
    if(p == NULL){
        printf("memory exhausted");
        return FALSE;
    } 
    pb->buffer = p;
    pb->length = new_size;
    return TRUE;
}