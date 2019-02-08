#ifndef HERO_QUERY_H
#define HERO_QUERY_H
#include "proto/packet.h"
#include "conn.h"

int handle_mysql_command(front_conn* front);

int handle_com_query(front_conn* front);

int write_unkown_error_message(front_conn* front);

#endif