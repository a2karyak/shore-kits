/* -*- mode:c++; c-basic-offset:4 -*- */

#ifndef __WORKLOAD_CLIENT_H
#define __WORKLOAD_CLIENT_H


#include "memory/mem_obj.h"

#include "workload/driver.h"
#include "workload/client_sync.h"
#include "workload/measurements.h"



ENTER_NAMESPACE(workload);


//------------------------------------------------------
//  @class workload_client_t
//
//  @brief A client thread created and run by a workload.
//

class workload_client_t : public thread_t {

private:

    c_str _name;

    execution_time_t* _etime;
    driver_t*         _driver;
    void*             _driver_arg;
    client_wait_t*    _wait;

    int _num_iterations;
    int _think_time_sec;

public:

    memObject_t _mem;
    

    workload_client_t(const c_str      &name,
                      execution_time_t* etime,
                      driver_t*         driver,
                      void*             driver_arg,
                      client_wait_t*    wait,
                      int               num_iterations,
                      int               think_time_sec);
                      
    virtual ~workload_client_t();

    virtual void run();

    virtual void allocate_resources() {
        assert(_driver);
        _driver->f_allocate(&_mem);
    }

    virtual void deallocate_resources() {
        assert(_driver);
        _driver->f_deallocate(&_mem);
    }


}; // EOF workload_client



EXIT_NAMESPACE(workload);


#endif
