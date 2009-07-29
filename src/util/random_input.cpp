/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
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

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   random_input.cpp
 *
 *  @brief:  Definition of the (common) random functions, used for the
 *           workload inputs
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */


#include "util/random_input.h"



int URand(const int low, const int high) 
{
  thread_t* self = thread_get_self();
  assert (self);
  randgen_t* randgenp = self->randgen();
  assert (randgenp);

  int d = high - low + 1;
  return (low + randgenp->rand(d));
}


bool URandBool()
{
    return (URand(0,1) ? true : false);
}


short URandShort(const short low, const short high) 
{
  thread_t* self = thread_get_self();
  assert (self);
  randgen_t* randgenp = self->randgen();
  assert (randgenp);

  short d = high - low + 1;
  return (low + (short)randgenp->rand(d));
}


void URandFillStrCaps(char* dest, const int sz)
{
    assert (dest);
    for (int i=0; i<sz; i++) {
        dest[i] = CAPS_CHAR_ARRAY[URand(0,sizeof(CAPS_CHAR_ARRAY)-1)];
    }
}


void URandFillStrNumbx(char* dest, const int sz)
{
    assert (dest);
    for (int i=0; i<sz; i++) {
        dest[i] = NUMBERS_CHAR_ARRAY[URand(0,sizeof(NUMBERS_CHAR_ARRAY)-1)];
    }
}


