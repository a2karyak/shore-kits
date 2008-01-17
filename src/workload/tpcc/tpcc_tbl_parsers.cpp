/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_tbl_parsers.cpp
 *
 *  @brief Implementation of the TPC-C table parsing functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util/trace.h"
#include "util/time_util.h"
#include "util/progress.h"
#include "util/store_string.h"

#include "workload/tpcc/tpcc_tbl_parsers.h"


//#define SECOND_PADDING
#define DUPLICATES_PADDING 1000

ENTER_NAMESPACE(tpcc);


/* definitions of exported functions */

////////////////////////////////////////////////////
// BDB-TABLE LOADING
// 
// (ip) To be removed and replaced with the row
//      parsers defined below.

void tpcc_parse_tbl_CUSTOMER(Db* db, FILE* fd) { 
  
    char linebuffer[MAX_LINE_LENGTH];
    unsigned long progress = 0;

    TRACE(TRACE_DEBUG, "Populating TPC-C CUSTOMER...\n");
    progress_reset(&progress);

    tpcc_customer_tuple tup;
    tpcc_customer_tuple_key tk;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {

        // clear the tuple & key
        memset(&tup, 0, sizeof(tup));
        memset(&tk, 0, sizeof(tk));

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


        // construct key
        // CUSTOMER key is composed of 3 fields: C_C_ID, C_D_ID, C_W_ID
        tk.C_C_ID = tup.C_C_ID;
        tk.C_D_ID = tup.C_D_ID;
        tk.C_W_ID = tup.C_W_ID;

        // insert tuple into database
        Dbt key(&tk, sizeof(tk));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update(&progress);
    }

    progress_done("CUSTOMER");
}



void tpcc_parse_tbl_DISTRICT(Db* db, FILE* fd) { 

    char linebuffer[MAX_LINE_LENGTH];
    unsigned long progress = 0;

    TRACE(TRACE_DEBUG, "Populating TPC-C DISTRICT...\n");
    progress_reset(&progress);

    tpcc_district_tuple tup;
    tpcc_district_tuple_key tk;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {

        // clear the tuple & key
        memset(&tup, 0, sizeof(tup));
        memset(&tk, 0, sizeof(tk));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.D_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.D_W_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.D_NAME,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.D_STREET_1,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.D_STREET_2,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.D_CITY,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.D_STATE,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.D_ZIP,tmp);
        tmp = strtok(NULL, "|");
        tup.D_TAX = atof(tmp);
        tmp = strtok(NULL, "|");
        tup.D_YTD = atof(tmp);
        tmp = strtok(NULL, "|");
        tup.D_NEXT_O_ID = atoi(tmp);

        // construct key
        // DISTRICT key composed (D_ID, D_W_ID)
        tk.D_ID = tup.D_ID;
        tk.D_W_ID = tup.D_W_ID;

        // insert tuple into database
        Dbt key(&tk, sizeof(tk));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update(&progress);
    }

    progress_done("DISTRICT");
}



void tpcc_parse_tbl_HISTORY(Db* db, FILE* fd) { 

    char linebuffer[MAX_LINE_LENGTH];
    unsigned long progress = 0;

    TRACE(TRACE_DEBUG, "Populating TPC-C HISTORY...\n");
    progress_reset(&progress);

    tpcc_history_tuple tup;
    tpcc_history_tuple_key hk;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {

        // clear the tuple
        memset(&tup, 0, sizeof(tup));
        memset(&hk, 0, sizeof(hk));

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
        tup.H_AMOUNT = atof(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.H_DATA,tmp);

        // construct key
        // HISTORY does not have a key, use the first 6 integers
        // as the key
        hk.H_C_ID = tup.H_C_ID;
        hk.H_C_D_ID = tup.H_C_D_ID;
        hk.H_C_W_ID = tup.H_C_W_ID;
        hk.H_D_ID = tup.H_D_ID;
        hk.H_W_ID = tup.H_W_ID;
        hk.H_DATE = tup.H_DATE;

        // insert tuple into database
        Dbt key(&hk, sizeof(hk));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update(&progress);
    }

    progress_done("HISTORY");
}



