/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_datastr.h
 *
 *  @brief The TPC-C data structures for the creation of the data files
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __TPCC_DBGEN_DATA_STR_H
#define __TPCC_DBGEN_DATA_STR_H

#include <sys/types.h>

struct in_neword_struct {
  struct  in_items_struct {
    int32_t    s_OL_I_ID;
    int32_t   s_OL_SUPPLY_W_ID;
    int16_t   s_OL_QUANTITY;
  } in_item[15];
  int32_t    s_C_ID;
  int32_t   s_W_ID;
  int16_t   s_D_ID;
  int16_t   s_O_OL_CNT;
  int16_t   s_all_local;
  int16_t   duplicate_items;
  int64_t s_O_ENTRY_D_time;
};


struct out_neword_struct {
  struct  items_struct {
    double  s_I_PRICE;
    double  s_OL_AMOUNT;
    int16_t   s_S_QUANTITY;
    char    s_I_NAME[25];
    char    s_brand_generic;
  } item[15];
  int64_t s_O_ENTRY_D_time;
  double  s_W_TAX;
  double  s_D_TAX;
  double  s_C_DISCOUNT;
  double  s_total_amount;
  int32_t    s_O_ID;
  int16_t   s_O_OL_CNT;
  int16_t   s_transtatus;
  int16_t   deadlocks;
  char    s_C_LAST[17];
  char    s_C_CREDIT[3];
};


struct in_payment_struct {
  double  s_H_AMOUNT;
  int32_t    s_C_ID;
  int32_t   s_W_ID;
  int16_t   s_D_ID;
  int16_t   s_C_D_ID;
  int32_t   s_C_W_ID;
  int64_t    s_H_DATE_time;
  char    s_C_LAST[17];
};


struct out_payment_struct {
  int64_t    s_H_DATE_time;
  double  s_C_CREDIT_LIM;
  double  s_C_DISCOUNT;
  double  s_C_BALANCE;
  int32_t    s_C_ID;
  char    s_W_STREET_1[21];
  char    s_W_STREET_2[21];
  char    s_W_CITY[21];
  char    s_W_STATE[3];
  char    s_W_ZIP[10];
  char    s_D_STREET_1[21];
  char    s_D_STREET_2[21];
  char    s_D_CITY[21];
  char    s_D_STATE[3];
  char    s_D_ZIP[10];
  char    s_C_FIRST[17];
  char    s_C_MIDDLE[3];
  char    s_C_LAST[17];
  char    s_C_STREET_1[21];
  char    s_C_STREET_2[21];
  char    s_C_CITY[21];
  char    s_C_STATE[3];
  char    s_C_ZIP[10];
  char    s_C_PHONE[17];
  int64_t s_C_SINCE_time;
  char    s_C_CREDIT[3];
  char    s_C_DATA[201];
  int16_t   s_transtatus;
  int16_t   deadlocks;
};


struct in_ordstat_struct {
  int32_t     s_C_ID;
  int32_t    s_W_ID;
  int16_t    s_D_ID;
  char     s_C_LAST[17];
};


struct out_ordstat_struct {
  double   s_C_BALANCE;
  int64_t s_O_ENTRY_D_time;
  int32_t     s_C_ID;
  int32_t     s_O_ID;
  int16_t    s_O_CARRIER_ID;
  int16_t    s_ol_cnt;
  struct   oitems_struct {
    double s_OL_AMOUNT;
    int64_t s_OL_DELIVERY_D_time;
    int32_t   s_OL_I_ID;
    int32_t  s_OL_SUPPLY_W_ID;
    int16_t  s_OL_QUANTITY;
  } item[15];
  char     s_C_FIRST[17];
  char     s_C_MIDDLE[3];
  char     s_C_LAST[17];
  int16_t    s_transtatus;
  int16_t    deadlocks;
};


struct in_delivery_struct {
  int64_t s_O_DELIVERY_D_time;
  int32_t   s_W_ID;
  int16_t   s_O_CARRIER_ID;
};



struct out_delivery_struct {
  int32_t    s_O_ID[10];
  int16_t   s_transtatus;
  int16_t   deadlocks;
};



struct in_stocklev_struct {
  int32_t   s_W_ID;
  int16_t   s_D_ID;
  int16_t   s_threshold;
};



struct out_stocklev_struct {
  int32_t    s_low_stock;
  int16_t   s_transtatus;
  int16_t   deadlocks;
};


#endif
