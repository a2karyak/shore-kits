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


/** sl_thread_t methods */


/** @fn run
 * 
 *  @brief Runs a shore thread
 */

void sl_thread_t::run() 
{
    if (!shore_env->is_initialized()) {
        if (_env->init()) {
            // Couldn't initialize the Shore environment
            // cannot proceed
            return;
        }
    }


    static int const THREADS = 1;// ShoreTPCCEnv::SHORE_PAYMENT_TABLES;
    guard<shore_parse_thread> threads[THREADS];

    // initialize all table loaders
    threads[ShoreTPCCEnv::WAREHOUSE] = 
        new shore_parser_impl_WAREHOUSE(ShoreTPCCEnv::WAREHOUSE, _env);

    /*
    threads[ShoreTPCCEnv::DISTRICT] = 
        new shore_parser_impl_DISTRICT(ShoreTPCCEnv::DISTRICT, 
                                       ssm.get(), vid, _env);
    threads[ShoreTPCCEnv::CUSTOMER] = 
        new shore_parser_impl_CUSTOMER(ShoreTPCCEnv::CUSTOMER, 
                                       ssm.get(), vid, _env);

    threads[ShoreTPCCEnv::HISTORY] = 
        new shore_parser_impl_HISTORY(ShoreTPCCEnv::HISTORY, 
                                      ssm.get(), vid, _env);
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
	
    // done!
    cout << "Shutting down Shore..." << endl;
    if (shore_env->is_initialized())
        _env->close();
}
