/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_compare.cpp
 *
 *  @brief Implementation of the TPC-C B-trees comparators
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include <cstdlib>

#include "workload/common/bdb_env.h"
#include "workload/common/table_compare.h"

#include "workload/tpcc/tpcc_compare.h"
//#include "workload/tpcc/tpcc_struct.h"


using namespace workload;


ENTER_NAMESPACE(tpcc);


// BerkeleyDB comparators for B-tree TPC-C table organization


// CUSTOMER key is composed of 3 (int) fields: 
// C_C_ID, C_D_ID, C_W_ID
int tpcc_bt_compare_fn_CUSTOMER(Db*, 
                                const Dbt* k1, 
                                const Dbt* k2)
{ 
    return (bt_compare_fn_3_int(k1, k2));
}


// DISTRICT key is composed of 2 (int) fields: 
// D_ID, D_W_ID
int tpcc_bt_compare_fn_DISTRICT(Db*, 
                                const Dbt* k1, 
                                const Dbt* k2)
{ 
    return (bt_compare_fn_2_int(k1, k2));
}


// HISTORY does not have a key, use the whole tuple
// *** (ip) Instead we are using a comparison of the
// ***      first 6 integers
int tpcc_bt_compare_fn_HISTORY(Db*, 
                               const Dbt* k1, 
                               const Dbt* k2)
{ 
    return (bt_compare_fn_6_int(k1, k2));
}


// ITEM key is composed of 1 (int) field: 
// I_ID
int tpcc_bt_compare_fn_ITEM(Db*, 
                            const Dbt* k1, 
                            const Dbt* k2)
{ 
    return (bt_compare_fn_1_int(k1, k2));
}


// NEW_ORDER key is composed of 3 (int) fields: 
// NO_O_ID, NO_D_ID, NO_W_ID
int tpcc_bt_compare_fn_NEW_ORDER(Db*, 
                                 const Dbt* k1, 
                                 const Dbt* k2)
{ 
    return (bt_compare_fn_3_int(k1, k2));
}


// ORDER key is composed of 4 (int) fields: 
// O_ID, O_C_ID, O_D_ID, O_W_ID
int tpcc_bt_compare_fn_ORDER(Db*, 
                             const Dbt* k1, 
                             const Dbt* k2)
{ 
    return (bt_compare_fn_4_int(k1, k2));
}


// ORDERLINE key is composed of 4 (int) fields: 
// OL_O_ID, OL_D_ID, OL_W_ID, OL_NUMBER
int tpcc_bt_compare_fn_ORDERLINE(Db*, 
                                 const Dbt* k1,
                                 const Dbt* k2)
{ 
    return (bt_compare_fn_4_int(k1, k2));
}


// STOCK key is composed of 2 (int) fields: 
// S_I_ID, S_W_ID
int tpcc_bt_compare_fn_STOCK(Db*, 
                             const Dbt* k1,
                             const Dbt* k2)
{ 
    return (bt_compare_fn_2_int(k1, k2));
}


// WAREHOUSE key is composed of 1 (int) field: 
// W_ID
int tpcc_bt_compare_fn_WAREHOUSE(Db*, 
                                 const Dbt* k1, 
                                 const Dbt* k2)
{ 
    return (bt_compare_fn_1_int(k1, k2));
}



EXIT_NAMESPACE(tpcc);

