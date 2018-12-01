#include "server_parse.h"
#include <string.h>

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
                return -1;
            case 'U':
            case 'u':
                // u check
                return -1;
            case 'K':
            case 'k':
                return -1;
            default:
                return -1;                
        }
    }
}//