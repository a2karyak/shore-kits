/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University

                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/


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
