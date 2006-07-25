/* -*- mode:C++; c-basic-offset:4 -*- */

#include "server/command/tpch_handler.h"
#include "server/print.h"
#include "server/config.h"
#include "workload/tpch/tpch_db.h"
#include "workload/tpch/drivers/tpch_q1.h"

#include "engine/dispatcher/dispatcher_policy_os.h"
#include "engine/dispatcher/dispatcher_policy_rr_cpu.h"
#include "engine/dispatcher/dispatcher_policy_rr_module.h"
#include "engine/dispatcher/dispatcher_policy_query_cpu.h"

#include "qpipe_panic.h"




pthread_mutex_t tpch_handler_t::state_mutex = PTHREAD_MUTEX_INITIALIZER;

tpch_handler_t::state_t tpch_handler_t::state = TPCH_HANDLER_UNINITIALIZED;



/**
 *  @brief Initialize TPC-H handler. We must invoke db_open() to
 *  initial our global table environment, must we must ensure that
 *  this happens exactly once, despite the fact that we may register
 *  the same tpch_handler_t instance, or multiple tpch_handler_t
 *  instances for different command tags.
 */
void tpch_handler_t::init() {

    // use a global thread-safe state machine to ensure that db_open()
    // is called exactly once

    critical_section_t cs(&state_mutex);

    if ( state == TPCH_HANDLER_UNINITIALIZED ) {

        // open DB tables
        db_open();

        // register drivers...
        add_driver("1", new tpch_q1_driver(c_str("TPCH-Q1")));

        // register dispatcher policies...
        add_dispatcher_policy("OS",        new dispatcher_policy_os_t());
        add_dispatcher_policy("RR_CPU",    new dispatcher_policy_rr_cpu_t());
        add_dispatcher_policy("RR_MODULE", new dispatcher_policy_rr_module_t());
        add_dispatcher_policy("QUERY_CPU", new dispatcher_policy_query_cpu_t());
        
        state = TPCH_HANDLER_INITIALIZED;
    }


    
}



void tpch_handler_t::shutdown() {

    // use a global thread-safe state machine to ensure that
    // db_close() is called exactly once

    critical_section_t cs(&state_mutex);

    if ( state == TPCH_HANDLER_INITIALIZED ) {
        
        // close DB tables
        PRINT("... closing db\n");
        db_close();
        state = TPCH_HANDLER_SHUTDOWN;
    }
}



void tpch_handler_t::handle_command(const char* command) {

    int num_clients;
    int num_iterations;
    int think_time;
    

    // parse command
    char driver_tag[SERVER_COMMAND_BUFFER_SIZE];
    char dispatcher_policy_tag[SERVER_COMMAND_BUFFER_SIZE];
    if ( sscanf(command, "%*s %s %d %d %d %s",
                driver_tag,
                &num_clients,
                &num_iterations,
                &think_time,
                dispatcher_policy_tag) < 5 ) {
        
        char command_tag[SERVER_COMMAND_BUFFER_SIZE];
        int command_found = sscanf(command, "%s", command_tag);
        assert(command_found == 1);
        PRINT("Usage: %s"
              " <driver_tag>"
              " <num_clients>"
              " <num_iterations>"
              " <think_time>"
              " <dispatcher_policy_tag>"
              "\n",
              command_tag);
        return;
    }


    // debugging
    TRACE(TRACE_ALWAYS, "num_clients = %d, num_iterations = %d, think_time = %d, dispatcher_policy = %s\n",
          num_clients,
          num_iterations,
          think_time,
          dispatcher_policy_tag);
    

    // lookup driver
    driver_t* driver = _drivers[c_str(driver_tag)];
    if ( driver == NULL ) {
        // no such driver registered!
        PRINT("No driver registered for %s\n", driver_tag);
        return;
    }

    // lookup dispatcher policy
    dispatcher_policy_t* dp = _dispatcher_policies[c_str(dispatcher_policy_tag)];
    if ( dp == NULL ) {
        // no such policy registered!
        PRINT("%s is not a valid dispatcher policy\n", dispatcher_policy_tag);
        return;
    }
    

    // run new workload...
    workload_t::results_t results;
    c_str workload_name("%s + %s", driver_tag, dispatcher_policy_tag);
    workload_t w(workload_name, driver, dp, num_clients, num_iterations, think_time);
    w.run(results);

    
    // report results...
    print_run_statistics(results);
}



void tpch_handler_t::print_run_statistics(workload_t::results_t &results) {

    // print final statistics
    int queries_completed =
        results.num_clients * results.num_iterations;
    
    // queries per hour
    float tpmC = (60.0 * queries_completed) / results.total_time.time();
    
    TRACE(TRACE_STATISTICS, "~~~\n");
    TRACE(TRACE_STATISTICS, "~~~ Clients           = %d \n", results.num_clients);
    TRACE(TRACE_STATISTICS, "~~~ Iterations        = %d \n", results.num_iterations);
    TRACE(TRACE_STATISTICS, "~~~ Think Time        = %d \n", results.think_time);
    TRACE(TRACE_STATISTICS, "~~~ Duration          = %.2f \n", results.total_time.time());
    TRACE(TRACE_STATISTICS, "~~~\n");
    TRACE(TRACE_STATISTICS, "~~~ Throughput        = %.2f queries/min\n", tpmC);
    TRACE(TRACE_STATISTICS, "~~~\n");
}



void tpch_handler_t::add_driver(const char* tag, driver_t* driver) {
    add_driver(c_str(tag), driver);
}



void tpch_handler_t::add_driver(const c_str &tag, driver_t* driver) {
    // make sure no mapping currently exists
    assert( _drivers.find(tag) == _drivers.end() );
    _drivers[tag] = driver;
}



void tpch_handler_t::add_dispatcher_policy(const char* tag, dispatcher_policy_t* dp) {
    add_dispatcher_policy(c_str(tag), dp);
}



void tpch_handler_t::add_dispatcher_policy(const c_str &tag, dispatcher_policy_t* dp) {
    // make sure no mapping currently exists
    assert( _dispatcher_policies.find(tag) == _dispatcher_policies.end() );
    _dispatcher_policies[tag] = dp;
}
