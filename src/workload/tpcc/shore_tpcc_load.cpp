/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_load.h
 *
 *  @brief Definition of the Shore TPC-C loaders
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "workload/tpcc/shore_tpcc_load.h"


using namespace tpcc;


/******************** loading_smthread_t methods ******************/


/** @fn run
 * 
 *  @brief Loads the Shore data if not loaded already. If the environment
 *  not initiliazed, it also initializes it.
 */

void loading_smt_t::run() 
{
    if (!_env->is_initialized()) {
        if (_env->init()) {
            // Couldn't initialize the Shore environment
            // cannot proceed
            cout << "Couldn't initialize Shore " << endl;
            _rv = 1;
            return;
        }
    }

    // load data
    _rv = loaddata();
}



/** @fn loaddata
 *
 *  @brief Loads the TPC-C data from a specified location
 *
 *  @return 0 on sucess, non-zero otherwise
 */

int loading_smt_t::loaddata() 
{
    CRITICAL_SECTION(cs, *_env->get_load_mutex());

    if (_env->is_loaded_no_cs()) {
        cerr << "Shore data already loaded... " << endl;
        return (1);
    }

    //    TRACE( TRACE_DEBUG, "Getting data from (%s)\n", loadDir.data());

    // Start measuring loading time
    time_t tstart = time(NULL);

    static int const THREADS = SHORE_PAYMENT_TABLES;
    //static int const THREADS = ShoreTPCCEnv::SHORE_TPCC_TABLES;
    guard<shore_parse_thread> threads[THREADS];

    // initialize all table loaders
    threads[TBL_ID_WAREHOUSE] = 
        new shore_parser_impl_WAREHOUSE(TBL_ID_WAREHOUSE, _env);

    threads[TBL_ID_DISTRICT] = 
        new shore_parser_impl_DISTRICT(TBL_ID_DISTRICT, _env);

    threads[TBL_ID_CUSTOMER] = 
        new shore_parser_impl_CUSTOMER(TBL_ID_CUSTOMER, _env);

    threads[TBL_ID_HISTORY] = 
        new shore_parser_impl_HISTORY(TBL_ID_HISTORY, _env);

    /**
    threads[TBL_ID_ITEM] = 
        new shore_parser_impl_ITEM(TBL_ID_ITEM, _env);

    threads[TBL_ID_NEW_ORDER] = 
        new shore_parser_impl_NEW_ORDER(TBL_ID_NEW_ORDER, _env);

    threads[TBL_ID_ORDER] = 
        new shore_parser_impl_ORDER(TBL_ID_ORDER, _env);

    threads[TBL_ID_ORDERLINE] = 
        new shore_parser_impl_ORDERLINE(TBL_ID_ORDERLINE, _env);

    threads[TBL_ID_STOCK] = 
        new shore_parser_impl_STOCK(TBL_ID_STOCK, _env);
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



/******************** closing_smt_t methods ******************/


/** @fn run
 * 
 *  @brief Closes the Shore database
 */

void closing_smt_t::run() {
    assert(_env);
    cout << "Shutting down Shore..." << endl;
    if (_env && _env->is_initialized())            
        _env->close();
}



/******************** du_smt_t methods ******************/


/** @fn run
 * 
 *  @brief Prints sm volume statistics
 */

void du_smt_t::run() {
    assert(_env);
    _env->statistics();
}
