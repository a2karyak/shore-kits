/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_const.h
 *
 *  @brief:  Constants needed by the TPC-C kit
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_TPCC_CONST_H
#define __SHORE_TPCC_CONST_H


#include "util/namespace.h"


ENTER_NAMESPACE(shore);


/*
 * indices created on the tables are:
 *
 * 1. unique index on order_line(ol_w_id,ol_d_id,ol_o_id,ol_number)
 * 2. unique index on stock(s_w_id,s_i_id)
 * 3. unique index on customer(c_w_id,c_d_id,c_id)
 * 4. index on customer(c_w_id,c_d_id,c_last,c_first,c_id)
 * 5. unique index on order(o_w_id,o_d_id,o_id)
 * 6. unique index on order(o_w_id,o_d_id,o_c_id,o_id) desc
 * 7. unique index on item(i_id)
 * 8. unique index on new_order(no_w_id,no_d_id,no_o_id)
 * 9. unique index on district(d_id,d_w_id)
 * 10. unique index on warehouse(w_id)
 */


/* parameters used to estimate the required storage
 * set it to 4 or 5 for extremely small databases and 2 for
 * databases with standard scales. */
#define STORAGE_FACTOR     2
 
/* scale factor for tpcc benchmark */

/* Scale-down version 
#define DISTRICTS_PER_WAREHOUSE                 4
#define CUSTOMERS_PER_DISTRICT                  120
#define ITEMS                                   4000
#define STOCK_PER_WAREHOUSE                     ITEMS
// #define MIN_OL_PER_ORDER                        4
// #define MAX_OL_PER_ORDER                        8
#define MIN_OL_PER_ORDER                        5
#define MAX_OL_PER_ORDER                        15
#define NU_ORDERS_PER_DISTRICT                  36
*/
 
/* Scale-down version 2) 
#define DISTRICTS_PER_WAREHOUSE                 4
#define CUSTOMERS_PER_DISTRICT                  300
#define ITEMS                                   10000
#define STOCK_PER_WAREHOUSE                     ITEMS
#define MIN_OL_PER_ORDER                        4
#define MAX_OL_PER_ORDER                        8
#define NU_ORDERS_PER_DISTRICT                  90
*/

/*  standard scale 
*/
#define DISTRICTS_PER_WAREHOUSE                 10
#define CUSTOMERS_PER_DISTRICT                  3000
#define ITEMS                                   100000
#define STOCK_PER_WAREHOUSE                     ITEMS
#define MIN_OL_PER_ORDER                        5
#define MAX_OL_PER_ORDER                        15
#define NU_ORDERS_PER_DISTRICT                  900

#define MAX_TABLENAM_LENGTH       20
#define MAX_RECORD_LENGTH        512

/* number of fields for tables */
#define TPCC_WAREHOUSE_FCOUNT   9
#define TPCC_DISTRICT_FCOUNT   11
#define TPCC_CUSTOMER_FCOUNT   21
#define TPCC_HISTORY_FCOUNT     8
#define TPCC_NEW_ORDER_FCOUNT   3
#define TPCC_ORDER_FCOUNT       8
#define TPCC_ORDER_LINE_FCOUNT 10
#define TPCC_ITEM_FCOUNT        5
#define TPCC_STOCK_FCOUNT      17



/* ----------------- */
/* --- TPC-C MIX --- */
/* ----------------- */

#define   XCT_NEW_ORDER    1
#define   XCT_PAYMENT      2
#define   XCT_ORDER_STATUS 3
#define   XCT_DELIVERY     4
#define   XCT_STOCK_LEVEL  5


/* --- probabilities for the TPC-C MIX --- */

const int prob_neworder = 45;
const int prob_payment = 43;
const int prob_order_status = 4;
const int prob_delivery = 4;
const int prob_stock_level = 4;





EXIT_NAMESPACE(shore);

#endif /* __SHORE_TPCC_CONST_H */
