/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_compare.cpp
 *
 *  @brief Implementation of the TPC-C B-trees comparators
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include <cstdlib>

#include "workload/tpcc/tpcc_compare.h"
#include "workload/tpcc/tpcc_struct.h"
#include "workload/common/bdb_env.h"

using namespace workload;


ENTER_NAMESPACE(tpcc);


// BerkeleyDB comparators for B-tree TPC-C table organization

int tpcc_bt_compare_fn_CUSTOMER(Db* idx, 
                                const Dbt* k1, const Dbt* k2)
 { TRACE( TRACE_ALWAYS, "Doing Nothing!"); return (-1); }


int tpcc_bt_compare_fn_DISTRICT(Db* idx, 
                                const Dbt* k1, const Dbt* k2)
 { TRACE( TRACE_ALWAYS, "Doing Nothing!"); return (-1); }

int tpcc_bt_compare_fn_HISTORY(Db* idx, 
                               const Dbt* k1, const Dbt* k2)
 { TRACE( TRACE_ALWAYS, "Doing Nothing!"); return (-1); }

int tpcc_bt_compare_fn_ITEM(Db* idx, 
                            const Dbt* k1, const Dbt* k2)
 { TRACE( TRACE_ALWAYS, "Doing Nothing!"); return (-1); }

int tpcc_bt_compare_fn_NEW_ORDER(Db* idx, 
                                 const Dbt* k1, const Dbt* k2)
 { TRACE( TRACE_ALWAYS, "Doing Nothing!"); return (-1); }

int tpcc_bt_compare_fn_ORDER(Db* idx, 
                                const Dbt* k1, const Dbt* k2)
 { TRACE( TRACE_ALWAYS, "Doing Nothing!"); return (-1); }

int tpcc_bt_compare_fn_ORDERLINE(Db* idx, 
                                 const Dbt* k1, const Dbt* k2)
 { TRACE( TRACE_ALWAYS, "Doing Nothing!"); return (-1); }

int tpcc_bt_compare_fn_STOCK(Db* idx, 
                             const Dbt* k1, const Dbt* k2)
 { TRACE( TRACE_ALWAYS, "Doing Nothing!"); return (-1); }

int tpcc_bt_compare_fn_WAREHOUSE(Db* idx, 
                                 const Dbt* k1, const Dbt* k2)
 { TRACE( TRACE_ALWAYS, "Doing Nothing!"); return (-1); }


/*
int tpch_lineitem_shipdate_key_fcn(Db*, const Dbt*, const Dbt* pd, Dbt* sk) {
    sk->set_data(FIELD(pd->get_data(), tpch_lineitem_tuple, L_SHIPDATE));
    sk->set_size(SIZEOF(tpch_lineitem_tuple, L_SHIPDATE));
    return 0;    
}

int tpch_bt_compare_fn_SUPPLIER(Db*, const Dbt* k1, const Dbt* k2) {

    int u1;
    int u2;
    memcpy(&u1,((int*)k1->get_data()), sizeof(int));
    memcpy(&u2,((int*)k2->get_data()), sizeof(int));

    if (u1 < u2)
        return -1;
    else if (u1 == u2)
        return 0;
    else
        return 1;
}

*/

EXIT_NAMESPACE(tpcc);

