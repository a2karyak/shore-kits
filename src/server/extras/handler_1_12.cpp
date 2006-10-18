/* -*- mode:C++; c-basic-offset:4 -*- */
#include "scheduler.h"
#include "workload/common.h"
#include "workload/driver.h"
#include "workload/driver_directory.h"

using namespace workload;

class tpch_m_1_12_driver : public driver_t {

private:

    driver_directory_t* _directory;

public:

    tpch_m_1_12_driver(const c_str& description, driver_directory_t* directory)
        : driver_t(description),
          _directory(directory)
    {
    }
    
    virtual void submit(void* disp);
    
};


void tpch_m_1_12_driver::submit(void* disp) {
 
    // randomly select one of the drivers 1, 4, or 6...
    thread_t* this_thread = thread_get_self();
    int selection = this_thread->rand(2);

    driver_t* driver = NULL;
    switch (selection) {
    case 0:
        driver = _directory->lookup_driver(c_str("q1"));
        break;
        
    case 1:
        driver = _directory->lookup_driver("q12");
        break;

    default:
        assert(false);
    }

    TRACE(TRACE_DEBUG, "selection = %d\n", selection);
    driver->submit(disp);
}

// hook for dlsym
extern "C"
driver_t* m_1_12(const c_str& description, driver_directory_t* directory) {
    return new tpch_m_1_12_driver(description, directory);
}
