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

/** @file workload/tpcc/common/tpcc_random.cpp
 *  
 *  @brief Functions used for the generation of the inputs for 
 *         all the tpcc transaction.
 *
 *  @version Based on TPC-C Standard Specification Revision 5.4 (Apr 2005)
 */


#include "workload/tpcc/tpcc_random.h"
#include "workload/tpcc/tpcc_const.h"


ENTER_NAMESPACE(tpcc);


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
  assert (self);
  randgen_t* randgenp = self->randgen();
  assert (randgenp);

  int d = high - low + 1;

  /*
  if (((d & -d) == d) && (high > 1)) {
      // we avoid to pass a power of 2 to rand()
      return( low + ((randgenp->rand(high - low + 2) + randgenp->rand(high - low))/2) );
  }
  */

  //return (low + sthread_t::me()->rand());
  return (low + randgenp->rand(d));
}


/** @func NURand(int, int, int)
 *
 *  @brief Generates a non-uniform random number
 */

int NURand(int A, int low, int high) {

  thread_t* self = thread_get_self();
  assert (self);
  randgen_t* randgenp = self->randgen();
  assert (randgenp);

  return ( (((random(0, A, randgenp) | random(low, high, randgenp)) 
             + random(0, A, randgenp)) 
            % (high - low + 1)) + low );
}

/** @func generate_cust_last(int)
 *
 *  @brief Generates a customer last name (C_LAST) according to clause 4.3.2.3 of the
 *  TPCC specification. 
 *  The C_LAST must be generated by the concatenation of three variable length syllabes
 *  selected from the following list:
 *  0 BAR
 *  1 OUGHT
 *  2 ABLE
 *  3 PRI
 *  4 PRES
 *  5 ESE
 *  6 ANTI
 *  7 CALLY
 *  8 ATION
 *  9 EING
 *
 *  Given a number between 0 and 999 each of the three syllables is determined by the 
 *  corresponding digit representation of the number. 
 *  For example, 
 *  The number: 371 generates the name: PRICALLYOUGHT
 *  The number: 40 generates the name: BARPRESBAR
 *
 *  @return The length of the created string. 0 on errors.
 */

const char* CUST_LAST[10] = { "BAR", "OUGHT", "ABLE", "PRI", "PRES", "ESE",
                              "ANTI", "CALLY", "ATION", "EING" };


int generate_cust_last(int select, char* dest) 
{  
  assert ((select>=0) && (select<1000));
  assert (dest);

  int i1, i2, i3;

  i3 = select % 10;
  i2 = ((select % 100) - i3) / 10;
  i1 = (select - (select % 100)) / 100;

  int iLen = strlen(CUST_LAST[i1]) + strlen(CUST_LAST[i2]) + strlen(CUST_LAST[i3]) + 1;

  assert (iLen < 17);   // C_LAST is char[16]
  
  snprintf(dest, iLen, "%s%s%s", 
          CUST_LAST[i1],
          CUST_LAST[i2],
          CUST_LAST[i3]);
  dest[iLen] = '\0';
  
  return (iLen);
}




/******************************************************************************* 
 * 
 *  @func:  random_xct_type
 *  
 *  @brief: Translates or picks a random xct type given the benchmark 
 *          specification
 *
 *******************************************************************************/

int random_xct_type(int selected)
{
    int random_type = selected;
    if (random_type < 0)
        random_type = rand()%100;
    assert (random_type >= 0);

    int sum = 0;
    sum+=PROB_NEWORDER;
    if (random_type < sum)
	return XCT_NEW_ORDER;

    sum+=PROB_PAYMENT;
    if (random_type < sum)
	return XCT_PAYMENT;

    sum+=PROB_ORDER_STATUS;
    if (random_type < sum)
	return XCT_ORDER_STATUS;

    sum+=PROB_DELIVERY;
    if (random_type < sum)
	return XCT_DELIVERY;

    sum+=PROB_STOCK_LEVEL;
    if (random_type < sum)
	return XCT_STOCK_LEVEL;
}


EXIT_NAMESPACE(tpcc);
