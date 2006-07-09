/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_compare.h"
#include <stdlib.h>



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
