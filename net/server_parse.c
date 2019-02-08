#include "server_parse.h"
#include <string.h>

int s_check(char* sql,int offset,int length);
int se_check(char* sql,int offset,int length);
int select_check(char* sql,int offset,int length);
int show_check(char* sql,int offset,int length);
int kill_check(char* sql,int offset,int length);

int server_parse_sql(char* sql){
    printf("sql = %s\n",sql);
    int length = strlen(sql);
    for(int i=0 ; i < length ; i++){
        switch(sql[i]){
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                continue;
            case '/':
            case '#':
                // parse comment
                continue;
            case 'B':
            case 'b':
                // parse begin
                return -1;
            case 'C':
            case 'c':
                // parse schema and commit
                return -1;
            case 'D':
            case 'd':
                // parse delete
                return -1;
            case 'E':
            case 'e':
                // parse explain
                return -1;
            case 'I':
            case 'i':
                // parse insert    
                return -1;
            case 'R':
            case 'r':
                // r check
                return -1;    
            case 'S':
            case 's':
                // s check
                return s_check(sql,i,length);
            case 'U':
            case 'u':
                // u check
                return -1;
            case 'K':
            case 'k':
                return k_check(sql,i,length);
            default:
                return -1;                
        }
    }
}

int k_check(char* sql,int offset,int length){
    return kill_check(sql,offset,length);
}

int kill_check(char* sql,int offset,int length){
    printf("go into kill check\n");
    if(length > offset + 4){
        char c1 = sql[++offset];
        char c2 = sql[++offset];
        char c3 = sql[++offset];
        char c4 = sql[++offset];
        if ((c1 == 'i' || c1 == 'I') && (c2 == 'l' || c2 == 'L') && (c3=='l' || c3 == 'L') && (c4 == ' ' || c4 == '\t' || c4 == '\r' || c4 == '\n')) {
            return (offset << 8) | KILL_QUERY;
        }
    }
    return OTHER;
}

int s_check(char* sql,int offset,int length){
    if(length > ++offset){
        switch(sql[offset]){
            case 'A':
            case 'a':
                // save point check
                return -1;
            case 'E':
            case 'e':
                // se check
                return se_check(sql,offset,length);
            case 'H':
            case 'h':
                // show check;
                return show_check(sql,offset,length);   
            case 'T':
            case 't':
                // start check
                return -1;    
            default:
                return OTHER;         
        }
    }
    return OTHER;
}

int show_check(char* sql,int offset,int length){
    if(length > offset + 3){
        char c1 = sql[++offset];
        char c2 = sql[++offset];
        char c3 = sql[++offset];
        if ((c1 == 'O' || c1 == 'o') && (c2 == 'W' || c2 == 'w') && (c3 == ' ' || c3 == '\t' || c3 == '\r' || c3 == '\n')) {
            return (offset << 8) | SHOW;
        }
    }
    return OTHER;
}

int se_check(char* sql,int offset,int length){
    if(length > ++offset){
        switch(sql[offset]){
            case 'L':
            case 'l':
                return select_check(sql,offset,length);
            case 'T':
            case 't':
                // set check
                return OTHER;    
            default:
                return OTHER;
        }
    }
    return OTHER;
}

int select_check(char* sql,int offset,int length){
    if(length > offset + 4){
        char c1 = sql[++offset];
        char c2 = sql[++offset];
        char c3 = sql[++offset];
        char c4 = sql[++offset];
        if ((c1 == 'E' || c1 == 'e') && (c2 == 'C' || c2 == 'c') && (c3 == 'T' || c3 == 't')
                    && (c4 == ' ' || c4 == '\t' || c4 == '\r' || c4 == '\n' || c4 == '/' || c4 == '#')) {
            return (offset << 8) | SELECT;   
        }     
    }
}