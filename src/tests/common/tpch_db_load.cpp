/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common/tpch_db_load.h"
#include "tests/common/tpch_compare.h"
#include "tests/common/tpch_env.h"
#include "tests/common/tpch_struct.h"
#include "tests/common/tpch_type_convert.h"

#include "engine/bdb_config.h"
#include "qpipe_panic.h"
#include "trace.h"

#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

void progress_reset();
void progress_update();
void progress_done();

#define MAX_LINE_LENGTH 1024
#define MAX_FILENAME_SIZE 1024





void tpch_load_customer_table(Db* db, const char* fname) {

    char linebuffer[MAX_LINE_LENGTH];

    printf("Populating CUSTOMER...\n");
    progress_reset();
    tpch_customer_tuple tup;

    FILE* fd = fopen(fname, "r");
    if (!fd) {
        fprintf(stderr, "Enable to open %s - bailing out\n", fname);
        exit(1);
    }
    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.C_CUSTKEY = atoi(tmp);
        tmp = strtok(NULL, "|");
        memcpy(tup.C_NAME, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        memcpy(tup.C_ADDRESS, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        tup.C_NATIONKEY = atoi(tmp);
        tmp = strtok(NULL, "|");
        memcpy(tup.C_PHONE, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        tup.C_ACCTBAL = atof(tmp);
        tmp = strtok(NULL, "|");
        memcpy(tup.C_MKTSEGMENT, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");

        int comment_len = strlen(tmp);
        strncpy(tup.C_COMMENT, tmp, comment_len);
        tup.C_COMMENT[comment_len] = '\0';

        // insert tuple into database
        Dbt key(&tup.C_CUSTKEY, sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }
    fclose(fd);
    progress_done();
}



void tpch_load_lineitem_table(Db* db, const char* fname) {

    char linebuffer[MAX_LINE_LENGTH];

    printf("Populating LINEITEM...\n");
    progress_reset();
    tpch_lineitem_tuple tup;

    FILE* fd = fopen(fname, "r");
    if (!fd) {
        fprintf(stderr, "Unable to open %s - bailing out\n", fname);
        exit(1);
    }
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
        memcpy(tup.L_SHIPINSTRUCT, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        tup.L_SHIPMODE = modestr_to_shipmode(tmp);
        tmp = strtok(NULL, "|");

        int comment_len = strlen(tmp);
        strncpy(tup.L_COMMENT, tmp, comment_len);
        tup.L_COMMENT[comment_len] = '\0';

        // insert tuple into database
        // key is composed of 3 fields 	L_ORDERKEY, L_PARTKEY, and L_SUPPKEY
        Dbt key(&tup.L_ORDERKEY, 3 * sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }
    fclose(fd);
    progress_done();
}



void tpch_load_nation_table(Db* db, const char* fname) {

    char linebuffer[MAX_LINE_LENGTH];
 
    printf("Populating NATION...\n");
    progress_reset();
    tpch_nation_tuple tup;

    FILE* fd = fopen(fname, "r");
    if (!fd) {
        fprintf(stderr, "Unable to open %s - bailing out\n", fname);
        exit(1);
    }
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

        int comment_len = strlen(tmp);
        strncpy(tup.N_COMMENT, tmp, comment_len);
        tup.N_COMMENT[comment_len] = '\0';

        // insert tuple into database
        Dbt key(&tup.N_NATIONKEY, sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }
    fclose(fd);
    progress_done();
}



void tpch_load_orders_table(Db* db, const char* fname) {
    
    char linebuffer[MAX_LINE_LENGTH];

    printf("Populating ORDERS...\n");
    progress_reset();
    tpch_orders_tuple tup;

    FILE* fd = fopen(fname, "r");
    if (!fd) {
        fprintf(stderr, "Unable to open %s - bailing out\n", fname);
        exit(1);
    }
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
        memcpy(tup.O_CLERK, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        tup.O_SHIPPRIORITY = atoi(tmp);
        tmp = strtok(NULL, "|");
        
        int comment_len = strlen(tmp);
        strncpy(tup.O_COMMENT, tmp, comment_len);
        tup.O_COMMENT[comment_len] = '\0';

        // insert tuple into database
        // key is composed of 2 fields 	O_ORDERKEY and O_CUSTKEY
        Dbt key(&tup.O_ORDERKEY, 2 * sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }
    fclose(fd);
    progress_done();
}



void tpch_load_part_table(Db* db, const char* fname) {

    char linebuffer[MAX_LINE_LENGTH];
 
    printf("Populating PART...\n");
    progress_reset();
    tpch_part_tuple tup;

    FILE* fd = fopen(fname, "r");
    if (!fd) {
        fprintf(stderr, "Unable to open %s - bailing out\n", fname);
        exit(1);
    }
    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.P_PARTKEY = atoi(tmp);
        tmp = strtok(NULL, "|");
        memcpy(tup.P_NAME, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        memcpy(tup.P_MFGR, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        memcpy(tup.P_BRAND, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        memcpy(tup.P_TYPE, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        tup.P_SIZE = atoi(tmp);
        tmp = strtok(NULL, "|");
        memcpy(tup.P_CONTAINER, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        tup.P_RETAILPRICE = atof(tmp);
        tmp = strtok(NULL, "|");

        int comment_len = strlen(tmp);
        strncpy(tup.P_COMMENT, tmp, comment_len);
        tup.P_COMMENT[comment_len] = '\0';

        // insert tuple into database
        Dbt key(&tup.P_PARTKEY, sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }
    fclose(fd);
    progress_done();
}



void tpch_load_partsupp_table(Db* db, const char* fname) {

    char linebuffer[MAX_LINE_LENGTH];

    printf("Populating PARTSUPP...\n");
    progress_reset();
    tpch_partsupp_tuple tup;

    FILE* fd = fopen(fname, "r");
    if (!fd) {
        fprintf(stderr, "Unable to open %s - bailing out\n", fname);
        exit(1);
    }
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

        int comment_len = strlen(tmp);
        strncpy(tup.PS_COMMENT, tmp, comment_len);
        tup.PS_COMMENT[comment_len] = '\0';
	
        // insert tuple into database
        // key is composed of 2 fields 	PS_PARTKEY and PS_SUPPKEY
        Dbt key(&tup.PS_PARTKEY, 2 * sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }
    fclose(fd);
    progress_done();
}



void tpch_load_region_table(Db* db, const char* fname) {

    char linebuffer[MAX_LINE_LENGTH];

    printf("Populating REGION...\n");
    progress_reset();
    tpch_region_tuple tup;

    FILE* fd = fopen(fname, "r");
    if (!fd) {
        fprintf(stderr, "Unable to open %s - bailing out\n", fname);
        exit(1);
    }
    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.R_REGIONKEY= atoi(tmp);
        tmp = strtok(NULL, "|");
        memcpy(tup.R_NAME, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");

        int comment_len = strlen(tmp);
        strncpy(tup.R_COMMENT, tmp, comment_len);
        tup.R_COMMENT[comment_len] = '\0';

        // insert tuple into database
        Dbt key(&tup.R_REGIONKEY, sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }
    fclose(fd);
    progress_done();
}



void tpch_load_supplier_table(Db* db, const char* fname) {

    char linebuffer[MAX_LINE_LENGTH];

    printf("Populating SUPPLIER...\n");
    progress_reset();
    tpch_supplier_tuple tup;

    FILE* fd = fopen(fname, "r");
    if (!fd) {
        fprintf(stderr, "Unable to open %s - bailing out\n", fname);
        exit(1);
    }
    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.S_SUPPKEY = atoi(tmp);
        tmp = strtok(NULL, "|");
        memcpy(tup.S_NAME, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        memcpy(tup.S_ADDRESS, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        tup.S_NATIONKEY = atoi(tmp);
        tmp = strtok(NULL, "|");
        memcpy(tup.S_PHONE, tmp, strlen(tmp));
        tmp = strtok(NULL, "|");
        tup.S_ACCTBAL= atof(tmp);
        tmp = strtok(NULL, "|");

        int comment_len = strlen(tmp);
        strncpy(tup.S_COMMENT, tmp, comment_len);
        tup.S_COMMENT[comment_len] = '\0';

        // insert tuple into database
        Dbt key(&tup.S_SUPPKEY, sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update();
    }
    fclose(fd);
    progress_done();
}



bool db_table_load(void (*tbl_loader) (Db*, const char*),
                   Db* db,
                   const char* tbl_path, const char* tbl_filename) {
    
    // prepend filename with common path
    char path[MAX_FILENAME_SIZE];
    snprintf(path, MAX_FILENAME_SIZE, "%s/%s", tbl_path, tbl_filename);
    tbl_loader(db, path);
    return true;
}
    


bool db_load(const char* tbl_path) {

    // load all TPC-H tables from the files generated by dbgen
    if ( !db_table_load(tpch_load_customer_table, tpch_customer, tbl_path, TABLE_CUSTOMER_TBL_FILENAME) )
        return false;
    if ( !db_table_load(tpch_load_lineitem_table, tpch_lineitem, tbl_path, TABLE_LINEITEM_TBL_FILENAME) )
        return false;
    if ( !db_table_load(tpch_load_nation_table,   tpch_nation,   tbl_path, TABLE_NATION_TBL_FILENAME) )
        return false;
    if ( !db_table_load(tpch_load_orders_table,   tpch_orders,   tbl_path, TABLE_ORDERS_TBL_FILENAME) )
        return false;
    if ( !db_table_load(tpch_load_part_table,     tpch_part,     tbl_path, TABLE_PART_TBL_FILENAME) )
        return false;
    if ( !db_table_load(tpch_load_partsupp_table, tpch_partsupp, tbl_path, TABLE_PARTSUPP_TBL_FILENAME) )
        return false;
    if ( !db_table_load(tpch_load_region_table,   tpch_region,   tbl_path, TABLE_REGION_TBL_FILENAME) )
        return false;
    if ( !db_table_load(tpch_load_supplier_table, tpch_supplier, tbl_path, TABLE_SUPPLIER_TBL_FILENAME) )
        return false;

    return true;
}



// progress reporting
// TODO move to another file?

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
