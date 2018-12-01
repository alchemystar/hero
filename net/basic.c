#include "basic.h"
#include <stdio.h>

// 64位 8字节对齐
#define HERO_ALIGN_SIZE 8

mem_pool* mem_pool_create(int size){
    // 至少分配512字节的空间，防止碎片太多
    if(size < DEFAULT_MEM_POOL_SIZE){
        size = DEFAULT_MEM_POOL_SIZE;
    }
    int mem_size = 1;
    // 向上2的幂次取整
    while (mem_size < size){
        mem_size <<= 1;
    }
    mem_pool* pool = mem_alloc(sizeof(mem_pool));
    if(pool == NULL){
        printf("mem exhausted");
        return NULL;       
    }
    // 用malloc的原因是少走向上取整这一步
    unsigned char* buffer = malloc(mem_size);
    if(buffer == NULL){
        printf("mem exhausted");
        return NULL;
    }
    pool->buffer = buffer;
    pool->length = mem_size;
    pool->pos = 0;
    pool->next = NULL;
    return pool;
}

void* mem_pool_alloc(int size,mem_pool* pool){
    // 对齐size为8字节倍数
    int mod = size%HERO_ALIGN_SIZE;
    if(mod != 0){
        size = size + HERO_ALIGN_SIZE - size%HERO_ALIGN_SIZE;
    }
    mem_pool* entry;
    // 记录最后一个last_entry,即->next=NULL的那个
    mem_pool* last_entry;
    for(entry=pool,last_entry=pool ;entry != NULL;entry = entry->next){
        // 遍历搜索内存块中有足够buffer的块
        if(entry->pos + size > entry->length){
            last_entry = entry;
            continue;
        }else{
             // 有,直接返回
             void* ptr = entry->buffer + entry->pos;
             entry->pos +=size;
             return ptr;
        }
    }
    // 到此处说明需要分配新的内存
    mem_pool* mem_pool = mem_pool_create(size);
    if(mem_pool == NULL) {
        // 内存耗尽
        return NULL;
    }else{
        // 当前last_entry已经指向了最后一个mem_pool
        // 将新创建的链接上去
        last_entry->next = mem_pool;
        void* ptr = mem_pool->buffer + mem_pool->pos;
        mem_pool->pos += size;
        return ptr;
    }
}

void mem_pool_free(mem_pool* pool){
    mem_pool* entry = pool;
    mem_pool* to_free = pool;
    while(entry != NULL){
        to_free = entry;
        entry = entry->next;
        free(to_free->buffer);
        free(to_free);
    }
}

// for 预留内存池实现,返回是指针类型
void* mem_alloc(int size){
    // 这边不做内存对齐的原因是malloc内部已经内存对齐
    int mem_size = 1;
    // 向上2的幂次取整
    while (mem_size < size){
      mem_size <<= 1;
    }
    return malloc(size);
}
// for 预留内存池实现
void mem_free(void* addr){
    free(addr);
}
