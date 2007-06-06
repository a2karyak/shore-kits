/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core.h"
#include "server/command/tpch_handler.h"
#include "server/print.h"
#include "server/config.h"
#include "workload/tpch/tpch_db.h"

#include "workload/tpch/drivers/merge_test.h"

#include "workload/tpch/drivers/tpch_q1.h"
#include "workload/tpch/drivers/tpch_q4.h"
#include "workload/tpch/drivers/tpch_q6.h"
#include "workload/tpch/drivers/tpch_q12.h"
#include "workload/tpch/drivers/tpch_q13.h"
#include "workload/tpch/drivers/tpch_q14.h"
#include "workload/tpch/drivers/tpch_q16.h"
#include "workload/tpch/drivers/tpch_q19.h"
// #include "workload/tpch/drivers/tpch_q6pf.h"
// #include "workload/tpch/drivers/tpch_q6pipe.h"
// #include "workload/tpch/drivers/tpch_q1pipe.h"

#include "workload/tpch/drivers/tpch_m146.h"
#include "workload/tpch/drivers/tpch_m_1_4_6_12.h"
#include "workload/tpch/drivers/tpch_upto13.h"
#include "workload/tpch/drivers/tpch_m14612.h"
#include "workload/tpch/drivers/tpch_sim_mix.h"

#include "scheduler.h"


using namespace tpch;
using namespace workload;
using qpipe::tuple_fifo;

pthread_mutex_t tpch_handler_t::state_mutex = PTHREAD_MUTEX_INITIALIZER;

tpch_handler_t::state_t tpch_handler_t::state = TPCH_HANDLER_UNINITIALIZED;

tpch_handler_t *tpch_handler = NULL;

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

    critical_section_t cs(state_mutex);

    if ( state == TPCH_HANDLER_UNINITIALIZED ) {

        // open DB tables (1.25 GB bpool)

        db_open();

        // register drivers...
        add_driver("merge_test", new merge_test_driver(c_str("MERGE-TEST")));
        add_driver("q1", new tpch_q1_driver(c_str("TPCH-Q1")));
        add_driver("q4", new tpch_q4_driver(c_str("TPCH-Q4")));
        add_driver("q6", new tpch_q6_driver(c_str("TPCH-Q6")));
        add_driver("q12", new tpch_q12_driver(c_str("TPCH-Q12")));
        add_driver("q13", new tpch_q13_driver(c_str("TPCH-Q13")));
        add_driver("q14", new tpch_q14_driver(c_str("TPCH-Q14")));
        add_driver("q16", new tpch_q16_driver(c_str("TPCH-Q16")));
        add_driver("q19", new tpch_q19_driver(c_str("TPCH-Q19")));
//         add_driver("q6pf", new tpch_q6pf_driver(c_str("TPCH-Q6PF")));
//         add_driver("q6pipe", new tpch_q6pipe_driver(c_str("TPCH-Q6PIPE")));
//         add_driver("q1pipe", new tpch_q6pipe_driver(c_str("TPCH-Q1PIPE")));

        // Need to pass a mix driver like m146 a directory... We are
        // the directory since we implement a lookup_driver
        // method. TODO change this so we pass down one of our data
        // fields.
        add_driver("m146", new tpch_m146_driver(c_str("TPCH-MIX-1,4,6"), this));
        add_driver("m_1_4_6_12", new tpch_m_1_4_6_12_driver(c_str("TPCH-MIX 1,4,6,12"), this));
        add_driver("upto13", new tpch_upto13_driver(c_str("TPCH-MIX 1,4,6,12,13"), this));
        add_driver("m14612", new tpch_m14612_driver(c_str("TPCH-MIX-1,4,6,12"), this));
        add_driver("sim_mix", new tpch_sim_mix_driver(c_str("TPCH-SIM-MIX"), this));
        

        // register dispatcher policies...
        add_scheduler_policy("OS",        new scheduler::policy_os_t());
        add_scheduler_policy("RR_CPU",    new scheduler::policy_rr_cpu_t());
        add_scheduler_policy("RR_MODULE", new scheduler::policy_rr_module_t());
        add_scheduler_policy("QUERY_CPU", new scheduler::policy_query_cpu_t());

        tpch_handler = this;
        state = TPCH_HANDLER_INITIALIZED;
    }


    
}



void tpch_handler_t::shutdown() {

    // use a global thread-safe state machine to ensure that
    // db_close() is called exactly once

    critical_section_t cs(state_mutex);

    if ( state == TPCH_HANDLER_INITIALIZED ) {
        
        // close DB tables
        TRACE(TRACE_ALWAYS, "... closing db\n");
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
        for (map<c_str, driver_t*>::iterator it = _drivers.begin(); it != _drivers.end(); ++it)
        {
            TRACE(TRACE_ALWAYS, "%s\n", it->first.data());
        }
        return;
    }
   
    
    if ( sscanf(command, "%*s %*s %d %d %d %s",
                &num_clients,
                &num_iterations,
                &think_time,
                scheduler_policy_tag) < 4 ) {
        char command_tag[SERVER_COMMAND_BUFFER_SIZE];
        int command_found = sscanf(command, "%s", command_tag);
        assert(command_found == 1);
        print_usage(command_tag);
        return;
    }


    // debugging
    TRACE(TRACE_ALWAYS, "num_clients = %d, num_iterations = %d, think_time = %d, scheduler_policy = %s\n",
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
    tuple_fifo::trace_stats();
}



void tpch_handler_t::print_usage(const char* command_tag)
{
    TRACE(TRACE_ALWAYS, "Usage: %s"
          " list | <driver_tag> <num_clients> <num_iterations> <think_time> <scheduler_policy_tag>"
          "\n",
          command_tag);
}


void tpch_handler_t::print_run_statistics(const c_str& desc, workload_t::results_t &results) {

    tuple_fifo::clear_stats();
    
    // print final statistics
    int queries_completed =
        results.num_clients * results.num_iterations;
    
    // queries per hour
    float tpmC = (60.0 * queries_completed) / results.total_time;
    
    TRACE(TRACE_STATISTICS, "~~~\n");
    TRACE(TRACE_STATISTICS, "~~~ Description       = %s\n", desc.data());
    TRACE(TRACE_STATISTICS, "~~~ Clients           = %d \n", results.num_clients);
    TRACE(TRACE_STATISTICS, "~~~ Iterations        = %d \n", results.num_iterations);
    TRACE(TRACE_STATISTICS, "~~~ Think Time        = %d \n", results.think_time);
    TRACE(TRACE_STATISTICS, "~~~ Duration          = %.2f \n", results.total_time);
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



void tpch_handler_t::add_scheduler_policy(const char* tag, scheduler::policy_t* dp) {
    add_scheduler_policy(c_str(tag), dp);
}



void tpch_handler_t::add_scheduler_policy(const c_str &tag, scheduler::policy_t* dp) {
    // make sure no mapping currently exists
    assert( _scheduler_policies.find(tag) == _scheduler_policies.end() );
    _scheduler_policies[tag] = dp;
}



driver_t* tpch_handler_t::lookup_driver(const c_str &tag) {
    return _drivers[tag];
}

