#include "handle_util.h"

int is_char_eof(unsigned char c){
    if(c == '\0' || c == ' ' || c == '\n' || c == '\r' || c == '\n' || c==';'){
        return TRUE;
    }
    return FALSE;
}