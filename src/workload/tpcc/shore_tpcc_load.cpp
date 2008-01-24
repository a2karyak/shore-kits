/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_load.h
 *
 *  @brief Definition of the Shore TPC-C loaders
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "workload/tpcc/shore_tpcc_env.h"
#include "workload/tpcc/shore_tpcc_load.h"


using namespace tpcc;


/******************** loading_smthread_t methods ******************/


/** @fn run
 * 
 *  @brief Loads the Shore data if not loaded already. If the environment
 *  not initiliazed, it also initializes it.
 */

void loading_smthread_t::run() 
{
    if (!_env->is_initialized()) {
        if (_env->init()) {
            // Couldn't initialize the Shore environment
            // cannot proceed
            cout << "Couldn't initialize Shore " << endl;
            retval = 1;
            return;
        }
    }

    // load data
    retval = loaddata();
}



/** @fn loaddata
 *
 *  @brief Loads the TPC-C data from a specified location
 *
 *  @return 0 on sucess, non-zero otherwise
 */

int loading_smthread_t::loaddata() 
{
    CRITICAL_SECTION(cs, *_env->get_load_mutex());

    if (_env->is_loaded_no_cs()) {
        cerr << "Shore data already loaded... " << endl;
        return (1);
    }

    //    TRACE( TRACE_DEBUG, "Getting data from (%s)\n", loadDir.data());

    // Start measuring loading time
    time_t tstart = time(NULL);

    static int const THREADS = ShoreTPCCEnv::SHORE_PAYMENT_TABLES;
    //static int const THREADS = ShoreTPCCEnv::SHORE_TPCC_TABLES;
    guard<shore_parse_thread> threads[THREADS];

    // initialize all table loaders
    threads[ShoreTPCCEnv::WAREHOUSE] = 
        new shore_parser_impl_WAREHOUSE(ShoreTPCCEnv::WAREHOUSE, _env);

    threads[ShoreTPCCEnv::DISTRICT] = 
        new shore_parser_impl_DISTRICT(ShoreTPCCEnv::DISTRICT, _env);

    threads[ShoreTPCCEnv::CUSTOMER] = 
        new shore_parser_impl_CUSTOMER(ShoreTPCCEnv::CUSTOMER, _env);

    threads[ShoreTPCCEnv::HISTORY] = 
        new shore_parser_impl_HISTORY(ShoreTPCCEnv::HISTORY, _env);

    /**
    threads[ShoreTPCCEnv::ITEM] = 
        new shore_parser_impl_ITEM(ShoreTPCCEnv::ITEM, _env);

    threads[ShoreTPCCEnv::NEW_ORDER] = 
        new shore_parser_impl_NEW_ORDER(ShoreTPCCEnv::NEW_ORDER, _env);

    threads[ShoreTPCCEnv::ORDER] = 
        new shore_parser_impl_ORDER(ShoreTPCCEnv::ORDER, _env);

    threads[ShoreTPCCEnv::ORDERLINE] = 
        new shore_parser_impl_ORDERLINE(ShoreTPCCEnv::ORDERLINE, _env);

    threads[ShoreTPCCEnv::STOCK] = 
        new shore_parser_impl_STOCK(ShoreTPCCEnv::STOCK, _env);
    */


    // start each table loader
    for(int i=0; i < THREADS; i++) {
        cout << "Starting loader..." << endl;
	threads[i]->fork();
    }

    for(int i=0; i < THREADS; i++) {
	threads[i]->join();
	//stats += threads[i]->_stats;
    }    

    time_t tstop = time(NULL);

    //    TRACE( TRACE_ALWAYS, "Loading finished in (%d) secs\n", tstop - tstart);
    cout << "Loading finished in " << (tstop - tstart) << " secs." << endl;
    cout << THREADS << " tables loaded..." << endl;
    _env->set_loaded_no_cs(true);
    return (0);
}



/******************** closing_smthread_t methods ******************/


/** @fn run
 * 
 *  @brief Closes the Shore database
 */

void closing_smthread_t::run() {
    assert(_env);
    cout << "Shutting down Shore..." << endl;
    if (_env && _env->is_initialized())            
        _env->close();
}