void tpcc_parse_tbl_ITEM(Db* db, FILE* fd) { 

    char linebuffer[MAX_LINE_LENGTH];
    unsigned long progress = 0;

    TRACE(TRACE_DEBUG, "Populating TPC-C ITEM...\n");
    progress_reset(&progress);

    tpcc_item_tuple tup;
    tpcc_item_tuple_key ik;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {

        // clear the tuple
        memset(&tup, 0, sizeof(tup));
        memset(&ik, 0, sizeof(ik));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.I_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.I_IM_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.I_NAME,tmp);
        tmp = strtok(NULL, "|");
        tup.I_PRICE = atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.I_DATA,tmp);

        // construct key
        // ITEM key composed (I_ID)
        ik.I_ID = tup.I_ID;

        // insert tuple into database
        Dbt key(&ik, sizeof(ik));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update(&progress);
    }

    progress_done("ITEM");
}


void tpcc_parse_tbl_NEW_ORDER(Db* db, FILE* fd) { 

    char linebuffer[MAX_LINE_LENGTH];
    unsigned long progress = 0;

    TRACE(TRACE_DEBUG, "Populating TPC-C NEW_ORDER...\n");
    progress_reset(&progress);
    tpcc_new_order_tuple tup;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
        // clear the tuple
        memset(&tup, 0, sizeof(tup));
        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.NO_O_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.NO_D_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.NO_W_ID = atoi(tmp);


        // insert tuple into database
        // NEW_ORDER key composed (NO_O_ID, NO_D_ID, NO_W_ID) -- the whole record
        Dbt key(&tup, sizeof(tup));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update(&progress);
    }

    progress_done("NEW_ORDER");
}



void tpcc_parse_tbl_ORDER(Db* db, FILE* fd) {

    char linebuffer[MAX_LINE_LENGTH];
    unsigned long progress = 0;

    TRACE(TRACE_DEBUG, "Populating TPC-C ORDER...\n");
    progress_reset(&progress);

    tpcc_order_tuple tup;
    tpcc_order_tuple_key tk;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {

        // clear the tuple
        memset(&tup, 0, sizeof(tup));
        memset(&tk, 0, sizeof(tk));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.O_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.O_C_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.O_D_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.O_W_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.O_ENTRY_D = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.O_CARRIER_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.O_OL_CNT = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.O_ALL_LOCAL = atoi(tmp);

        // construct key
        // ORDER key composed (O_ID, O_C_ID, O_D_ID, O_W_ID)
        tk.O_ID = tup.O_ID;
        tk.O_C_ID = tup.O_C_ID;
        tk.O_D_ID = tup.O_D_ID;
        tk.O_W_ID = tup.O_W_ID;


        // insert tuple into database
        Dbt key(&tk, sizeof(tk));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update(&progress);
    }

    progress_done("ORDER");
}



void tpcc_parse_tbl_ORDERLINE (Db* db, FILE* fd) { 

    char linebuffer[MAX_LINE_LENGTH];
    unsigned long progress = 0;

    TRACE(TRACE_DEBUG, "Populating TPC-C ORDERLINE...\n");
    progress_reset(&progress);

    tpcc_orderline_tuple tup;
    tpcc_orderline_tuple_key tk;


    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {

        // clear the tuple & key
        memset(&tup, 0, sizeof(tup));
        memset(&tk, 0, sizeof(tk));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.OL_O_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.OL_D_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.OL_W_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.OL_NUMBER = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.OL_I_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.OL_SUPPLY_W_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.OL_DELIVERY_D = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.OL_QUANTITY = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.OL_AMOUNT = atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.OL_DIST_INFO,tmp);

        // consturct key
        // ORDERLINE key composed (OL_O_ID, OL_D_ID, OL_W_ID, OL_NUMBER)
        tk.OL_O_ID = tup.OL_O_ID;
        tk.OL_D_ID = tup.OL_D_ID;
        tk.OL_W_ID = tup.OL_W_ID;
        tk.OL_NUMBER = tup.OL_NUMBER;


        // insert tuple into database
        Dbt key(&tk, sizeof(tk));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);
       
        progress_update(&progress);
    }

    progress_done("tpcc_table");
}



