#ifndef HERO_WORKER_H
#define HERO_WORKER_H
#include "basic.h"
#include "conn.h"

int create_and_start_rw_thread(int worker_fd,mem_pool* pool);

void handle_ready_read_connection(connection* conn);

void hanlde_ready_write_connection(connection* conn);

int write_nonblock(connection* conn);

int write_auth_okay(connection* conn);

int write_okay(connection* conn);

#endif