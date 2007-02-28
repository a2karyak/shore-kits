/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_tuple_parsers.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "util.h"

#include "core.h"

#include <cstdlib>
#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>

using namespace qpipe;



/* internal constants */

#define MAX_LINE_LENGTH 1024
#define MAX_FILENAME_SIZE 1024



/* helper functions */

static void store_string(char* dest, char* src);



/* definitions of exported functions */


void tpch_tuple_parser_CUSTOMER(char* dest, char* linebuffer) {

    tpch_customer_tuple &tup = *aligned_cast<tpch_customer_tuple>(dest);
    
    // split line into tab separated parts
    char* tmp = strtok(linebuffer, "|");
    tup.C_CUSTKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.C_NAME, tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.C_ADDRESS, tmp);
    tmp = strtok(NULL, "|");
    tup.C_NATIONKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.C_PHONE, tmp);
    tmp = strtok(NULL, "|");
    tup.C_ACCTBAL = atof(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.C_MKTSEGMENT, tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.C_COMMENT, tmp);
}



void tpch_tuple_parser_LINEITEM(char* dest, char* linebuffer) {

    tpch_lineitem_tuple &tup = *aligned_cast<tpch_lineitem_tuple>(dest);
    
    // split line into tab separated parts
    char* tmp = strtok(linebuffer, "|");
    tup.L_ORDERKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    tup.L_PARTKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    tup.L_SUPPKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    tup.L_LINENUMBER = atoi(tmp);
    tmp = strtok(NULL, "|");
    tup.L_QUANTITY = atof(tmp);
    tmp = strtok(NULL, "|");
    tup.L_EXTENDEDPRICE = atof(tmp);
    tmp = strtok(NULL, "|");
    tup.L_DISCOUNT = atof(tmp);
    tmp = strtok(NULL, "|");
    tup.L_TAX = atof(tmp);
    tmp = strtok(NULL, "|");
    tup.L_RETURNFLAG = tmp[0];
    tmp = strtok(NULL, "|");
    tup.L_LINESTATUS = tmp[0];
    tmp = strtok(NULL, "|");
    tup.L_SHIPDATE = datestr_to_timet(tmp);
    tmp = strtok(NULL, "|");
    tup.L_COMMITDATE = datestr_to_timet(tmp);
    tmp = strtok(NULL, "|");
    tup.L_RECEIPTDATE = datestr_to_timet(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.L_SHIPINSTRUCT, tmp);
    tmp = strtok(NULL, "|");
    tup.L_SHIPMODE = modestr_to_shipmode(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.L_COMMENT, tmp);
}



void tpch_tuple_parser_NATION(char* dest, char* linebuffer) {

    tpch_nation_tuple &tup = *aligned_cast<tpch_nation_tuple>(dest);
    
    // split line into tab separated parts
    char* tmp = strtok(linebuffer, "|");
    tup.N_NATIONKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    tup.N_NAME = nnamestr_to_nname(tmp);
    tmp = strtok(NULL, "|");
    tup.N_REGIONKEY = atoi(tmp);

    tmp = strtok(NULL, "|");
    store_string(tup.N_COMMENT, tmp);
}



void tpch_tuple_parser_ORDERS(char* dest, char* linebuffer) {

    tpch_orders_tuple &tup = *aligned_cast<tpch_orders_tuple>(dest);
    
    // split line into tab separated parts
    char* tmp = strtok(linebuffer, "|");
    tup.O_ORDERKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    tup.O_CUSTKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    tup.O_ORDERSTATUS = tmp[0];
    tmp = strtok(NULL, "|");
    tup.O_TOTALPRICE = atof(tmp);
    tmp = strtok(NULL, "|");
    tup.O_ORDERDATE = datestr_to_timet(tmp);
    tmp = strtok(NULL, "|");
    tup.O_ORDERPRIORITY = prioritystr_to_orderpriorty(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.O_CLERK, tmp);
    tmp = strtok(NULL, "|");
    tup.O_SHIPPRIORITY = atoi(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.O_COMMENT, tmp);
}



void tpch_tuple_parser_PART(char* dest, char* linebuffer) {

    tpch_part_tuple &tup = *aligned_cast<tpch_part_tuple>(dest);
    
    // split line into tab separated parts
    char* tmp = strtok(linebuffer, "|");
    tup.P_PARTKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.P_NAME, tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.P_MFGR, tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.P_BRAND, tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.P_TYPE, tmp);
    tmp = strtok(NULL, "|");
    tup.P_SIZE = atoi(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.P_CONTAINER, tmp);
    tmp = strtok(NULL, "|");
    tup.P_RETAILPRICE = atof(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.P_COMMENT, tmp);
}



void tpch_tuple_parser_PARTSUPP(char* dest, char* linebuffer) {

    tpch_partsupp_tuple &tup = *aligned_cast<tpch_partsupp_tuple>(dest);

    // split line into tab separated parts
    char* tmp = strtok(linebuffer, "|");
    tup.PS_PARTKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    tup.PS_SUPPKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    tup.PS_AVAILQTY = atoi(tmp);
    tmp = strtok(NULL, "|");
    tup.PS_SUPPLYCOST = atof(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.PS_COMMENT, tmp);
}



void tpch_tuple_parser_REGION(char* dest, char* linebuffer) {

    tpch_region_tuple &tup = *aligned_cast<tpch_region_tuple>(dest);

    // split line into tab separated parts
    char* tmp = strtok(linebuffer, "|");
    tup.R_REGIONKEY= atoi(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.R_NAME, tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.R_COMMENT, tmp);
}



void tpch_tuple_parser_SUPPLIER(char* dest, char* linebuffer) {

    tpch_supplier_tuple &tup = *aligned_cast<tpch_supplier_tuple>(dest);

    // split line into tab separated parts
    char* tmp = strtok(linebuffer, "|");
    tup.S_SUPPKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.S_NAME, tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.S_ADDRESS, tmp);
    tmp = strtok(NULL, "|");
    tup.S_NATIONKEY = atoi(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.S_PHONE, tmp);
    tmp = strtok(NULL, "|");
    tup.S_ACCTBAL= atof(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.S_COMMENT, tmp);
}



/* definitions of internal helper functions */

static void store_string(char* dest, char* src) {
    int len = strlen(src);
    strncpy(dest, src, len);
    dest[len] = '\0';
}
