/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file chomp.cpp
 *
 *  @brief Implementation of the chomp helper function
 *
 *  @author Naju Mancheril (ngm)
 */

#include "util/chomp.h"


#ifdef __SUNPRO_CC
#include <string.h>
#else
#include <cstring>
#endif


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
