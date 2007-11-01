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

int InMemTPCCEnv::loaddata(c_str loadDir) {

    TRACE( TRACE_DEBUG, "Getting data from (%s)\n", loadDir.data());

    // Start measuring loading time
    time_t tstart = time(NULL);

    array_guard_t<pthread_t> inmem_loader_ids = new pthread_t[INMEM_PAYMENT_TABLES];

    try {
        // WAREHOUSE LOADER
        typedef inmem_loader_impl<warehouse_array_t, parse_tpcc_WAREHOUSE> warehouse_inmem_loader;

        guard<warehouse_inmem_loader> wh_loader_thread = 
            new warehouse_inmem_loader(c_str("WH-LOADER"),
                                       c_str("%s/%s", loadDir.data(), INMEM_TPCC_DATA_WAREHOUSE),
                                       &im_warehouses);
        
        //        inmem_loader_ids[WAREHOUSE] = thread_create(wh_loader_thread);
        wh_loader_thread->run();

        // DISTRICT
        typedef inmem_loader_impl<district_array_t, parse_tpcc_DISTRICT> district_inmem_loader;

        guard<district_inmem_loader> distr_loader_thread = 
            new district_inmem_loader(c_str("DISTR-LOADER"),
                                      c_str("%s/%s", loadDir.data(), INMEM_TPCC_DATA_DISTRICT),
                                      &im_districts);

        //        inmem_loader_ids[DISTRICT] = thread_create(distr_loader_thread);
        distr_loader_thread->run();


        // (ip) Tree loaders 
    }
    catch (TrxException& e) {

        TRACE( TRACE_ALWAYS, "Exception thrown in InMem Loading...\n%s", e.what());
        throw e;
    }
                                                                    
    //    workload_t::wait_for_clients(inmem_loader_ids, INMEM_PAYMENT_TABLES);

    time_t tstop = time(NULL);

    TRACE( TRACE_ALWAYS, "Loading finished in (%d) secs\n", tstop - tstart);

    return (0);
}

