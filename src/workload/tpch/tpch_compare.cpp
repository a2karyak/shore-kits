/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_compare.h"
#include "workload/tpch/tpch_struct.h"
#include <cstdlib>

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
    GET_FIELD(&t1, k1->get_data(), tpch_lineitem_tuple, L_SHIPDATE);
    GET_FIELD(&t2, k2->get_data(), tpch_lineitem_tuple, L_SHIPDATE);
           
    return t1 - t2;
}

int tpch_lineitem_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2) {

    // key has 3 integers
    int u1[3];
    int u2[3];
    memcpy(u1, k1->get_data(), 3 * sizeof(int));
    memcpy(u2, k2->get_data(), 3 * sizeof(int));

    if ((u1[0] < u2[0]) || ((u1[0] == u2[0]) && (u1[1] < u2[1])) || ((u1[0] == u2[0]) && (u1[1] == u2[1]) && (u1[2] < u2[2])))
        return -1;
    else if ((u1[0] == u2[0]) && (u1[1] == u2[1]) && (u1[2] == u2[2]))
        return 0;
    else
        return 1;
}


int tpch_orders_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2) {
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


int tpch_part_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2) {

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


int tpch_partsupp_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2) {
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


int tpch_region_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2) {

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


int tpch_supplier_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2) {

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


int tpch_customer_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2) {

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


int tpch_nation_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2) {

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
