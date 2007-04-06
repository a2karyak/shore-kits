/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file workload/tpcc/drivers/common.cpp
 *  
 *  @brief Functions used for the generation of all the tpcc transaction.
 *  @version Based on TPC-C Standard Specification Revision 5.4 (Apr 2005)
 */

#include "workload/tpcc/drivers/common.h"

ENTER_NAMESPACE(workload);


/** Terminology
 *  
 *  [x .. y]: Represents a closed range of values starting with x and ending 
 *            with y
 *
 *  random(x,y): uniformly distributed value between x and y.
 *
 *  NURand(A, x, y): (((random(0, A) | random(x, y)) + C) % (y - x + 1)) + x
 *                   non-uniform random, where
 *  exp-1 | exp-2: bitwise logical OR
 *  exp-1 % exp-2: exp-1 modulo exp-2
 *  C: random(0, A)
 */                  


/** FIXME: (ip) random and NURand handle only integers, should add at least doubles */



/** @func random(int, int, randgen_t*)
 *
 *  @brief Generates a uniform random number between low and high. 
 *  Not seen by public.
 */

int random(int low, int high, randgen_t* rp) {

  return (low + rp->rand(high - low + 1));
}


/** @func URand(int, int)
 *
 *  @brief Generates a uniform random number between (low) and (high)
 */

int URand(int low, int high) {

  thread_t* self = thread_get_self();
  randgen_t* randgenp = self->randgen();

  return (low + randgenp->rand(high - low + 1));
}


/** FIXME: (ip) C has a constant value across ($2.1.6.1) */

/** @func NURand(int, int, int)
 *
 *  @brief Generates a non-uniform random number
 */

int NURand(int A, int low, int high) {

  thread_t* self = thread_get_self();
  randgen_t* randgenp = self->randgen();

  return ( (((random(0, A, randgenp) | random(low, high, randgenp)) 
             + random(0, A, randgenp)) 
            % (high - low + 1)) + low );
}

EXIT_NAMESPACE(workload);
