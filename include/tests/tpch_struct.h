
/**
 *
 *  @brief Exports structures that we store/load from BerkeleyDB.
 */
#ifndef TPCH_STRUCT_H
#define TPCH_STRUCT_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>



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
  int C_CUSTKEY;
  char C_NAME[25];
  char C_ADDRESS[40];
  int C_NATIONKEY;
  char C_PHONE[15];
  double C_ACCTBAL;
  char C_MKTSEGMENT[10];
  // char C_COMMENT[117];
};

struct tpch_lineitem_tuple {
  int L_ORDERKEY;
  int L_PARTKEY;
  int L_SUPPKEY;
  int L_LINENUMBER;
  double L_QUANTITY;
  double L_EXTENDEDPRICE;
  double L_DISCOUNT;
  double L_TAX;
  char L_RETURNFLAG;
  char L_LINESTATUS;
  time_t L_SHIPDATE;
  time_t L_COMMITDATE;
  time_t L_RECEIPTDATE;
  char L_SHIPINSTRUCT[25];
  tpch_l_shipmode L_SHIPMODE;
  // char L_COMMENT[44];
};

struct tpch_nation_tuple {
  int N_NATIONKEY;
  tpch_n_name N_NAME;
  int N_REGIONKEY;
  // char N_COMMENT[152];
};

struct tpch_orders_tuple {
  int O_ORDERKEY;
  int O_CUSTKEY;
  char O_ORDERSTATUS;
  double O_TOTALPRICE;
  time_t O_ORDERDATE;
  tpch_o_orderpriority O_ORDERPRIORITY;
  char O_CLERK[15];
  int O_SHIPPRIORITY;
  char O_COMMENT[79];
};

struct tpch_part_tuple {
  int P_PARTKEY;
  char P_NAME[55];
  char P_MFGR[25];
  char P_BRAND[10];
  char P_TYPE[25];
  int P_SIZE;
  char P_CONTAINER[10];
  double P_RETAILPRICE;
  // char P_COMMENT[23];
};

struct tpch_partsupp_tuple {
  int PS_PARTKEY;
  int PS_SUPPKEY;
  int PS_AVAILQTY;
  double PS_SUPPLYCOST;
  // char PS_COMMENT[199];
};

struct tpch_region_tuple {
  int R_REGIONKEY;
  char R_NAME[25];
  // char R_COMMENT[152];
};

struct tpch_supplier_tuple {
  int S_SUPPKEY;
  char S_NAME[25];
  char S_ADDRESS[40];
  int S_NATIONKEY;
  char S_PHONE[15];
  double S_ACCTBAL;
  // char S_COMMENT[101];
};


#endif	// TPCH_STRUCT_H
