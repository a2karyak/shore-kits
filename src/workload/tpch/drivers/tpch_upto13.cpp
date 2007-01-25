/* -*- mode:C++; c-basic-offset:4 -*- */

#include "scheduler.h"
#include "workload/tpch/drivers/tpch_upto13.h"

#include "workload/common.h"

ENTER_NAMESPACE(workload);


void tpch_upto13_driver::submit(void* disp) {
 
    thread_t* this_thread = thread_get_self();
    int selection = this_thread->rand(5);

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

    case 4:
        driver = _directory->lookup_driver("q13");
        break;
        

    default:
        unreachable();
    }

    TRACE(TRACE_DEBUG, "selection = %d\n", selection);
    driver->submit(disp);
}

EXIT_NAMESPACE(workload);
