/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

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

#include "workload/tpcc/common/tpcc_tbl_parsers.h"

ENTER_NAMESPACE(tpcc);


/* definitions of exported functions */



////////////////////////////////////////////////////
// PER-TABLE ROW PARSERS




DEFINE_TPCC_PARSER(ITEM) {
    // clear the tuple
    record_t record;
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

    // the whole record is the key
    
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
    FILL_STRING(record.second.S_DIST[0],tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST[1],tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST[2],tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST[3],tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST[4],tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST[5],tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST[6],tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST[7],tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST[8],tmp);
    tmp = strtok_r(NULL, "|", &lasts);
    FILL_STRING(record.second.S_DIST[9],tmp);
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

