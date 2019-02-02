#ifndef HERO_WORKER
#define HERO_WORKER
#include "basic.h"

int create_and_start_rw_thread(int worker_fd,mem_pool* pool);

void hanlde_ready_connection(connection* conn);

#endif