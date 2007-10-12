/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file store_string.h
 *
 *  @brief String manipulation functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __UTIL_STORE_STRING_H
#define __UTIL_STORE_STRING_H


/** exported helper functions */

#ifdef __SUNPRO_CC
#include <string.h>
#else
#include <cstring>
#endif

#define FILL_STRING(dest, src) strncpy(dest, src, sizeof(dest))

void store_string(char* dest, char* src);


#endif
