/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_env.h
 *
 *  @brief Definition of the InMemory TPC-C environment
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __INMEM_TPCC_ENV_H
#define __INMEM_TPCC_ENV_H

#include "util/guard.h"
#include "util/namespace.h"
#include "stages/tpcc/common/tpcc_struct.h"

// For the in-memory data structures
#include "util/latchedarray.h"
#include "util/bptree.h"
#include <boost/array.hpp>


ENTER_NAMESPACE(tpcc);

/* constants */

// (ip) In Lomond the data files and directory have different name
// just a quick hack to run it in both
#ifdef __SUNPRO_CC
#define INMEM_TPCC_DATA_DIR "tbl_tpcc"
#define INMEM_TPCC_DATA_WAREHOUSE "warehouse.dat"
#define INMEM_TPCC_DATA_DISTRICT "district.dat"
#define INMEM_TPCC_DATA_CUSTOMER "customer.dat"
#define INMEM_TPCC_DATA_HISTORY "history.dat"
#else
#define INMEM_TPCC_DATA_DIR "tpcc_sf"
#define INMEM_TPCC_DATA_WAREHOUSE "WAREHOUSE.dat"
#define INMEM_TPCC_DATA_DISTRICT "DISTRICT.dat"
#define INMEM_TPCC_DATA_CUSTOMER "CUSTOMER.dat"
#define INMEM_TPCC_DATA_HISTORY "HISTORY.dat"
#endif


#define SCALING_FACTOR 1
#define WAREHOUSE_FANOUT 1
#define DISTRICT_FANOUT 10

#define cCustNodeEntries 10
#define cCustNodePad 4
#define cCustLeafEntries 20
#define cCustLeafPad 4

#define cHistNodeEntries 50
#define cHistNodePad 8
#define cHistLeafEntries 100
#define cHistLeafPad 8

#define cArch 64


////////////////////////////////////////////////////////////////////////

/** @class InMemTPCCEnv
 *  
 *  @brief Class that contains the various data structures used in the
 *  In-Memory TPC-C environment.
 */

class InMemTPCCEnv 
{
private:    

    static const int WAREHOUSE = 0;
    static const int DISTRICT = 1;
    static const int CUSTOMER = 2;
    static const int HISTORY = 3;
    static const int INMEM_PAYMENT_TABLES = 4;

    /** Private functions */

    int loaddata(c_str loadDir);


public:

    /** Member variables */

    /* Arrays: WAREHOUSE, DISTRICT */

    typedef latchedArray<tpcc_warehouse_tuple, SCALING_FACTOR, WAREHOUSE_FANOUT> warehouse_array_t;
    warehouse_array_t im_warehouses;

    typedef latchedArray<tpcc_district_tuple, SCALING_FACTOR, DISTRICT_FANOUT> district_array_t;
    district_array_t im_districts;

    /* BPTrees: CUSTOMER, HISTORY */
    typedef BPlusTree<tpcc_customer_tuple_key, tpcc_customer_tuple_body,
                      cCustNodeEntries, cCustLeafEntries,
                      cCustNodePad, cCustLeafPad, cArch> customer_tree_t;
    customer_tree_t im_customers;

    typedef BPlusTree<tpcc_history_tuple_key, tpcc_history_tuple_body,
                      cHistNodeEntries, cHistLeafEntries,
                      cHistNodePad, cHistLeafPad, cArch> history_tree_t;
    history_tree_t im_histories;



    /** Construction  */

    InMemTPCCEnv() {
        // Loads the data from the INMEM_TPCC_DATA_DIR folder
        loaddata(c_str(INMEM_TPCC_DATA_DIR));
    }


    ~InMemTPCCEnv() { }


}; // EOF InMemTPCCEnv


EXIT_NAMESPACE(tpcc);


#endif
