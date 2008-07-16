/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_shore_env.h
 *
 *  @brief:  Defines common tester functions and variables related to the 
 *           Shore environment.
 *
 *  @author: Ippokratis Pandis, July 2008
 *
 */

#ifndef __TESTER_SHORE_ENV_H
#define __TESTER_SHORE_ENV_H


#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"


using namespace shore;
using namespace tpcc;


//// Default values for the environment

// default database size (scaling factor)
const int DF_NUM_OF_WHS = 10;
extern int _numOfWHs;


// default queried number of warehouses (queried factor)
const int DF_NUM_OF_QUERIED_WHS = 10;

// Instanciate and close the Shore environment
int inst_test_env(int argc, char* argv[]);
int close_test_env();

// Helper
void print_wh_usage(char* argv[]);



#endif /** __TESTER_SHORE_ENV_H */
