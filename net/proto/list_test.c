#include "list.h"


typedef struct _item{
    int n;
    struct list_head list;
}item,*item_ptr;

void testListHead(){
    item_ptr itemHeader = malloc(sizeof(item));
    itemHeader->n=1;
    // 所有对list的操作都是按list_head为
    INIT_LIST_HEAD(&(itemHeader->list));
    for(int i=0 ; i < 10 ;i++) {
        item_ptr item2ptr = malloc(sizeof(item));
        item2ptr->n=i;
        list_add(&(item2ptr->list),&(itemHeader->list));
    }
    struct list_head* pos;
    item_ptr tmp;
    struct list_head* head = &itemHeader->list;
    list_for_each_prev(pos,head) {
        tmp = list_entry(pos,struct _item,list);
        printf("%d\n",tmp->n);
    }
}


int main(int argc,char* argv[]){
    testListHead();
}