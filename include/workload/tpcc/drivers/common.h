/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file workload/tpcc/drivers/common.h
 *  
 *  @brief Functions used for the generation of all the tpcc transaction.
 *  @version Based on TPC-C Standard Specification Revision 5.4 (Apr 2005)
 */

#ifndef __TPCC_COMMON_H
#define __TPCC_COMMON_H

#include "util.h"


ENTER_NAMESPACE(tpcc);

/** Terminology
 *  
 *  [x .. y]:        Represents a closed range of values starting with x and ending 
 *                   with y
 *  NURand(A, x, y): Non-uniform distributed value between x and y
 *  URand(x, y):     Uniformly distributed value between x and y
 */                  


/** Exported Functions */

int URand(int low, int high);

int NURand(int A, int low, int high);

char* generate_cust_last(int select);

EXIT_NAMESPACE(tpcc);

#endif

