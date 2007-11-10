/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_env.cpp
 *
 *  @brief Declaration of the InMemory TPC-C environment
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "workload/workload.h"
#include "workload/tpcc/inmem_tpcc_load.h"
#include "workload/tpcc/inmem_tpcc_env.h"

using namespace workload;
using namespace tpcc;


/** Exported functions */


/** @fn savedata
 *
 *  @brief Loads the TPC-C data from a specified location
 */

int InMemTPCCEnv::savedata(c_str saveDir) {

    TRACE( TRACE_DEBUG, "Saving data to (%s)\n", saveDir.data());

    array_guard_t<pthread_t> inmem_save_ids = new pthread_t[INMEM_PAYMENT_TABLES];

    //////////////////////////
    // DEFINE WAREHOUSE SAVER
    //////////////////////////


    typedef save_table<warehouse_array_t> warehouse_inmem_saver;

    warehouse_inmem_saver* wh_saver_thread = 
        new warehouse_inmem_saver(im_warehouses, c_str("%s%s", saveDir.data(), INMEM_TPCC_SAVE_WAREHOUSE));



    /////////////////////////
    // DEFINE DISTRICT SAVER
    /////////////////////////
    
    typedef save_table<district_array_t> district_inmem_saver;

    district_inmem_saver* distr_saver_thread = 
        new district_inmem_saver(im_districts, c_str("%s%s", saveDir.data(), INMEM_TPCC_SAVE_DISTRICT));



    /////////////////////////
    // DEFINE CUSTOMER SAVER
    /////////////////////////

    typedef save_table<customer_tree_t> customer_inmem_saver;

    customer_inmem_saver* customer_saver_thread = 
        new customer_inmem_saver(im_customers, c_str("%s%s", saveDir.data(), INMEM_TPCC_SAVE_CUSTOMER));



    ////////////////////////
    // DEFINE HISTORY SAVER
    ////////////////////////

    typedef save_table<history_tree_t> history_inmem_saver;

    history_inmem_saver* history_saver_thread = 
        new history_inmem_saver(im_histories, c_str("%s%s", saveDir.data(), INMEM_TPCC_SAVE_HISTORY));


    // Start measuring loading time
    time_t tstart = time(NULL);


    try {

        /////////////////////////
        // START WAREHOUSE SAVER
        /////////////////////////
        
        inmem_save_ids[WAREHOUSE] = thread_create(wh_saver_thread);



        ////////////////////////
        // START DISTRICT SAVER
        ////////////////////////

        inmem_save_ids[DISTRICT] = thread_create(distr_saver_thread);



        ////////////////////////
        // START CUSTOMER SAVER
        ////////////////////////

        inmem_save_ids[CUSTOMER] = thread_create(customer_saver_thread);



        ////////////////////////
        // START CUSTOMER SAVER
        ////////////////////////

        inmem_save_ids[HISTORY] = thread_create(history_saver_thread);

    }
    catch (TrxException& e) {

        TRACE( TRACE_ALWAYS, "Exception thrown in InMem Saving...\n%s", e.what());
        throw e;
    }
                                                                    
    workload_t::wait_for_clients(inmem_save_ids, INMEM_PAYMENT_TABLES);

    time_t tstop = time(NULL);

    TRACE( TRACE_ALWAYS, "Save finished in (%d) secs\n", tstop - tstart);

    return (0);
}

/** @fn restoredata
 *
 *  @brief Loads the TPC-C data from a specified location
 */

int InMemTPCCEnv::restoredata(c_str restoreDir) {

    TRACE( TRACE_DEBUG, "Saving data to (%s)\n", restoreDir.data());

    array_guard_t<pthread_t> inmem_restore_ids = new pthread_t[INMEM_PAYMENT_TABLES];

    //////////////////////////
    // DEFINE WAREHOUSE RESTORER
    //////////////////////////


    typedef restore_table<warehouse_array_t> warehouse_inmem_restorer;

    warehouse_inmem_restorer* wh_restorer_thread = 
        new warehouse_inmem_restorer(im_warehouses, c_str("%s%s", restoreDir.data(), INMEM_TPCC_SAVE_WAREHOUSE));



    /////////////////////////
    // DEFINE DISTRICT RESTORER
    /////////////////////////
    
    typedef restore_table<district_array_t> district_inmem_restorer;

    district_inmem_restorer* distr_restorer_thread = 
        new district_inmem_restorer(im_districts, c_str("%s%s", restoreDir.data(), INMEM_TPCC_SAVE_DISTRICT));



    /////////////////////////
    // DEFINE CUSTOMER RESTORER
    /////////////////////////

    typedef restore_table<customer_tree_t> customer_inmem_restorer;

    customer_inmem_restorer* customer_restorer_thread = 
        new customer_inmem_restorer(im_customers, c_str("%s%s", restoreDir.data(), INMEM_TPCC_SAVE_CUSTOMER));



    ////////////////////////
    // DEFINE HISTORY RESTORER
    ////////////////////////

    typedef restore_table<history_tree_t> history_inmem_restorer;

    history_inmem_restorer* history_restorer_thread = 
        new history_inmem_restorer(im_histories, c_str("%s%s", restoreDir.data(), INMEM_TPCC_SAVE_HISTORY));


    // Start measuring loading time
    time_t tstart = time(NULL);


    try {

        /////////////////////////
        // START WAREHOUSE RESTORER
        /////////////////////////
        
        inmem_restore_ids[WAREHOUSE] = thread_create(wh_restorer_thread);



        ////////////////////////
        // START DISTRICT RESTORER
        ////////////////////////

        inmem_restore_ids[DISTRICT] = thread_create(distr_restorer_thread);



        ////////////////////////
        // START CUSTOMER RESTORER
        ////////////////////////

        inmem_restore_ids[CUSTOMER] = thread_create(customer_restorer_thread);



        ////////////////////////
        // START CUSTOMER RESTORER
        ////////////////////////

        inmem_restore_ids[HISTORY] = thread_create(history_restorer_thread);

    }
    catch (TrxException& e) {

        TRACE( TRACE_ALWAYS, "Exception thrown in InMem Saving...\n%s", e.what());
        throw e;
    }
                                                                    
    workload_t::wait_for_clients(inmem_restore_ids, INMEM_PAYMENT_TABLES);

    time_t tstop = time(NULL);

    TRACE( TRACE_ALWAYS, "Restore finished in (%d) secs\n", tstop - tstart);
    _initialized = true;
    return (0);
}


