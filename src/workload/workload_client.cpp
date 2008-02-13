/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/workload_client.h"

#include "core.h"


#include <unistd.h>


using namespace qpipe;


ENTER_NAMESPACE(workload);


#define TRACE_ITERATIONS 0


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
    // allocate any memory required for this driver
    allocate_resources();
            
    // reset timer
    execution_time_t::reset(_etime);
}



inline workload_client_t::~workload_client_t() {
    // we don't own the execution_time_t
    // we don't own the driver_t
    // we don't own the client_wait_t

    // we own our memory object
    deallocate_resources();
}



void workload_client_t::run() {

    // wait for it...
    if ( !_wait->wait_for_runner() ) {
        // error! return immediately...
        return;
    }
    // record client start time...
    _etime->start();

    stopwatch_t qtime;
    for (int i = 0; i < _num_iterations; i++) {

        TRACE( TRACE_QUERY_RESULTS,
              "Iteration %d\n", i);        

	TRACE(TRACE_RESPONSE_TIME, "Query '%s' started at %lld\n",
	      _driver->description().data(), qtime.now());

        _driver->submit(_driver_arg, &_mem);

	TRACE(TRACE_RESPONSE_TIME, "Query '%s' finished at %lld\n",
	      _driver->description().data(), qtime.now());

        if (_think_time_sec > 0)
            sleep(_think_time_sec);

        TRACE(TRACE_DEBUG, "Open Fifos: %d\n", tuple_fifo::open_fifos());

        TRACE( TRACE_DEBUG, "Done with Iteration %d\n", i);
    }    

    // record client end time...
    _etime->stop();
}


EXIT_NAMESPACE(workload);
