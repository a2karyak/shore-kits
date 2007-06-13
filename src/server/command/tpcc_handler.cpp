/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core.h"
#include "server/command/tpcc_handler.h"
#include "server/print.h"
#include "server/config.h"

#include "workload/tpcc/tpcc_db.h"

// tpcc drivers header files
#include "workload/tpcc/drivers/tpcc_payment.h"

#include "scheduler.h"

using namespace tpcc;
using namespace workload;
using qpipe::tuple_fifo;

pthread_mutex_t tpcc_handler_t::state_mutex = PTHREAD_MUTEX_INITIALIZER;

tpcc_handler_t::state_t tpcc_handler_t::state = TPCC_HANDLER_UNINITIALIZED;

tpcc_handler_t *tpcc_handler = NULL;

/**
 *  @brief Initialize TPC-C handler. We must invoke db_open() to
 *  initial our global table environment, must we must ensure that
 *  this happens exactly once, despite the fact that we may register
 *  the same tpcc_handler_t instance, or multiple tpcc_handler_t
 *  instances for different command tags.
 */

void tpcc_handler_t::init() {

    // use a global thread-safe state machine to ensure that db_open()
    // is called exactly once

    critical_section_t cs(state_mutex);

    if ( state == TPCC_HANDLER_UNINITIALIZED ) {

        // open DB tables (1.25 GB bpool)       
        db_open();

        // register drivers...
        add_driver("payment", new tpcc_payment_driver(c_str("PAYMENT")));

        // register dispatcher policies...
        add_scheduler_policy("OS",        new scheduler::policy_os_t());
        add_scheduler_policy("RR_CPU",    new scheduler::policy_rr_cpu_t());
        add_scheduler_policy("RR_MODULE", new scheduler::policy_rr_module_t());
        add_scheduler_policy("QUERY_CPU", new scheduler::policy_query_cpu_t());

        tpcc_handler = this;
        state = TPCC_HANDLER_INITIALIZED;
    }


    
}



void tpcc_handler_t::shutdown() {

    // use a global thread-safe state machine to ensure that
    // db_close() is called exactly once

    critical_section_t cs(state_mutex);

    if ( state == TPCC_HANDLER_INITIALIZED ) {
        
        // close DB tables
        TRACE(TRACE_ALWAYS, "... closing db\n");
	db_close();

        state = TPCC_HANDLER_SHUTDOWN;
    }
}



void tpcc_handler_t::handle_command(const char* command) {

    int num_clients;
    int num_iterations;
    int think_time;
    

    // parse command
    char driver_tag[SERVER_COMMAND_BUFFER_SIZE];
    char scheduler_policy_tag[SERVER_COMMAND_BUFFER_SIZE];
    

    // parse driver tag
    if ( sscanf(command, "%*s %s", driver_tag) < 1 ) {
        char command_tag[SERVER_COMMAND_BUFFER_SIZE];
        int command_found = sscanf(command, "%s", command_tag);
        assert(command_found == 1);
        print_usage(command_tag);
        return;
    }


    // 'list' tag handled differently from all others...
    if (!strcmp(driver_tag, "list"))
    {
        for (map<c_str, driver_t*>::iterator it = _drivers.begin(); 
	     it != _drivers.end(); ++it)
        {
            TRACE(TRACE_ALWAYS, "%s\n", it->first.data());
        }

        return;
    }
   
    
    if ( sscanf(command, "%*s %*s %d %d %d %s",
                &num_clients,
                &num_iterations,
                &think_time,
                scheduler_policy_tag) < 4 ) 
    {
        char command_tag[SERVER_COMMAND_BUFFER_SIZE];
        int command_found = sscanf(command, "%s", command_tag);
        assert(command_found == 1);
        print_usage(command_tag);
        return;
    }


    // debugging
    TRACE(TRACE_ALWAYS, "num_cl = %d, num_iter = %d, think_time = %d, sched_policy = %s\n",
          num_clients,
          num_iterations,
          think_time,
          scheduler_policy_tag);
    

    // lookup driver
    driver_t* driver = lookup_driver(c_str(driver_tag));
    if ( driver == NULL ) {
        // no such driver registered!
        TRACE(TRACE_ALWAYS, "No driver registered for %s\n", driver_tag);
        return;
    }

    // lookup dispatcher policy
    scheduler::policy_t* dp = _scheduler_policies[c_str(scheduler_policy_tag)];

    if (dp == NULL) {

        // no such policy registered!
        TRACE(TRACE_ALWAYS, "%s is not a valid dispatcher policy\n", scheduler_policy_tag);
        return;
    }
    

    /* We will construct a workload object to run this experiment. The
       workload object will create the required worker threads,
       provide the synchronization required to coordinate their
       iteration, etc. */

    /* Provide the workload with a name so it has an intelligent way
       to name its client threads. */
    c_str workload_name("%s+%s", driver_tag, scheduler_policy_tag);
    workload_t w(workload_name, driver, dp, num_clients,
                 num_iterations, think_time);

    /* The basic interface is the run() method, which takes a
       statistics object and fills it with the stats collected during
       the workload execution. */
    workload_t::results_t results;
    w.run(results);

    /* Report results. We'll use the workload name for its
       description. */
    print_run_statistics(workload_name, results);
}



void tpcc_handler_t::print_usage(const char* command_tag)
{
    TRACE(TRACE_ALWAYS, "Usage: %s"
          " list | <driver_tag> <num_clients> <num_iterations> <think_time> <scheduler_policy_tag>"
          "\n",
          command_tag);
}


void tpcc_handler_t::print_run_statistics(const c_str& desc, 
					  workload_t::results_t &results) 
{
    tuple_fifo::clear_stats();
    
    // print final statistics
    int trx_completed = results.num_clients * results.num_iterations;
    
    // queries per hour
    float tpmC = (60.0 * trx_completed) / results.total_time;
    
    TRACE(TRACE_STATISTICS, "~~~\n");
    TRACE(TRACE_STATISTICS, "~~~ Description       = %s\n", desc.data());
    TRACE(TRACE_STATISTICS, "~~~ Clients           = %d \n", results.num_clients);
    TRACE(TRACE_STATISTICS, "~~~ Iterations        = %d \n", results.num_iterations);
    TRACE(TRACE_STATISTICS, "~~~ Think Time        = %d \n", results.think_time);
    TRACE(TRACE_STATISTICS, "~~~ Duration          = %.2f \n", results.total_time);
    TRACE(TRACE_STATISTICS, "~~~\n");
    TRACE(TRACE_STATISTICS, "~~~ Throughput        = %.2f trx/min\n", tpmC);
    TRACE(TRACE_STATISTICS, "~~~\n");
}



void tpcc_handler_t::add_driver(const char* tag, driver_t* driver) {

    add_driver(c_str(tag), driver);
}



void tpcc_handler_t::add_driver(const c_str &tag, driver_t* driver) {

    // make sure no mapping currently exists
    assert( _drivers.find(tag) == _drivers.end() );
    _drivers[tag] = driver;
}



void tpcc_handler_t::add_scheduler_policy(const char* tag, 
					  scheduler::policy_t* dp) 
{
    add_scheduler_policy(c_str(tag), dp);
}



void tpcc_handler_t::add_scheduler_policy(const c_str &tag, 
					  scheduler::policy_t* dp) 
{
    // make sure no mapping currently exists
    assert( _scheduler_policies.find(tag) == _scheduler_policies.end() );
    _scheduler_policies[tag] = dp;
}



driver_t* tpcc_handler_t::lookup_driver(const c_str &tag) {
    return _drivers[tag];
}

