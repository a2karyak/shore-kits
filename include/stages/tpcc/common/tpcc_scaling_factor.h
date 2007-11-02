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
# define TPCC_SCALING_FACTOR             10
//# define TPCC_SCALING_FACTOR             100


// Sets the range of warehouses queried
// @note The tpcc_drivers create their requests bases on this value


//# define QUERIED_TPCC_SCALING_FACTOR             1
# define QUERIED_TPCC_SCALING_FACTOR             10
//# define QUERIED_TPCC_SCALING_FACTOR             100


#endif

