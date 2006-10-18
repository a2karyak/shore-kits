/* -*- mode:C++; c-basic-offset:4 -*- */

#include "scheduler.h"
#include "workload/tpch/drivers/tpch_sim_mix.h"

#include "workload/common.h"

ENTER_NAMESPACE(workload);


void tpch_sim_mix_driver::submit(void* disp) {
 
    thread_t* this_thread = thread_get_self();
    int selection = this_thread->rand(4);

    driver_t* driver = NULL;
    switch (selection) {
    case 0:
        driver = _directory->lookup_driver(c_str("q1"));
        break;
        
    case 1:
        driver = _directory->lookup_driver(c_str("q6"));
        break;
        
    case 2:
        driver = _directory->lookup_driver(c_str("q13"));
        break;
    case 3:
        driver = _directory->lookup_driver("q16");
        break;

    default:
        assert(false);
    }

    TRACE(TRACE_DEBUG, "selection = %d\n", selection);
    driver->submit(disp);
}

EXIT_NAMESPACE(workload);
