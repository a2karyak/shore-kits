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

   int ware_num = 0 ;    
   int dist_num = 0 ;    
   int cust_num = 0 ;    
   int ord_num = 0 ;     
   int ordr_carrier_id;
   int ordr_ol_cnt;
   int oline_ol_num;
   int oline_item_num;

   int oline_amount;
   char oline_dist_info[25];
   Int64 nulltmstmp = 0;
   Int64 currtmstmp;    
   int ware_num = 0 ;    
   int dist_num = 0 ;    
   int cust_num = 0 ;    
   int ord_num = 0 ;     
   int ordr_carrier_id;
   int ordr_ol_cnt;
   int oline_ol_num;
   int oline_item_num;

   int oline_amount;
   char oline_dist_info[25];
   Int64 nulltmstmp = 0;
   Int64 currtmstmp;    
};


/*
enum tpch_l_shipmode {
    REG_AIR,
    AIR,
    RAIL,
    TRUCK,
    MAIL,
    FOB,
    SHIP,
    END_SHIPMODE
};
*/


EXIT_NAMESPACE(tpcc);

#endif
