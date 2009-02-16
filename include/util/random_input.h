/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   random_input.h
 *
 *  @brief:  Declaration of the (common) random functions, used for the
 *           workload inputs
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */


#ifndef __UTIL_RANDOM_INPUT_H
#define __UTIL_RANDOM_INPUT_H


#include "util.h"


/** Exported helper functions */

const int   URand(const int low, const int high);
const bool  URandBool();
const short URandShort(const short low, const short high);


const char CAPS_CHAR_ARRAY[]  = { "ABCDEFGHIJKLMNOPQRSTUVWXYZ" };
const char NUMBERS_CHAR_ARRAY[] = { "012345789" }; 


const void URandFillStrCaps(char* dest, const int sz);
const void URandFillStrNumbx(char* dest, const int sz);




/* use this for allocation of NULL-terminated strings */
#define STRSIZE(x)(x+1)



#endif
