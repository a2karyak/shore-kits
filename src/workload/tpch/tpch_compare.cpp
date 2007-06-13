/* -*- mode:C++; c-basic-offset:4 -*- */

#include <cstdlib>

#include "workload/common/table_compare.h"

#include "workload/tpch/tpch_compare.h"
#include "workload/tpch/tpch_struct.h"



using namespace workload;


ENTER_NAMESPACE(tpch);


int tpch_lineitem_shipdate_key_fcn(Db*, const Dbt*, const Dbt* pd, Dbt* sk) {
    sk->set_data(FIELD(pd->get_data(), tpch_lineitem_tuple, L_SHIPDATE));
    sk->set_size(SIZEOF(tpch_lineitem_tuple, L_SHIPDATE));
    return 0;    
}

int tpch_lineitem_shipdate_compare_fcn(Db*, const Dbt* k1, const Dbt* k2) {
    // extract the fields and compare
    TYPEOF(tpch_lineitem_tuple, L_SHIPDATE) t1, t2;
    memcpy(&t1, k1->get_data(), SIZEOF(tpch_lineitem_tuple, L_SHIPDATE));
    memcpy(&t2, k2->get_data(), SIZEOF(tpch_lineitem_tuple, L_SHIPDATE));
    return t1 - t2;
}



int tpch_bt_compare_fn_CUSTOMER(Db*, const Dbt* k1, const Dbt* k2) {

    return (bt_compare_fn_1_int(k1, k2));
}



int tpch_bt_compare_fn_LINEITEM(Db*, const Dbt* k1, const Dbt* k2) {
    
    struct {
        TYPEOF(tpch_lineitem_tuple, L_ORDERKEY) ok;
        TYPEOF(tpch_lineitem_tuple, L_LINENUMBER) ln;
    } key1, key2;

    memcpy(&key1, k1->get_data(), sizeof(key1));
    memcpy(&key2, k2->get_data(), sizeof(key2));

    TYPEOF(tpch_lineitem_tuple, L_ORDERKEY) ok_diff = key1.ok - key2.ok;
    if(ok_diff != 0)
        return ok_diff;

    TYPEOF(tpch_lineitem_tuple, L_LINENUMBER) ln_diff = key1.ln - key2.ln;
    return ln_diff;
}



int tpch_bt_compare_fn_NATION(Db*, const Dbt* k1, const Dbt* k2) {

    return (bt_compare_fn_1_int(k1, k2));
}



int tpch_bt_compare_fn_ORDERS(Db*, const Dbt* k1, const Dbt* k2) {

    return (bt_compare_fn_2_int(k1, k2));
}



int tpch_bt_compare_fn_PART(Db*, const Dbt* k1, const Dbt* k2) {

    return (bt_compare_fn_1_int(k1, k2));
}



int tpch_bt_compare_fn_PARTSUPP(Db*, const Dbt* k1, const Dbt* k2) {

    return (bt_compare_fn_2_int(k1, k2));
}



int tpch_bt_compare_fn_REGION(Db*, const Dbt* k1, const Dbt* k2) {

    return (bt_compare_fn_1_int(k1, k2));
}



int tpch_bt_compare_fn_SUPPLIER(Db*, const Dbt* k1, const Dbt* k2) {

    return (bt_compare_fn_1_int(k1, k2));
}


EXIT_NAMESPACE(tpch);


