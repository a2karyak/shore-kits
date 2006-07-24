/* -*- mode:c++; c-basic-offset:4 -*- */

#ifndef _WORKLOAD_H
#define _WORKLOAD_H

#include "engine/util/c_str.h"
#include "engine/util/guard.h"
#include "workload/measurements.h"
#include "workload/driver.h"



using namespace qpipe;



// include me last!!!
#include "engine/namespace.h"



/**
 *  @class workload_t
 *
 *  @brief Most of the experiements we run on QPIPE involve the
 *  creation of N client threads, each or which submits M requests to
 *  the system. A workload is a simple container class designed to
 *  encapsulate (1) creating these client threads and (2) waiting for
 *  all of them to finish executing.
 */
class workload_t {

private:

    c_str _name;
  
    // what the workload does
    pointer_guard_t<driver_t> _driver;

    // other workload properties
    int _num_clients;
    int _think_time;
    int _num_iterations;

    void wait_for_clients(pthread_t* thread_ids, int num_thread_ids);

public:

    struct results_t {

        // store workload properties with the results
        int num_clients;
        int think_time;
        int num_iterations;

        // total execution time
        execution_time_t total_time;
        
        array_guard_t<execution_time_t> client_times;
    };

    workload_t(const c_str &name, driver_t* driver, int num_clients, int think_time, int num_iterations)
        : _name(name),
          _driver(driver->clone()),
          _num_clients(num_clients),
          _think_time(think_time),
          _num_iterations(num_iterations)
    {
    }

    ~workload_t() { }

    // run a workload and wait for it to finish executing
    bool run(results_t &results);
};



#include "engine/namespace.h"



#endif	// _WORKLOAD_H
