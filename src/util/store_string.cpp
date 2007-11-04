/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file store_string.cpp
 *
 *  @brief Implementation of string manipulation functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util/store_string.h"


#ifdef __SUNPRO_CC
#include <stdio.h>
#include <string.h>
#else
#include <cstdio>
#include <cstring>
#endif


/* definitions of exported helper functions */


/** @fn store_string
 *  @brief Copies a string to another
 */

void store_string(char* dest, char* src) {
    int len = strlen(src);
    strncpy(dest, src, len);
    dest[len] = '\0';
}



/** @fn store_string
 *  @brief Copies a const string to another
 */


void store_string(char* dest, const char* src) {
    int len = strlen(src);
    strncpy(dest, src, len);
    dest[len] = '\0';
}

