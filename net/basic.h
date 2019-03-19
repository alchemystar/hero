#ifndef HERO_BASIC_H
#define HERO_BASIC_H
#include <sys/time.h>
#include <stdio.h>
#include <sys/errno.h>
#include <unistd.h>
#include "config.h"

#define DEFAULT_MEM_POOL_SIZE 4096
#define HERO_DEBUG
// 是否使用epoll进行编译
#define HERO_USE_EPOLL
#define TRUE 1
#define FALSE 0

// 简易内存池实现
typedef struct _mem_pool{
    int pos;
    int length;
    // 这边必须是unsigned char的buffer,原因是这样才以字节进行移动
    unsigned char* buffer;
    // 指向下一个mem_pool,假设内存不够的话
    void* next;
}mem_pool;

// for 对齐
// 采用<<c interface and implemention>>的实现
union hero_align{
    int i;
    long l;
    long *lp;
    void *p;
    void (*fp)(void);
    float f;
    double d;
    long double ld;
};

// 必须以2的整数倍分配
mem_pool* mem_pool_create(int size);
void* mem_pool_alloc(int size,mem_pool* pool);
void* mem_pool_alloc_ignore_check(int size,mem_pool* pool);
void mem_pool_free(mem_pool* pool);


void* mem_alloc(int size);
void mem_free(void* address);
void* mem_realloc(void* ptr ,int size);
void init_signal_handlers();
void mem_pool_reset(mem_pool* pool);

#endif