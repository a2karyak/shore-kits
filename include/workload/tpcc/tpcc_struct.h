/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_struct.h
 *
 *  @brief Exports structures that we store/load from BerkeleyDB
 *         for the TPC-C database
 *
 *  @author Ippokratis Pandis (ipandis)
 *
 *  Comment field:
 *
 */

#ifndef __TPCC_STRUCT_H
#define __TPCC_STRUCT_H

#include <cstdlib>
#include <unistd.h>
#include <sys/time.h>
#include "util.h"


ENTER_NAMESPACE(tpcc);


/* use this for allocation of NUL-terminated strings */

#define STRSIZE(x)(x+1)


/* exported structures */


// FIXME (ip): We may need to add some padding to pretend row-level locking
//             BDB does page-level locking. 
//             The hot structures, such as WAREHOUSE and DISTRICT are the
//             candidates for padding.


struct tpcc_customer_tuple {
    int C_C_ID;
    int C_D_ID;
    int C_W_ID;
    char C_FIRST    [STRSIZE(16)];
    char C_MIDDLE   [STRSIZE(2)];
    char C_LAST     [STRSIZE(16)];
    char C_STREET_1 [STRSIZE(20)];
    char C_STREET_2 [STRSIZE(20)];
    char C_CITY     [STRSIZE(20)];
    char C_STATE    [STRSIZE(2)];
    char C_ZIP      [STRSIZE(9)];
    char C_PHONE    [STRSIZE(16)];
    int C_SINCE;
    char C_CREDIT   [STRSIZE(2)];
    decimal C_CREDIT_LIM;
    decimal C_DISCOUNT;
    decimal C_BALANCE;
    decimal C_YTD_PAYMENT;
    decimal C_LAST_PAYMENT;
    int C_PAYMENT_CNT;
    char C_DATA_1   [STRSIZE(250)];
    char C_DATA_2   [STRSIZE(250)];    
};


// FIXME (ip): We may need padding to the DISTRICT

struct tpcc_district_tuple {
    int D_ID;
    int D_W_ID;
    char D_NAME     [STRSIZE(10)];
    char D_STREET_1 [STRSIZE(20)];
    char D_STREET_2 [STRSIZE(20)];
    char D_CITY     [STRSIZE(20)];
    char D_STATE    [STRSIZE(2)];
    char D_ZIP      [STRSIZE(10)];
    decimal D_TAX;
    decimal D_YTD;
    int D_NEXT_O_ID;
};



struct tpcc_history_tuple {
    int H_C_ID;
    int H_C_D_ID;
    int H_C_W_ID;
    int H_D_ID;
    int H_W_ID;
    int H_DATE;
    decimal H_AMOYNT;
    char H_DATA [STRSIZE(25)];
};



struct tpcc_item_tuple {
    int I_ID;
    int I_IM_ID;
    char I_NAME [STRSIZE(24)];
    int I_PRICE;
    char I_DATA [STRSIZE(50)];
};



struct tpcc_new_order_tuple {
    int NO_O_ID;
    int NO_D_ID;
    int NO_W_ID;
};


struct tpcc_order_tuple {
    int O_ID;
    int O_C_ID;
    int O_D_ID;
    int O_W_ID;
    int O_ENTRY_D;
    int O_CARRIER_ID;
    int O_OL_CNT;
    int O_ALL_LOCAL;
};


struct tpcc_orderline_tuple {
    int OL_O_ID;
    int OL_D_ID;
    int OL_W_ID;
    int OL_NUMBER;
    int OL_I_ID;
    int OL_SUPPLY_W_ID;
    int OL_DELIVERY_D;
    int OL_QUANTITY;
    int OL_AMOYNT;
    char OL_DIST_INFO [STRSIZE(25)];
};



struct tpcc_stock_tuple {
    int S_I_ID;
    int S_W_ID;
    int S_REMOTE_CNT;
    int S_QUANTITY;
    int S_ORDER_CNT;
    int S_YTD;
    char S_DIST_01 [STRSIZE(24)];
    char S_DIST_02 [STRSIZE(24)];
    char S_DIST_03 [STRSIZE(24)];
    char S_DIST_04 [STRSIZE(24)];
    char S_DIST_05 [STRSIZE(24)];
    char S_DIST_06 [STRSIZE(24)];
    char S_DIST_07 [STRSIZE(24)];
    char S_DIST_08 [STRSIZE(24)];
    char S_DIST_09 [STRSIZE(24)];
    char S_DIST_10 [STRSIZE(24)];
    char S_DATA    [STRSIZE(50)];
};


// FIXME (ip): We may need padding to the WAREHOUSE

struct tpcc_warehouse_tuple {
    int W_ID;
    char W_NAME     [STRSIZE(20)];
    char W_STREET_1 [STRSIZE(20)];
    char W_STREET_2 [STRSIZE(20)];
    char W_CITY     [STRSIZE(20)];
    char W_STATE    [STRSIZE(2)];
    char W_ZIP      [STRSIZE(9)];
    decimal W_TAX;
    decimal W_YTD;
};


EXIT_NAMESPACE(tpcc);

#endif
