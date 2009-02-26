/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   random_input.cpp
 *
 *  @brief:  Definition of the (common) random functions, used for the
 *           workload inputs
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */


#include "util/random_input.h"



const int URand(const int low, const int high) 
{
  thread_t* self = thread_get_self();
  assert (self);
  randgen_t* randgenp = self->randgen();
  assert (randgenp);

  int d = high - low + 1;
  return (low + randgenp->rand(d));
}


const bool URandBool()
{
    return (URand(0,1) ? true : false);
}


const short URandShort(const short low, const short high) 
{
  thread_t* self = thread_get_self();
  assert (self);
  randgen_t* randgenp = self->randgen();
  assert (randgenp);

  short d = high - low + 1;
  return (low + (short)randgenp->rand(d));
}

const void URandFillStrCaps(char* dest, const int sz)
{
    assert (dest);
    for (int i=0; i<sz; i++) {
        dest[i] = CAPS_CHAR_ARRAY[URand(0,sizeof(CAPS_CHAR_ARRAY)-1)];
    }
}

const void URandFillStrNumbx(char* dest, const int sz)
{
    assert (dest);
    for (int i=0; i<sz; i++) {
        dest[i] = NUMBERS_CHAR_ARRAY[URand(0,sizeof(NUMBERS_CHAR_ARRAY)-1)];
    }
}


