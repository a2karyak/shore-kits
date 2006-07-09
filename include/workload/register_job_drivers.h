/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : repos.h
 *  @brief   : Definition of the implemented Jobs (compiled queries)
 *  @version : 0.1
 *  @history :
 7/7/2006: Initial version
*/ 

#ifndef __REPOS_H
#define __REPOS_H

#include "workload/job_directory.h"
#include "workload/repos/tpch_q1.h"
#include "workload/repos/tpch_q6.h"



/** @fn    : register_job_drivers()
 *  @brief : Registers all the implemented Jobs
 */

void register_job_drivers() {

    TRACE( TRACE_ALWAYS, " ** SHOULD REGISTER JOBS ** \n");

    
    tpch_q1_driver* q1_driver = new tpch_q1_driver();
    job_directory::register_job_driver("1", q1_driver);


    tpch_q6_driver* q6_driver = new tpch_q6_driver();
    job_directory::register_job_driver("6", q6_driver);
}


#endif // __REPOS_H
