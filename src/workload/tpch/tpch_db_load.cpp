/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_db_load.h"
#include "workload/tpch/tpch_env.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"

#include "core.h"

#include <cstdlib>
#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>



void progress_reset();
void progress_update();
void progress_done();

#define MAX_LINE_LENGTH 1024
#define MAX_FILENAME_SIZE 1024



/* internal helper functions */

static void store_string(char* dest, char* src);

using namespace qpipe;

/* definitions of exported functions */

// dest is guaranteed to be zeroed out and have "enough" alignment for casting purposes
typedef void (*parse_tuple)(char* dest, char* linebuffer);

void parse_table(char const* fin_name, char const* fout_name,
		 parse_tuple parse, size_t tuple_size)
{
    char linebuffer[MAX_LINE_LENGTH];
    
    // force 8-byte alignment
    array_guard_t<char> tuple_data = new char[tuple_size+7];
    union {
	size_t i;
	char* p;
    } u;
    u.p = tuple_data;
    u.i = (u.i+7) & -8;
    tuple_t tuple(u.p, tuple_size);

    TRACE(TRACE_DEBUG, "Populating %s...\n", fout_name);
    progress_reset();

    FILE* fin = fopen(fin_name, "r");
    if (fin == NULL) {
        TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", fin_name);
        THROW1(BdbException, "fopen() failed");
    }
    FILE* fout = fopen(fout_name, "w");
    if (fout == NULL) {
        TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", fout_name);
        THROW1(BdbException, "fopen() failed");
    }

    qpipe::page *p = qpipe::page::alloc(tuple_size);
    while (fgets(linebuffer, MAX_LINE_LENGTH, fin)) {
        // flush to file?
        if(p->full()) {
            p->fwrite_full_page(fout);
            p->clear();
        }
        
        memset(tuple_data, 0, tuple_size);
        parse(tuple_data, linebuffer);
        p->append_tuple(tuple);
        progress_update();
    }

    if(!p->empty())
        p->fwrite_full_page(fout);
    p->free();
    
    if ( fclose(fout) ) {
        TRACE(TRACE_ALWAYS, "fclose() failed on %s\n", fout_name);
        THROW1(BdbException, "fclose() failed");
    }
    if ( fclose(fin) ) {
        TRACE(TRACE_ALWAYS, "fclose() failed on %s\n", fin_name);
        THROW1(BdbException, "fclose() failed");
    }
    
    progress_done();
}

void tpch_load_customer_table(char* dest, char* linebuffer) {

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



void tpch_load_lineitem_table(char* dest, char* linebuffer) {
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



void tpch_load_nation_table(char* dest, char* linebuffer) {
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



void tpch_load_orders_table(char* dest, char* linebuffer) {
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



void tpch_load_part_table(char* dest, char* linebuffer) {
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



void tpch_load_partsupp_table(char* dest, char* linebuffer) {
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



void tpch_load_region_table(char* dest, char* linebuffer) {
    tpch_region_tuple &tup = *aligned_cast<tpch_region_tuple>(dest);

    // split line into tab separated parts
    char* tmp = strtok(linebuffer, "|");
    tup.R_REGIONKEY= atoi(tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.R_NAME, tmp);
    tmp = strtok(NULL, "|");
    store_string(tup.R_COMMENT, tmp);
}



void tpch_load_supplier_table(char* dest, char* linebuffer) {
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



/**
 *  @brief Load TPC-H tables.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void db_load(const char*) {
#define PARSE(fin, fout, table) \
parse_table("tbl/" fin, "database/" fout, &tpch_load_##table##_table, sizeof(tpch_##table##_tuple))
    
    // load all TPC-H tables from the files generated by dbgen
    PARSE(TBL_FILENAME_CUSTOMER, CDB_FILENAME_CUSTOMER, customer);
    PARSE(TBL_FILENAME_LINEITEM, CDB_FILENAME_LINEITEM, lineitem);
    PARSE(TBL_FILENAME_NATION,   CDB_FILENAME_NATION,   nation);
    PARSE(TBL_FILENAME_ORDERS,   CDB_FILENAME_ORDERS,   orders);
    PARSE(TBL_FILENAME_PART,     CDB_FILENAME_PART,     part);
    PARSE(TBL_FILENAME_PARTSUPP, CDB_FILENAME_PARTSUPP, partsupp);
    PARSE(TBL_FILENAME_REGION,   CDB_FILENAME_REGION,   region);
    PARSE(TBL_FILENAME_SUPPLIER, CDB_FILENAME_SUPPLIER, supplier);
}



/* definitions of internal helper functions */

static void store_string(char* dest, char* src) {
    int len = strlen(src);
    strncpy(dest, src, len);
    dest[len] = '\0';
}



// progress reporting

#define PROGRESS_INTERVAL 100000

static unsigned long progress = 0;

void progress_reset() {
    progress = 0;
}

void progress_update() {
    if ( (progress++ % PROGRESS_INTERVAL) == 0 ) {
        printf(".");
        fflush(stdout);
        progress = 1; // prevent overflow
    }
}

void progress_done() {
    printf("done\n");
    fflush(stdout);
}
