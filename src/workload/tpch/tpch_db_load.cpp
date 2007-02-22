/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_db_load.h"
#include "workload/tpch/tpch_compare.h"
#include "workload/tpch/tpch_env.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"

#include "util.h"

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
static void db_table_load(void (*tbl_loader) (Db*, FILE*),
                          Db* db,
                          const char* tbl_path, const char* tbl_filename);



/* definitions of exported functions */

void tpch_load_customer_table(Db* db, FILE* fd) {

    char linebuffer[MAX_LINE_LENGTH];

    TRACE(TRACE_DEBUG, "Populating CUSTOMER...\n");
    progress_reset();
    tpch_customer_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));
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

        // insert tuple into database
        Dbt key(&tup.C_CUSTKEY, sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }

    progress_done();
}



void tpch_load_lineitem_table(Db* db, FILE* fd) {

    char linebuffer[MAX_LINE_LENGTH];

    printf("Populating LINEITEM...\n");
    progress_reset();
    tpch_lineitem_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

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

        // insert tuple into database
        // key is composed of 2 fields 	L_ORDERKEY, L_LINENUMBER
        Dbt key(&tup.L_ORDERKEY, 2 * sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }

    progress_done();
}



void tpch_load_nation_table(Db* db, FILE* fd) {

    char linebuffer[MAX_LINE_LENGTH];
 
    printf("Populating NATION...\n");
    progress_reset();
    tpch_nation_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.N_NATIONKEY = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.N_NAME = nnamestr_to_nname(tmp);
        tmp = strtok(NULL, "|");
        tup.N_REGIONKEY = atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.N_COMMENT, tmp);

        // insert tuple into database
        Dbt key(&tup.N_NATIONKEY, sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }

    progress_done();
}



void tpch_load_orders_table(Db* db, FILE* fd) {
    
    char linebuffer[MAX_LINE_LENGTH];

    printf("Populating ORDERS...\n");
    progress_reset();
    tpch_orders_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

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

        // insert tuple into database
        // key is composed of 2 fields 	O_ORDERKEY and O_CUSTKEY
        Dbt key(&tup.O_ORDERKEY, 2 * sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }

    progress_done();
}



void tpch_load_part_table(Db* db, FILE* fd) {

    char linebuffer[MAX_LINE_LENGTH];
 
    printf("Populating PART...\n");
    progress_reset();
    tpch_part_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

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

        // insert tuple into database
        Dbt key(&tup.P_PARTKEY, sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }

    progress_done();
}



void tpch_load_partsupp_table(Db* db, FILE* fd) {

    char linebuffer[MAX_LINE_LENGTH];

    printf("Populating PARTSUPP...\n");
    progress_reset();
    tpch_partsupp_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

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
	
        // insert tuple into database
        // key is composed of 2 fields 	PS_PARTKEY and PS_SUPPKEY
        Dbt key(&tup.PS_PARTKEY, 2 * sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }

    progress_done();
}



void tpch_load_region_table(Db* db, FILE* fd) {

    char linebuffer[MAX_LINE_LENGTH];

    printf("Populating REGION...\n");
    progress_reset();
    tpch_region_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.R_REGIONKEY= atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.R_NAME, tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.R_COMMENT, tmp);

        // insert tuple into database
        Dbt key(&tup.R_REGIONKEY, sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }

    progress_done();
}



void tpch_load_supplier_table(Db* db, FILE* fd) {

    char linebuffer[MAX_LINE_LENGTH];
    static int count = 0;

    printf("Populating SUPPLIER...\n");
    progress_reset();
    tpch_supplier_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

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

        // insert tuple into database
        Dbt key(&tup.S_SUPPKEY, sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        if (count++ < 10 && 0) {
            TRACE(TRACE_ALWAYS, "Inserting supplier tuple (%d|%s|%s|%d|%s|%lf|%s)\n",
                  tup.S_SUPPKEY,
                  tup.S_NAME,
                  tup.S_ADDRESS,
                  tup.S_NATIONKEY,
                  tup.S_PHONE,
                  tup.S_ACCTBAL.to_double(),
                  tup.S_COMMENT);
        }

        progress_update();
    }

    progress_done();
}



/**
 *  @brief Load TPC-H tables.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void db_load(const char* tbl_path) {

    // load all TPC-H tables from the files generated by dbgen
    db_table_load(tpch_load_customer_table, tpch_customer, tbl_path, TBL_FILENAME_CUSTOMER);
    db_table_load(tpch_load_lineitem_table, tpch_lineitem, tbl_path, TBL_FILENAME_LINEITEM);
    db_table_load(tpch_load_nation_table,   tpch_nation,   tbl_path, TBL_FILENAME_NATION);
    db_table_load(tpch_load_orders_table,   tpch_orders,   tbl_path, TBL_FILENAME_ORDERS);
    db_table_load(tpch_load_part_table,     tpch_part,     tbl_path, TBL_FILENAME_PART);
    db_table_load(tpch_load_partsupp_table, tpch_partsupp, tbl_path, TBL_FILENAME_PARTSUPP);
    db_table_load(tpch_load_region_table,   tpch_region,   tbl_path, TBL_FILENAME_REGION);
    db_table_load(tpch_load_supplier_table, tpch_supplier, tbl_path, TBL_FILENAME_SUPPLIER);
}



/* definitions of internal helper functions */

static void store_string(char* dest, char* src) {
    int len = strlen(src);
    strncpy(dest, src, len);
    dest[len] = '\0';
}



static void db_table_load(void (*tbl_loader) (Db*, FILE*),
                          Db* db,
                          const char* tbl_path, const char* tbl_filename) {
    
    // prepend filename with common path
    c_str path("%s/%s", tbl_path, tbl_filename);

    FILE* fd = fopen(path.data(), "r");
    if (fd == NULL) {
        TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", path.data());
        THROW1(BdbException, "fopen() failed");
    }

    tbl_loader(db, fd);

    if ( fclose(fd) ) {
        TRACE(TRACE_ALWAYS, "fclose() failed on %s\n", path.data());
        THROW1(BdbException, "fclose() failed");
    }
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
