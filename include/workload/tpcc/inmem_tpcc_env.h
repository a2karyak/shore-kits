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

#define INMEM_TPCC_DATA_DIR "tpcc_sf"

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


    /** Private functions */

    int loaddata(c_str loadDir);


public:

    /** Member variables */

    /* Arrays: WAREHOUSE, DISTRICT */

    lathedArray<tpcc_warehouse_tuple, 
                SCALING_FACTOR, 
                WAREHOUSE_FANOUT> im_warehouses;

    lathedArray<tpcc_district_tuple, 
                SCALING_FACTOR, 
                DISTRICT_FANOUT> im_districts;

    /* BPTrees: CUSTOMER, HISTORY */

    BPlusTree<tpcc_customer_tuple_key, tpcc_customer_tuple_body,
              cCustNodeEntries, cCustLeafEntries,
              cCustNodePad, cCustLeafPad, cArch> im_customers;

    BPlusTree<tpcc_history_tuple_key, tpcc_history_tuple_body,
              cHistNodeEntries, cHistLeafEntries,
              cHistNodePad, cHistLeafPad, cArch> im_histories;



    /** Construction  */

    InMemTPCCEnv() {
        // Loads the data from the INMEM_TPCC_DATA_DIR folder
        loaddata(c_str(INMEM_TPCC_DATA_DIR));
    }


    ~InMemTPCCEnv() { }


}; // EOF InMemTPCCEnv


EXIT_NAMESPACE(tpcc);


#endif