/** @fn loaddata
 *
 *  @brief Loads the TPC-C data from a specified location
 */

int InMemTPCCEnv::loaddata(c_str loadDir) {

    TRACE( TRACE_DEBUG, "Getting data from (%s)\n", loadDir.data());

    array_guard_t<pthread_t> inmem_loader_ids = new pthread_t[INMEM_PAYMENT_TABLES];

    //////////////////////////
    // DEFINE WAREHOUSE LOADER
    //////////////////////////


    typedef inmem_array_loader_impl<warehouse_array_t, parse_tpcc_WAREHOUSE> warehouse_inmem_loader;

    warehouse_inmem_loader* wh_loader_thread = 
        new warehouse_inmem_loader(c_str("WH-LOADER"),
                                   c_str("%s/%s", loadDir.data(), INMEM_TPCC_DATA_WAREHOUSE),
                                   &im_warehouses);



    /////////////////////////
    // DEFINE DISTRICT LOADER
    /////////////////////////
    
    typedef inmem_array_loader_impl<district_array_t, parse_tpcc_DISTRICT> district_inmem_loader;
    
    district_inmem_loader* distr_loader_thread = 
        new district_inmem_loader(c_str("DISTR-LOADER"),
                                  c_str("%s/%s", loadDir.data(), INMEM_TPCC_DATA_DISTRICT),
                                  &im_districts);


    /////////////////////////
    // DEFINE CUSTOMER LOADER
    /////////////////////////

    typedef inmem_bptree_loader_impl<customer_tree_t, parse_tpcc_CUSTOMER> customer_inmem_loader;

    customer_inmem_loader* customer_loader_thread = 
        new customer_inmem_loader(c_str("CUST-LOADER"),
                                  c_str("%s/%s", loadDir.data(), INMEM_TPCC_DATA_CUSTOMER),
                                  &im_customers);    


    ////////////////////////
    // DEFINE HISTORY LOADER
    ////////////////////////

    typedef inmem_bptree_loader_impl<history_tree_t, parse_tpcc_HISTORY> history_inmem_loader;

    history_inmem_loader* history_loader_thread = 
        new history_inmem_loader(c_str("HIST-LOADER"),
                                 c_str("%s/%s", loadDir.data(), INMEM_TPCC_DATA_HISTORY),
                                 &im_histories);


    // Start measuring loading time
    time_t tstart = time(NULL);


    try {

        /////////////////////////
        // START WAREHOUSE LOADER
        /////////////////////////
        
        inmem_loader_ids[WAREHOUSE] = thread_create(wh_loader_thread);



        ////////////////////////
        // START DISTRICT LOADER
        ////////////////////////

        inmem_loader_ids[DISTRICT] = thread_create(distr_loader_thread);



        ////////////////////////
        // START CUSTOMER LOADER
        ////////////////////////

        inmem_loader_ids[CUSTOMER] = thread_create(customer_loader_thread);



        ////////////////////////
        // START CUSTOMER LOADER
        ////////////////////////

        inmem_loader_ids[HISTORY] = thread_create(history_loader_thread);

    }
    catch (TrxException& e) {

        TRACE( TRACE_ALWAYS, "Exception thrown in InMem Loading...\n%s", e.what());
        throw e;
    }
                                                                    
    workload_t::wait_for_clients(inmem_loader_ids, INMEM_PAYMENT_TABLES);

    time_t tstop = time(NULL);

    TRACE( TRACE_ALWAYS, "Loading finished in (%d) secs\n", tstop - tstart);
    _initialized = true;
    return (0);
}



/** @fn dump
 *
 *  @brief Dumps the data
 */

void InMemTPCCEnv::dump() {
    TRACE( TRACE_DEBUG, "~~~~~~~~~~~~~~~~~~~~~\n");    
    TRACE( TRACE_DEBUG, "Dumping InMemory Data\n");
    
    TRACE( TRACE_DEBUG, "Dumping (im_warehouses)\n");
    im_warehouses.dump();

    TRACE( TRACE_DEBUG, "Dumping (im_districts)\n");
    im_districts.dump();

    TRACE( TRACE_DEBUG, "Dumping (im_customers)\n");        
    im_customers.dump();

    TRACE( TRACE_DEBUG, "Dumping (im_histories)\n");
    im_histories.dump();

    TRACE( TRACE_DEBUG, "~~~~~~~~~~~~~~~~~~~~~\n");    
}
