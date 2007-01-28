
#include "util/chomp.h"

#include <cstring>

#ifndef __GCC
using std::strlen;
#endif

void chomp_newline(char* str) {
    int len = strlen(str);
    if ( str[len-1] == '\n' )
        str[len-1] = '\0';
}


void chomp_carriage_return(char* str) {
    int len = strlen(str);
    if ( str[len-1] == '\r' )
        str[len-1] = '\0';
}
