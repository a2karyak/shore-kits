/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file workload/tpcc/common/tpcc_random.h
 *  
 *  @brief Functions used for the generation of the inputs for
 *         all the TPC-C transactions
 *
 *  @version Based on TPC-C Standard Specification Revision 5.4 (Apr 2005)
 */

#ifndef __TPCC_COMMON_H
#define __TPCC_COMMON_H

#include "util.h"


ENTER_NAMESPACE(tpcc);


/** Terminology
 *  
 *  [x .. y]:        Represents a closed range of values starting with x 
 *                   and ending with y
 *  NURand(A, x, y): Non-uniform distributed value between x and y
 *  URand(x, y):     Uniformly distributed value between x and y
 */                  


/** Exported Functions */

int NURand(int A, int low, int high);

int generate_cust_last(int select, char* dest);

EXIT_NAMESPACE(tpcc);

#endif

