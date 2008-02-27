/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_handler.cpp
 *
 *  @brief Implementation of handler of SHORE-TPC-C related commands
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "server/command/shore_tpcc_handler.h"

// Shore-TPCC data-structures
#include "sm/shore/shore_tools.h"
#include "stages/tpcc/common/tpcc_scaling_factor.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "workload/tpcc/shore_tpcc_load.h"
#include "sm/shore/shore_helper_loader.h"


// Shore-TPCC drivers header files
// baseline
#include "workload/tpcc/drivers/shore/shore_tpcc_new_order_baseline.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_payment_baseline.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_order_status_baseline.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_delivery_baseline.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_stock_level_baseline.h"
#include "workload/tpcc/drivers/shore/shore_tpcc_mix_baseline.h"

// staged
//#include "workload/tpcc/drivers/shore/shore_tpcc_payment_staged.h"


using namespace tpcc;
using namespace workload;
using qpipe::tuple_fifo;

pthread_mutex_t shore_tpcc_handler_t::state_mutex = PTHREAD_MUTEX_INITIALIZER;

shore_tpcc_handler_t::state_t shore_tpcc_handler_t::state = SHORE_TPCC_HANDLER_UNINITIALIZED;

shore_tpcc_handler_t *shore_tpcc_handler = NULL;


/** @fn init
 *
 *  @brief Initialize SHORE-TPC-C handler. 
 *
 *  @note We must create an instance of the shore_env initialize our global table 
 *  environment. We must ensure that this happens exactly once, 
 *  despite the fact that we may register the same shore_tpcc_handler_t 
 *  instance, or multiple shore_tpcc_handler_t instances for different 
 *  command tags.
 */

void shore_tpcc_handler_t::init() {

    // use a global thread-safe state machine to ensure that db_open()
    // is called exactly once

    critical_section_t cs(state_mutex);

    if ( state == SHORE_TPCC_HANDLER_UNINITIALIZED ) {

        // Create the Shore environment instance
        // @note Once the user decide to load the data the system 
        // will configure and start
        shore_env = new ShoreTPCCEnv(SHORE_DEFAULT_CONF_FILE);

        // the initialization must be executed in a shore context
        db_init_smt_t* initializer = new db_init_smt_t(c_str("init"), shore_env);
        initializer->fork();
        initializer->join();        
        if (initializer) {
            delete (initializer);
            initializer = NULL;
        }


        // register drivers...
        add_driver("shore_new_order_baseline", 
                   new shore_tpcc_new_order_baseline_driver(c_str("SHORE_NEW_ORDER_BASELINE")));
        add_driver("shore_payment_baseline", 
                   new shore_tpcc_payment_baseline_driver(c_str("SHORE_PAYMENT_BASELINE")));
        add_driver("shore_order_status_baseline", 
                   new shore_tpcc_order_status_baseline_driver(c_str("SHORE_ORDER_STATUS_BASELINE")));
        add_driver("shore_delivery_baseline", 
                   new shore_tpcc_delivery_baseline_driver(c_str("SHORE_DELIVERY_BASELINE")));
        add_driver("shore_stock_level_baseline", 
                   new shore_tpcc_stock_level_baseline_driver(c_str("SHORE_STOCK_LEVEL_BASELINE")));
        add_driver("shore_mix_baseline", 
                   new shore_tpcc_mix_baseline_driver(c_str("SHORE_MIX_BASELINE")));

        // register dispatcher policies...
        add_scheduler_policy("OS",        new scheduler::policy_os_t());
        add_scheduler_policy("RR_CPU",    new scheduler::policy_rr_cpu_t());
        add_scheduler_policy("RR_MODULE", new scheduler::policy_rr_module_t());
        add_scheduler_policy("QUERY_CPU", new scheduler::policy_query_cpu_t());

        shore_tpcc_handler = this;
        state = SHORE_TPCC_HANDLER_INITIALIZED;
    }    
}


/*********************************************************************
 *
 *  @fn:    shutdown
 *
 *  @brief: Do the appropriate shutdown
 *
 *********************************************************************/

void shore_tpcc_handler_t::shutdown() {

    assert(shore_env);

    // use a global thread-safe state machine to ensure that
    // the db close function is called exactly once

    CRITICAL_SECTION(cs, state_mutex);

    if ( state == SHORE_TPCC_HANDLER_INITIALIZED ) {
        
        // close Storage manager
        TRACE(TRACE_ALWAYS, "... closing db\n");

        // close Shore env
        close_smt_t* clt = new close_smt_t(shore_env, c_str("clt"));
        clt->fork();
        clt->join();
        if (clt->_rv) {
            TRACE( TRACE_ALWAYS, "*** Error in closing thread... ***\n");
        }

        if (clt) {
            delete (clt);
            clt = NULL;
        }
        
        state = SHORE_TPCC_HANDLER_SHUTDOWN;
    }
}



