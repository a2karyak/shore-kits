/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_compare.h
 *
 *  @brief Interface for the TPC-C B-trees comparators
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_COMPARE_H
#define __TPCC_COMPARE_H

#include <db_cxx.h>
#include <cstdio>

#include "util/namespace.h"

ENTER_NAMESPACE(tpcc);

// BerkeleyDB comparators for B-tree TPC-C table organization

int tpcc_bt_compare_fn_CUSTOMER(Db* idx, 
                                const Dbt* k1, const Dbt* k2);

int tpcc_bt_compare_fn_DISTRICT(Db* idx, 
                                const Dbt* k1, const Dbt* k2);

int tpcc_bt_compare_fn_HISTORY(Db* idx, 
                               const Dbt* k1, const Dbt* k2);

int tpcc_bt_compare_fn_ITEM(Db* idx, 
                            const Dbt* k1, const Dbt* k2);

int tpcc_bt_compare_fn_NEW_ORDER(Db* idx, 
                                 const Dbt* k1, const Dbt* k2);

int tpcc_bt_compare_fn_ORDER(Db* idx, 
                                const Dbt* k1, const Dbt* k2);

int tpcc_bt_compare_fn_ORDERLINE(Db* idx, 
                                 const Dbt* k1, const Dbt* k2);

int tpcc_bt_compare_fn_STOCK(Db* idx, 
                             const Dbt* k1, const Dbt* k2);

int tpcc_bt_compare_fn_STOCK(Db* idx, 
                             const Dbt* k1, const Dbt* k2);

int tpcc_bt_compare_fn_WAREHOUSE(Db* idx, 
                                 const Dbt* k1, const Dbt* k2);


// (ip) Still, we are not using indexes 
// (ip) To be done!!

/*
// indexes
int tpcc_lineitem_shipdate_compare_fcn(Db* idx, 
                                       const Dbt* k1, const Dbt* k2);
int tpcc_lineitem_shipdate_key_fcn(Db*idx, 
                                   const Dbt* pk, const Dbt* pd, Dbt* sk);

*/
                                   
EXIT_NAMESPACE(tpcc);


#endif
