/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : job.cpp
 *  @brief   : Implementation of a generic job (coded query) class
 *  @version : 0.1
 *  @history :
 6/15/2006: Initial version
*/

# include "job/job.h"


// include me last!!!
#include "namespace.h"



///////////////////
// class Job
//



/** @fn     : int register_job(string)
 *  @brief  : Register the job to the workload repository.
 *            The user can invoke the job by calling -j <unique_cmd>
 *  @return : 0 on success, 1 otherwise
 */

int job_t::register_job() {

    TRACE( TRACE_DEBUG, "Registering Job with id=%s\n", job_cmd.c_str());

    /** @TODO: Add command to repository, with a pointer to the start() */
    
    return (0);
}



/** @fn     : void unregister_job()
 *  @brief  : Unregister from the workload repository, allows dynamic loading/unloading of commands
 *  @return : 0 on success, 1 otherwise
 */

int job_t::unregister_job() {

    TRACE( TRACE_DEBUG, "Unregistering Job with id=%s\n", job_cmd.c_str());

    /** @TODO: Remove command from repository */
    
    return (0);
}





#include "namespace.h"