void shore_tpcc_handler_t::handle_command(const char* command) {

    assert(shore_env);

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

    // 'scaling' tag sets the scaling factor of warehouses
    if (!strcmp(driver_tag, "scaling")) {
        int num_warehouses = 0;
        sscanf(command, "%*s %*s %d", &num_warehouses);
        if (num_warehouses>0) {
            shore_env->set_sf(num_warehouses);
        }
        TRACE( TRACE_ALWAYS, "Scaling Factor = (%d)\n", shore_env->get_sf());
        TRACE( TRACE_ALWAYS, "Queried Factor = (%d)\n", shore_env->get_qf());
        return;
    }


    // 'queried' tag sets the number of queried warehouses
    if (!strcmp(driver_tag, "queried")) {
        int queried_warehouses = 0;
        sscanf(command, "%*s %*s %d", &queried_warehouses);
        if (queried_warehouses>0) {
            selectedQueriedSF = queried_warehouses;
            shore_env->set_qf(queried_warehouses);
        }
        TRACE( TRACE_ALWAYS, "Scaling Factor = (%d)\n", shore_env->get_sf());
        TRACE( TRACE_ALWAYS, "Queried Factor = (%d)\n", shore_env->get_qf());
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


    // Load data
    if( !strcmp(driver_tag, "parse")) {
        // Load data to the Shore Database
        // This shold be done within a Shore context

        db_load_smt_t* loader = new db_load_smt_t(c_str("loader"), shore_env);
        loader->fork();
        loader->join();
        if (loader->_rc) {
            TRACE( TRACE_ALWAYS, "Problem loading the database\n", 
                   loader->_rc.err_num());
            assert (false);
        }

        if (loader) {
            delete (loader);
            loader = NULL;
        }
        return;
    }


    // Continue only if data loaded
    if(!shore_env->is_loaded()) {
	TRACE(TRACE_ALWAYS, "No database loaded. Please use 'parse' to load one\n");
	return;
    }

    // 'dump' tag handled differently from all others...
    if (!strcmp(driver_tag, "dump"))
    {
        dump_smt_t* dumper = new dump_smt_t(shore_env, c_str("dumper"));
        dumper->fork();
        dumper->join();
        if (dumper->retval()) {
            TRACE( TRACE_ALWAYS, "Problem dumping the database\n");
        }

        if (dumper) {
            delete (dumper);
            dumper = NULL;
        }
        return;
    }

    // 'du' tag handled differently from all others...
    if (!strcmp(driver_tag, "du"))
    {
        assert (false); // (ip) untested
        cout << "Getting stats..." << endl;
        int* r=NULL;
        du_smt_t* duer = new du_smt_t(shore_env, c_str("duer"));
        run_smthread<du_smt_t,int>(duer,r);
        delete (duer);
        duer = NULL;
        return;
    }



    // Parses new run data
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

    /* Print shore_env stats */
    assert (shore_env);
    shore_env->print_tpcc_stats();

    /* Report results. We'll use the workload name for its
       description. */
    print_run_statistics(workload_name, results);
}




void shore_tpcc_handler_t::print_usage(const char* command_tag)
{
    TRACE(TRACE_ALWAYS, "Usage: %s"
          " list | <driver_tag> <num_clients> <num_iterations> <think_time> <scheduler_policy_tag>"
          "\n",
          command_tag);
}


void shore_tpcc_handler_t::print_run_statistics(const c_str& desc, 
					  workload_t::results_t &results) 
{
    tuple_fifo::clear_stats();
    
    // print final statistics
    int trx_completed = results.num_clients * results.num_iterations;
    
    // queries per sec
    float tpsC =  trx_completed / results.total_time;

    // queries per min
    //float tpmC =  (60 * trx_completed) / results.total_time;
    
    TRACE(TRACE_STATISTICS, "~~~\n");
    TRACE(TRACE_STATISTICS, "~~~ Description       = %s\n", desc.data());
    TRACE(TRACE_STATISTICS, "~~~ Clients           = %d \n", results.num_clients);
    TRACE(TRACE_STATISTICS, "~~~ Iterations        = %d \n", results.num_iterations);
    TRACE(TRACE_STATISTICS, "~~~ Think Time        = %d \n", results.think_time);
    TRACE(TRACE_STATISTICS, "~~~ Duration          = %.2f \n", results.total_time);
    TRACE(TRACE_STATISTICS, "~~~\n");
    TRACE(TRACE_STATISTICS, "~~~ Throughput        = %.2f trx/sec\n", tpsC);
    //    TRACE(TRACE_STATISTICS, "~~~ Throughput        = %.2f trx/min\n", tpsC);
    TRACE(TRACE_STATISTICS, "~~~\n");
}



void shore_tpcc_handler_t::add_driver(const char* tag, driver_t* driver) {

    add_driver(c_str(tag), driver);
}



void shore_tpcc_handler_t::add_driver(const c_str &tag, driver_t* driver) {

    // make sure no mapping currently exists
    assert( _drivers.find(tag) == _drivers.end() );
    _drivers[tag] = driver;
}



void shore_tpcc_handler_t::add_scheduler_policy(const char* tag, 
					  scheduler::policy_t* dp) 
{
    add_scheduler_policy(c_str(tag), dp);
}



void shore_tpcc_handler_t::add_scheduler_policy(const c_str &tag, 
					  scheduler::policy_t* dp) 
{
    // make sure no mapping currently exists
    assert( _scheduler_policies.find(tag) == _scheduler_policies.end() );
    _scheduler_policies[tag] = dp;
}



driver_t* shore_tpcc_handler_t::lookup_driver(const c_str &tag) {
    return _drivers[tag];
}

