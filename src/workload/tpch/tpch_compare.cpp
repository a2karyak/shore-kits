/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_compare.h"
#include "workload/tpch/tpch_struct.h"
#include <cstdlib>


ENTER_NAMESPACE(tpch);

// 'typeof' is a gnu extension
#define TYPEOF(TYPE, MEMBER) typeof(((TYPE *)0)->MEMBER)


#define SIZEOF(TYPE, MEMBER) sizeof(((TYPE *)0)->MEMBER)

// 'offsetof' is found in <stddef.h>
#define FIELD(SRC, TYPE, MEMBER) (((char*) (SRC)) + offsetof(TYPE, MEMBER))

#define GET_FIELD(DEST, SRC, TYPE, MEMBER) \
    memcpy(DEST, \
           FIELD(SRC, TYPE, MEMBER), \
           SIZEOF(TYPE, MEMBER))

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



int tpch_bt_compare_fn_ORDERS(Db*, const Dbt* k1, const Dbt* k2) {

    // key has 2 integers
    int u1[2];
    int u2[2];
    memcpy(u1, k1->get_data(), 2 * sizeof(int));
    memcpy(u2, k2->get_data(), 2 * sizeof(int));

    if ((u1[0] < u2[0]) || ((u1[0] == u2[0]) && (u1[1] < u2[1])))
	return -1;
    else if ((u1[0] == u2[0]) && (u1[1] == u2[1]))
	return 0;
    else
	return 1;
}



int tpch_bt_compare_fn_PART(Db*, const Dbt* k1, const Dbt* k2) {

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



int tpch_bt_compare_fn_PARTSUPP(Db*, const Dbt* k1, const Dbt* k2) {

    // key has 2 integers
    int u1[2];
    int u2[2];
    memcpy(u1, k1->get_data(), 2 * sizeof(int));
    memcpy(u2, k2->get_data(), 2 * sizeof(int));

    if ((u1[0] < u2[0]) || ((u1[0] == u2[0]) && (u1[1] < u2[1])))
        return -1;
    else if ((u1[0] == u2[0]) && (u1[1] == u2[1]))
        return 0;
    else
        return 1;
}



int tpch_bt_compare_fn_REGION(Db*, const Dbt* k1, const Dbt* k2) {

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

EXIT_NAMESPACE(tpch);


