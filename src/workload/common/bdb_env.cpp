/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file bdb_operation.cpp
 *
 *  @brief Common operations on the storage manager between the 
 *  two engines (tpch and tpcc)
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "util.h"
#include "workload/common/bdb_env.h"

using namespace workload;

/* exported data structures */


/* BerkeleyDB environment */
DbEnv* dbenv = NULL;


