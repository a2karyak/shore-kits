/* -*- mode:c++; c-basic-offset:4 -*- */

#ifndef _WORKLOAD_H
#define _WORKLOAD_H

#include "util.h"
#include "workload/measurements.h"
#include "workload/driver.h"




ENTER_NAMESPACE(workload);

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
    driver_t* _driver;     // we don't own this...
    void*     _driver_arg; // we don't own this...

    // other workload properties
    int _num_clients;
    int _num_iterations;
    int _think_time;


    void wait_for_clients(pthread_t* thread_ids, int num_thread_ids);

public:

    struct results_t {

        // store workload properties with the results
        int num_clients;
        int num_iterations;
        int think_time;

        // total execution time
        double total_time;
        
        array_guard_t<execution_time_t> client_times;
    };

    workload_t(const c_str &name,
               driver_t*    driver,
               void*        driver_arg,
               int          num_clients,
               int          num_iterations,
               int          think_time)
        : _name(name),
          _driver(driver),
          _driver_arg(driver_arg),
          _num_clients(num_clients),
          _num_iterations(num_iterations),
          _think_time(think_time)
    {
    }

    ~workload_t() { }

    // run a workload and wait for it to finish executing
    bool run(results_t &results);
};

EXIT_NAMESPACE(workload);

#endif	// _WORKLOAD_H
