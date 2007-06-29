/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/drivers/tpch_m14612.h"

#include "workload/common.h"

ENTER_NAMESPACE(workload);


void tpch_m14612_driver::submit(void* disp, memObject_t* mem) {
 
    // randomly select one of the drivers 1, 4, or 6...
    thread_t* this_thread = thread_get_self();
    int selection = this_thread->rand(4);

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
    case 3:
        driver = _directory->lookup_driver("q12");
        break;

    default:
        unreachable();
    }

    TRACE(TRACE_DEBUG, "selection = %d\n", selection);
    driver->submit(disp, mem);
}

EXIT_NAMESPACE(workload);
