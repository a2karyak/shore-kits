/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file store_string.cpp
 *
 *  @brief Implementation of string manipulation functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util/store_string.h"

#include <cstdio>
#include <cstring>

/* definitions of exported helper functions */


/** @fn store_string
 *  @brief copies a string to another
 */

void store_string(char* dest, char* src) {
    int len = strlen(src);
    strncpy(dest, src, len);
    dest[len] = '\0';
}

