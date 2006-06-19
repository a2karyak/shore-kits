/* -*- mode:C++; c-basic-offset:4 -*- */


/**
 *
 *  @brief Exports structures that we store/load from BerkeleyDB.
 *
 *  Comment field:
 *
 *  TPC-H spec documents the maximum number of "valid" characters for
 *  data. For example, the orders table has comment attributes of at
 *  most 79 characters. We add an extra character for NUL-termination
 *  so we can use libc string functions on the loaded data.
 */
#ifndef TPCH_STRUCT_H
#define TPCH_STRUCT_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>



/* use this for allocation of NUL-terminated strings */

#define STRSIZE(x)(x+1)



/* exported structures */

enum tpch_l_shipmode {
    REG_AIR,
    AIR,
    RAIL,
    TRUCK,
    MAIL,
    FOB,
    SHIP
};

enum tpch_o_orderpriority {
    URGENT_1,
    HIGH_2,
    MEDIUM_3,
    NOT_SPECIFIED_4,
    LOW_5
};

enum tpch_n_name{
    ALGERIA,
    ARGENTINA,
    BRAZIL,
    CANADA,
    EGYPT,
    ETHIOPIA,
    FRANCE,
    GERMANY,
    INDIA,
    INDONESIA,
    IRAN,
    IRAQ,
    JAPAN,
    JORDAN,
    KENYA,
    MOROCCO,
    MOZAMBIQUE,
    PERU,
    CHINA,
    ROMANIA,
    SAUDI_ARABIA,
    VIETNAM,
    RUSSIA,
    UNITED_KINGDOM,
    UNITED_STATES
};

struct tpch_customer_tuple {
    double C_ACCTBAL;
    int C_CUSTKEY;
    int C_NATIONKEY;
    char C_NAME      [STRSIZE(25)];
    char C_ADDRESS   [STRSIZE(40)];
    char C_PHONE     [STRSIZE(15)];
    char C_MKTSEGMENT[STRSIZE(10)];
    char C_COMMENT   [STRSIZE(117)];
};

struct tpch_lineitem_tuple {
    double L_QUANTITY;
    double L_EXTENDEDPRICE;
    double L_DISCOUNT;
    double L_TAX;
    time_t L_SHIPDATE;
    time_t L_COMMITDATE;
    time_t L_RECEIPTDATE;
    int L_ORDERKEY;
    int L_PARTKEY;
    int L_SUPPKEY;
    int L_LINENUMBER;
    tpch_l_shipmode L_SHIPMODE;
    char L_RETURNFLAG;
    char L_LINESTATUS;
    char L_SHIPINSTRUCT[STRSIZE(25)];
    char L_COMMENT     [STRSIZE(44)];
};

struct tpch_nation_tuple {
    int N_NATIONKEY;
    int N_REGIONKEY;
    tpch_n_name N_NAME;
    char N_COMMENT[STRSIZE(152)];
};

struct tpch_orders_tuple {
    double O_TOTALPRICE;
    time_t O_ORDERDATE;
    int O_ORDERKEY;
    int O_CUSTKEY;
    int O_SHIPPRIORITY;
    tpch_o_orderpriority O_ORDERPRIORITY;
    char O_ORDERSTATUS;
    char O_CLERK  [STRSIZE(15)];
    char O_COMMENT[STRSIZE(79)];
};

struct tpch_part_tuple {
    double P_RETAILPRICE;
    int P_PARTKEY;
    int P_SIZE;
    char P_NAME     [STRSIZE(55)];
    char P_CONTAINER[STRSIZE(10)];
    char P_MFGR     [STRSIZE(25)];
    char P_BRAND    [STRSIZE(10)];
    char P_TYPE     [STRSIZE(25)];
    char P_COMMENT  [STRSIZE(23)];
};

struct tpch_partsupp_tuple {
    double PS_SUPPLYCOST;
    int PS_PARTKEY;
    int PS_SUPPKEY;
    int PS_AVAILQTY;
    char PS_COMMENT[STRSIZE(199)];
};

struct tpch_region_tuple {
    int R_REGIONKEY;
    char R_NAME   [STRSIZE(25)];
    char R_COMMENT[STRSIZE(152)];
};

struct tpch_supplier_tuple {
    double S_ACCTBAL;
    int S_SUPPKEY;
    int S_NATIONKEY;
    char S_NAME   [STRSIZE(25)];
    char S_ADDRESS[STRSIZE(40)];
    char S_PHONE  [STRSIZE(15)];
    char S_COMMENT[STRSIZE(101)];
};


#endif	// TPCH_STRUCT_H
