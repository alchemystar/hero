#ifndef HERO_BASIC_H
#define HERO_BASIC_H
#define DEFAULT_MEM_POOL_SIZE 512

// 简易内存池实现
typedef struct _mem_pool{
    int pos;
    int length;
    // 这边必须是unsigned char的buffer,原因是这样才以字节进行移动
    unsigned char* buffer;
    // 指向下一个mem_pool,假设内存不够的话
    void* next;
}mem_pool;

// 必须以2的整数倍分配
mem_pool* mem_pool_create(int size);
void* mem_pool_alloc(int size,mem_pool* pool);
void mem_pool_free(mem_pool* pool);

void* mem_alloc(int size);
void mem_free(void* address);

#endif