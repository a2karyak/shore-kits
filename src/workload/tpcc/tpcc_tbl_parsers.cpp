/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_tbl_parsers.cpp
 *
 *  @brief Implementation of the TPC-C table parsing functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

/*
#include "workload/common/bdb_env.h"
//#include "workload/tpcc/tpcc_type_convert.h"
*/

#include "util/trace.h"
#include "util/time_util.h"
#include "util/progress.h"

#include "workload/tpcc/tpcc_tbl_parsers.h"
#include "workload/tpcc/tpcc_struct.h"


ENTER_NAMESPACE(tpcc);


/* definitions of exported functions */


/* internal constants */

static unsigned long progress = 0;


/* definitions of exported functions */


void tpcc_parse_tbl_CUSTOMER  (Db* db, FILE* fd) { 
  
    char linebuffer[MAX_LINE_LENGTH];

    TRACE(TRACE_DEBUG, "Populating TPC-C CUSTOMER...\n");
    progress_reset(&progress);
    tpcc_customer_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));
        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.C_C_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.C_D_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.C_W_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_FIRST, tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_MIDDLE, tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_LAST, tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_STREET_1, tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_STREET_2, tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_CITY, tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_STATE, tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_ZIP, tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_PHONE, tmp);
        tmp = strtok(NULL, "|");
        tup.C_SINCE = atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_CREDIT, tmp);
        tmp = strtok(NULL, "|");
        tup.C_CREDIT_LIM = atof(tmp);
        tmp = strtok(NULL, "|");
        tup.C_DISCOUNT = atof(tmp);
        tmp = strtok(NULL, "|");
        tup.C_BALANCE = atof(tmp);
        tmp = strtok(NULL, "|");
        tup.C_YTD_PAYMENT = atof(tmp);
        tmp = strtok(NULL, "|");
        tup.C_LAST_PAYMENT = atof(tmp);
        tmp = strtok(NULL, "|");
        tup.C_PAYMENT_CNT = atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_DATA_1, tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.C_DATA_2, tmp);


        // insert tuple into database
        // CUSTOMER key is composed of 3 fields: C_C_ID, C_D_ID, C_W_ID
        Dbt key(&tup.C_C_ID, 3 * sizeof(int));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update(&progress);
    }

    progress_done();
}



void tpcc_parse_tbl_DISTRICT  (Db* db, FILE* fd) { 
    TRACE( TRACE_ALWAYS, "Doing Nothing!"); }  


void tpcc_parse_tbl_HISTORY   (Db* db, FILE* fd) { 

    char linebuffer[MAX_LINE_LENGTH];

    TRACE(TRACE_DEBUG, "Populating TPC-C HISTORY...\n");
    progress_reset(&progress);
    tpcc_history_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));
        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.H_C_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.H_C_D_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.H_C_W_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.H_D_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.H_W_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.H_DATE = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.H_AMOYNT = atof(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.H_DATA,tmp);

        // insert tuple into database
        // HISTORY does not have a key, use the whole tuple
        Dbt key(&tup, sizeof(tpcc_history_tuple));
        Dbt data(&tup, sizeof(tpcc_history_tuple));
        db->put(NULL, &key, &data, 0);

        progress_update(&progress);
    }

    progress_done();
}


void tpcc_parse_tbl_ITEM      (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_NEW_ORDER (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_ORDER     (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_ORDERLINE (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_STOCK     (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_WAREHOUSE (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }


EXIT_NAMESPACE(tpcc);

