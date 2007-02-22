/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/workload_client.h"
#include "core.h"
#include <unistd.h>



using namespace qpipe;

ENTER_NAMESPACE(workload);

workload_client_t::workload_client_t(const c_str      &name,
                                     execution_time_t* etime,
                                     driver_t*         driver,
                                     void*             driver_arg,
                                     client_wait_t*    wait,
                                     int               num_iterations,
                                     int               think_time_sec)
    : thread_t(name),
      _etime(etime),
      _driver(driver),
      _driver_arg(driver_arg),
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

    // record client start time...
    _etime->start();

    for (int i = 0; i < _num_iterations; i++) {
        _driver->submit(_driver_arg);
        if (_think_time_sec > 0)
            sleep(_think_time_sec);
        TRACE(TRACE_DEBUG, "Open Fifos: %d\n", tuple_fifo::open_fifos());
    }    

    // record client end time...
    _etime->stop();
    return NULL;
}

EXIT_NAMESPACE(workload);
