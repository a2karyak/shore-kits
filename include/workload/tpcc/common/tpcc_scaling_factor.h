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

/** @file tpcc_scaling_factor.h
 *
 *  @brief Header file which only declares the scaling factor of 
 *  the TPC-C database
 */

#ifndef __TPCC_SCALING_FACTOR_H
#define __TPCC_SCALING_FACTOR_H

// Sets the scaling factor of the TPC-C database
// @note Some data structures base their size on this value

//# define TPCC_SCALING_FACTOR             1
 #define TPCC_SCALING_FACTOR             10
//# define TPCC_SCALING_FACTOR             100

//# define QUERIED_TPCC_SCALING_FACTOR             1
//# define QUERIED_TPCC_SCALING_FACTOR             8
//# define QUERIED_TPCC_SCALING_FACTOR             10
# define QUERIED_TPCC_SCALING_FACTOR             100


/** Use this definition to produce the same packet over and over */
//#define USE_SAME_INPUT 

extern int selectedQueriedSF;


#endif

