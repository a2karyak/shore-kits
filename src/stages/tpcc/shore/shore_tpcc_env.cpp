/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_env.cpp
 *
 *  @brief Declaration of the Shore TPC-C environment (database)
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/shore_tpcc_const.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"


using namespace shore;

ENTER_NAMESPACE(tpcc);


/** Exported variables */

ShoreTPCCEnv* shore_env;


/** Exported functions */


/********
 ******** Caution: The functions below should be invoked inside
 ********          the context of a smthread
 ********/


int ShoreTPCCEnv::loaddata() 
{
    assert (false); // (ip) TODO
    return (0);
}


EXIT_NAMESPACE(tpcc);
