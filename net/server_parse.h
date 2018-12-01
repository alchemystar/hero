#ifndef HERO_SERVER_PARSE_H
#define HERO_SERVER_PARSE_H

#define OTHER -1
#define BEGIN 1
#define COMMIT 2
#define DELETE 3
#define INSERT 4
#define REPLACE 5
#define ROLLBACK 6
#define SELECT 7
#define SET 8
#define SHOW 9
#define START 10
#define UPDATE 11
#define KILL 12
#define SAVEPOINT 13
#define USE 14
#define EXPLAIN 15
#define KILL_QUERY 16
#define CREATE_DATABASE 17
#define RELOAD 18

int server_parse_sql(char* sql);

#endif