void tpcc_parse_tbl_STOCK     (Db* db, FILE* fd) { 

    char linebuffer[MAX_LINE_LENGTH];
    unsigned long progress = 0;

    TRACE(TRACE_DEBUG, "Populating TPC-C STOCK...\n");
    progress_reset(&progress);

    tpcc_stock_tuple tup;
    tpcc_stock_tuple_key tk;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {

        // clear the tuple
        memset(&tup, 0, sizeof(tup));
        memset(&tk, 0, sizeof(tk));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.S_I_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.S_W_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.S_REMOTE_CNT = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.S_QUANTITY = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.S_ORDER_CNT = atoi(tmp);
        tmp = strtok(NULL, "|");
        tup.S_YTD = atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DIST_01,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DIST_02,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DIST_03,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DIST_04,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DIST_05,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DIST_06,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DIST_07,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DIST_08,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DIST_09,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DIST_10,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.S_DATA,tmp);

        // construct key
        // STOCK key composed (S_I_ID, S_W_ID)
        tk.S_I_ID = tup.S_I_ID;
        tk.S_W_ID = tup.S_W_ID;


        // insert tuple into database
        Dbt key(&tk, sizeof(tk));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update(&progress);
    }

    progress_done("tpcc_table");
}



void tpcc_parse_tbl_WAREHOUSE (Db* db, FILE* fd) {

    char linebuffer[MAX_LINE_LENGTH];
    unsigned long progress = 0;

    TRACE(TRACE_DEBUG, "Populating TPC-C WAREHOUSE...\n");
    progress_reset(&progress);

    tpcc_warehouse_tuple tup;
    tpcc_warehouse_tuple_key wk;

    while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {

        // clear the tuple
        memset(&tup, 0, sizeof(tup));
        memset(&wk, 0, sizeof(wk));

        // split line into tab separated parts
        char* tmp = strtok(linebuffer, "|");
        tup.W_ID = atoi(tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.W_NAME,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.W_STREET_1,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.W_STREET_2,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.W_CITY,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.W_STATE,tmp);
        tmp = strtok(NULL, "|");
        store_string(tup.W_ZIP,tmp);
        tmp = strtok(NULL, "|");
        tup.W_TAX = atof(tmp);
        tmp = strtok(NULL, "|");
        tup.W_YTD = atof(tmp);

        // construct key
        // WAREHOUSE key composed (W_ID)
        wk.W_ID = tup.W_ID;

        // insert tuple into database
        Dbt key(&wk, sizeof(wk));
        Dbt data(&tup, sizeof(tup));
        db->put(NULL, &key, &data, 0);

        progress_update(&progress);
    }

    progress_done("tpcc_table");
}


// EOF BDB-TABLE LOADING
////////////////////////////////////////////////////




////////////////////////////////////////////////////
// PER-TABLE ROW PARSERS




DEFINE_TPCC_PARSER(ITEM) {
    // clear the tuple
    record_t record;

    // split line into tab separated parts
    char* lasts;
    char* tmp = strtok_r(linebuffer, "|", &lasts);
    record.first.I_ID = atoi(tmp);
    record.second.I_ID = record.first.I_ID;

    // end of key

    tmp = strtok_r(NULL, "|", &lasts);
    record.second.I_IM_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.I_NAME,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.I_PRICE = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.I_DATA, tmp);
    
    return record;
}


DEFINE_TPCC_PARSER(NEW_ORDER) {
    // split line into tab separated parts
    record_t record;
    char* lasts;
    char* tmp = strtok_r(linebuffer, "|", &lasts);
    record.first.NO_O_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.NO_D_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.NO_W_ID = atoi(tmp);
    
    return record;
}


DEFINE_TPCC_PARSER(HISTORY) {
    // split line into tab separated parts
    record_t record;
    char* lasts;
    char* tmp = strtok_r(linebuffer, "|", &lasts);
    record.first.H_C_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.H_C_D_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.H_C_W_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.H_D_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.H_W_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.H_DATE = atoi(tmp);

    // end of key

    tmp = strtok_r(NULL, "|", &lasts);
    record.second.H_AMOUNT = atof(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.H_DATA,tmp);

    return record;
}


