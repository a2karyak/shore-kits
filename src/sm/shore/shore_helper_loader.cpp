/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_helper_loader.cpp
 *
 *  @brief Declaration of helper loader thread classes
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "sm/shore/shore_helper_loader.h"
#include "sm/shore/shore_env.h"


using namespace shore;


/** Exported functions */



/****************************************************************** 
 *
 * class db_init_smt_t
 *
 ******************************************************************/

void db_init_smt_t::work()
{
    if (!_env->is_initialized()) {
        if (_rv = _env->init()) {
            // Couldn't initialize the Shore environment
            // cannot proceed
            TRACE( TRACE_ALWAYS, "Couldn't initialize Shore...\n");
            return;
        }
    }

    // if reached this point everything went ok
    TRACE( TRACE_DEBUG, "Shore initialized...\n");
    _rv = 0;
}



