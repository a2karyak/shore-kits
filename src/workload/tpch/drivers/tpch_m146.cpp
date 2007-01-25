/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/drivers/tpch_m146.h"

#include "workload/common.h"

ENTER_NAMESPACE(workload);



void tpch_m146_driver::submit(void* disp) {
 
    // randomly select one of the drivers 1, 4, or 6...
    thread_t* this_thread = thread_get_self();
    int rand = this_thread->rand();
    int selection = rand % 3;

    driver_t* driver;
    switch (selection) {
    case 0:
        driver = _directory->lookup_driver(c_str("q1"));
        break;
        
    case 1:
        driver = _directory->lookup_driver(c_str("q4"));
        break;
        
    case 2:
        driver = _directory->lookup_driver(c_str("q6"));
        break;

    default:
        unreachable();
    }

    TRACE(TRACE_DEBUG, "rand = %d, selection = %d\n", rand, selection);
    driver->submit(disp);
}

EXIT_NAMESPACE(workload);