DEFINE_TPCC_PARSER(ORDER) {
    
    // split line into tab separated parts
    record_t record;
    char* lasts;
    char* tmp = strtok_r(linebuffer, "|", &lasts);
    record.first.O_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.O_C_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.O_D_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.O_W_ID = atoi(tmp);

    // end of key

    tmp = strtok_r(NULL, "|", &lasts);
    record.second.O_ENTRY_D = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.O_CARRIER_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.O_OL_CNT = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.O_ALL_LOCAL = atoi(tmp);

    return record;
}


DEFINE_TPCC_PARSER(ORDERLINE) {

    assert(false); // TODO
    
    // split line into tab separated parts
    record_t record;
    char* lasts;
    char* tmp = strtok_r(linebuffer, "|", &lasts);


    record.first.OL_O_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.OL_D_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.OL_W_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.OL_NUMBER = atoi(tmp);

    // end of key

    tmp = strtok_r(NULL, "|", &lasts);
    record.second.OL_I_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.OL_SUPPLY_W_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.OL_DELIVERY_D = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.OL_QUANTITY = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.OL_AMOUNT = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.OL_DIST_INFO,tmp);

    return record;
}


DEFINE_TPCC_PARSER(STOCK) {
    
    // split line into tab separated parts
    record_t record;
    char* lasts;
    char* tmp = strtok_r(linebuffer, "|", &lasts);
    record.first.S_I_ID = atoi(tmp);
    record.second.S_I_ID = record.first.S_I_ID;
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.S_W_ID = atoi(tmp);
    record.second.S_W_ID = record.first.S_W_ID;

    // end of key

    tmp = strtok_r(NULL, "|", &lasts);
    record.second.S_REMOTE_CNT = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.S_QUANTITY = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.S_ORDER_CNT = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.S_YTD = atoi(tmp);

    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST_01,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST_02,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST_03,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST_04,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST_05,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST_06,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST_07,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST_08,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST_09,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST_10,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DATA,tmp);

    return record;
}


DEFINE_TPCC_PARSER(DISTRICT) {
    
    // split line into tab separated parts
    record_t record;
    char* lasts;
    char* tmp = strtok_r(linebuffer, "|", &lasts);
    record.first.D_ID = atoi(tmp);
    record.second.D_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.D_W_ID = atoi(tmp);
    record.second.D_W_ID = atoi(tmp);

    // end of key

    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.D_NAME,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.D_STREET_1,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.D_STREET_2,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.D_CITY,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.D_STATE,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.D_ZIP,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.D_TAX = atof(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.D_YTD = atof(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.D_NEXT_O_ID = atoi(tmp);

    return record;
}


DEFINE_TPCC_PARSER(WAREHOUSE) {
    
    // split line into tab separated parts
    record_t record;
    char* lasts;
    char* tmp = strtok_r(linebuffer, "|", &lasts);
    record.first.W_ID = atoi(tmp);
    record.second.W_ID = atoi(tmp);

    // end of key

    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.W_NAME,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.W_STREET_1,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.W_STREET_2,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.W_CITY,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.W_STATE,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.W_ZIP,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.W_TAX = atof(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.W_YTD = atof(tmp);

    return record;
}


DEFINE_TPCC_PARSER(CUSTOMER) {
    
    // split line into tab separated parts
    record_t record;
    char* lasts;
    char* tmp = strtok_r(linebuffer, "|", &lasts);
    record.first.C_C_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.C_D_ID = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.first.C_W_ID = atoi(tmp);

    // end of key

    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_FIRST,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_MIDDLE,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_LAST,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_STREET_1,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_STREET_2,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_CITY,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_STATE,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_ZIP,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_PHONE,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.C_SINCE = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_CREDIT,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.C_CREDIT_LIM = atof(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.C_DISCOUNT = atof(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.C_BALANCE = atof(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.C_YTD_PAYMENT = atof(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.C_LAST_PAYMENT = atof(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    record.second.C_PAYMENT_CNT = atoi(tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_DATA_1,tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.C_DATA_2,tmp);

    return record;
}



// EOF PER-TABLE ROW PARSERS
////////////////////////////////////////////////////



EXIT_NAMESPACE(tpcc);

