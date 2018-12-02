#include "basic.h"
#include <stdio.h>

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

// todo 需要参照<<c interface and implement>> 将mem_pool自身内存也纳入其buffer中
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
    // 内存向上对齐
    // 数学推导过程大致如下
    // if size = k*n:
	//  (k *n + n -1)/n = k+(n-1)/n = > k
    // else if size = k*n+a && 1=<a<n
	//  (k*n+a + n - 1)/n = k + (a+n-1)/n
	//  1=<a<n
	//  n+(a-1)/n = 1 + 0=>k+1 => okay
    // size = ((size+sizeof(union hero_align)-1)/(sizeof(union hero_align))) * (sizeof(union hero_align));
    // 更进一步 (size/align) == size & ~(align - 1)=>下面的表达式
    size = (size + sizeof(union hero_align) - 1) & (~(sizeof(union hero_align) - 1));
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
        printf("Memory Exhausted");
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
    // 直接清0,calloc,防止野指针情况
    return calloc(1,size);
}
// for 预留内存池实现
void mem_free(void* addr){
    free(addr);
}
