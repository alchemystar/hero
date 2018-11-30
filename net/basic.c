#include "basic.h"

// for 预留内存池实现,返回是指针类型
void* mem_alloc(int size){
    return malloc(size);
}
// for 预留内存池实现
void mem_free(void* addr){
    free(addr);
}