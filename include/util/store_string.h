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


void store_string(char* dest, const char* src);


#endif
