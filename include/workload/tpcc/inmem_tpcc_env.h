/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_env.h
 *
 *  @brief Definitions useful for the TPC-C database
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __INMEM_TPCC_ENV_H
#define __INMEM_TPCC_ENV_H

#include "util/namespace.h"
#include "stages/tpcc/common/tpcc_struct.h"

// For the in-memory data structures
#include "util/latchedarray.h"
#include "util/bptree.h"
#include <boost/array.hpp>


ENTER_NAMESPACE(tpcc);

/* constants */

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


/* pointers to B+ tree and Arrays */

/* Arrays: WAREHOUSE, DISTRICT */

extern lathedArray<tpcc_warehouse_tuple, SCALING_FACTOR, WAREHOUSE_FANOUT> im_warehouses;

extern lathedArray<tpcc_district_tuple, SCALING_FACTOR, DISTRICT_FANOUT> im_districts;



/* BPTrees: CUSTOMER, HISTORY */
extern BPlusTree<tpcc_customer_tuple_key, tpcc_customer_tuple_body,
                 cCustNodeEntries, cCustLeafEntries,
                 cCustNodePad, cCustLeafPad, cArch> im_customers;

extern BPlusTree<tpcc_history_tuple_key, tpcc_history_tuple_body,
                 cHistNodeEntries, cHistLeafEntries,
                 cHistNodePad, cHistLeafPad, cArch> im_histories;


EXIT_NAMESPACE(tpcc);


#endif
