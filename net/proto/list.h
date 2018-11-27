#ifndef hero_list_h

// 参照linux中的list方案

struct list_head{
    struct list_head *prev,*next;
};

#ifndef container_of
// 指向对应偏移
#define container_of(ptr, type, member) \
    (type *)((char *)(ptr) - (char *) &((type *)0)->member)
#endif

#define LIST_HEAD_INIT(name) { &(name), &(name) }

// 这样list_head的prev和next就指向了&name,&name两个地址本身
#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->prev = list;
	list->next = list;
	list->next = list;
}    

static inline void __list_add(struct list_head *newObj,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = newObj;
	newObj->next = next;
	newObj->prev = prev;
	prev->next = newObj;
}

// 链表的添加 inline的必须为static,不然内联函数会有冲突问题
// 内联函数仅仅包含文件的内部可见
static inline void list_add(struct list_head *newObj,struct list_head *head)
{
    __list_add(newObj, head, head->next);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

// 链表的删除
static inline void list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

// list正向遍历
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

// list反向遍历
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#endif