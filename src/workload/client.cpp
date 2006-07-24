/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/client.h"
#include <unistd.h>



using namespace qpipe;



workload_client_t::workload_client_t(const c_str      &name,
                                     execution_time_t* etime,
                                     driver_t*         driver,
                                     client_wait_t*    wait,
                                     int               num_iterations,
                                     int               think_time_sec)
    : thread_t(name),
      _etime(etime),
      _driver(driver),
      _wait(wait),
      _num_iterations(num_iterations),
      _think_time_sec(think_time_sec)
{
    execution_time_t::reset(_etime);
}



workload_client_t::~workload_client_t() {
    // we don't own the execution_time_t
    // we don't own the driver_t
    // we don't own the client_wait_t
}



void* workload_client_t::run() {

    // wait for it...
    if ( !_wait->wait_for_runner() ) {
        // error! return immediately...
        return NULL;
    }

    
    // run client...
    _etime->start();

    for (int i = 0; i < _num_iterations; i++) {
        _driver->submit();
        if (_think_time_sec > 0)
            sleep(_think_time_sec);
    }    

    _etime->stop();
    return NULL;
}
