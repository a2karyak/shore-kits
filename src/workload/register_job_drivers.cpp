/* -*- mode:C++; c-basic-offset:4 -*- */

#include "trace.h"
#include "workload/job_directory.h"
#include "workload/tpch/jobs/tpch_q1.h"
#include "workload/tpch/jobs/tpch_q6.h"
#include "workload/tpch/jobs/tpch_q4.h"



/** @fn    : register_job_drivers()
 *  @brief : Registers all the implemented Jobs
 */
void register_job_drivers() {

    tpch_q1_driver* q1_driver = new tpch_q1_driver();
    job_directory::register_job_driver("1", q1_driver);

    tpch_q6_driver* q6_driver = new tpch_q6_driver();
    job_directory::register_job_driver("6", q6_driver);

    tpch_q4_driver* q4_driver = new tpch_q4_driver();
    job_directory::register_job_driver("4", q4_driver);
}
