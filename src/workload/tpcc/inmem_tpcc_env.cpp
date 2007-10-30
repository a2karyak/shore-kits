/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_env.cpp
 *
 *  @brief Declaration of the InMemory TPC-C database data structures
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "workload/tpcc/inmem_tpcc_env.h"

using namespace tpcc;

/* exported data structures */


/** Arrays: WAREHOUSE, DISTRICT */


lathedArray<tpcc_warehouse_tuple, SCALING_FACTOR, WAREHOUSE_FANOUT> im_warehouses;

lathedArray<tpcc_district_tuple, SCALING_FACTOR, DISTRICT_FANOUT> im_districts;


/** BPTrees: CUSTOMER, HISTORY */

BPlusTree<tpcc_customer_tuple_key, tpcc_customer_tuple_body,
          cCustNodeEntries, cCustLeafEntries,
          cCustNodePad, cCustLeafPad, cArch> im_customers;

BPlusTree<tpcc_history_tuple_key, tpcc_history_tuple_body,
          cHistNodeEntries, cHistLeafEntries,
          cHistNodePad, cHistLeafPad, cArch> im_histories;

