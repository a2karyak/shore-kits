/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-MT -- Multi-threaded port of the SHORE storage manager
   
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

/** @file:   shore_tpcc_schema.h
 *
 *  @brief:  Implementation of the TPC-C tables
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */


#include "workload/tpcc/shore_tpcc_schema.h"


using namespace shore;
using namespace tpcc;


/*********************************************************************
 *
 * loading utility
 *
 *********************************************************************/

/*
 * There are two methods for each table:
 *
 * 1. read_tuple_from_line(), which reads a tuple from a buffer.
 *
 * 2. random(), which creates a random tuple.
 *
 */



const int MAX_INT_LEN  = 60;
const int MAX_LONG_LEN = 550;


/*********************************************************************
 *
 * class warehouse_t
 *
 *********************************************************************/

bool warehouse_t::read_tuple_from_line(table_row_t& tuple, char* buf)
{
    static parse_tpcc_WAREHOUSE parser;
    parse_tpcc_WAREHOUSE::record_t record;

    /* 1. parse row to a record_t object */
    record = parser.parse_row(buf);

    /* 2. convert the object to a table_row_t */

    /* id */
    tuple.set_value(0, record.second.W_ID);
    /* name */
    tuple.set_value(1, record.second.W_NAME);
    /* street1 */
    tuple.set_value(2, record.second.W_STREET_1);
    /* street2 */
    tuple.set_value(3, record.second.W_STREET_2);
    /* city */
    tuple.set_value(4, record.second.W_CITY);
    /* state */
    tuple.set_value(5, record.second.W_STATE);
    /* zip */
    tuple.set_value(6, record.second.W_ZIP);
    /* tax */
    tuple.set_value(7, record.second.W_TAX.to_double());
    /* ytd */
    tuple.set_value(8, record.second.W_YTD.to_double());

    return (true);
}


/*********************************************************************
 *
 * class district_t
 *
 *********************************************************************/

bool district_t::read_tuple_from_line(table_row_t& tuple, char* buf)
{
    static parse_tpcc_DISTRICT parser;
    parse_tpcc_DISTRICT::record_t record;

    /* 1. parse row to a record_t object */
    record = parser.parse_row(buf);

    /* 2. convert the object to a table_row_t */

    /* id */
    tuple.set_value(0, record.second.D_ID);
    /* w_id */
    tuple.set_value(1, record.second.D_W_ID);
    /* name */
    tuple.set_value(2, record.second.D_NAME);
    /* street1 */
    tuple.set_value(3, record.second.D_STREET_1);
    /* street2 */
    tuple.set_value(4, record.second.D_STREET_2);
    /* city */
    tuple.set_value(5, record.second.D_CITY);
    /* state */
    tuple.set_value(6, record.second.D_STATE);
    /* zip */
    tuple.set_value(7, record.second.D_ZIP);
    /* tax */
    tuple.set_value(8, record.second.D_TAX.to_double());
    /* ytd */
    tuple.set_value(9, record.second.D_YTD.to_double());
    /* next_o_id */
    tuple.set_value(10, record.second.D_NEXT_O_ID);

    return (true);
}


/*********************************************************************
 *
 * class customer_t
 *
 *********************************************************************/

bool customer_t::read_tuple_from_line(table_row_t& tuple, char* buf)
{
    static parse_tpcc_CUSTOMER parser;
    parse_tpcc_CUSTOMER::record_t record;

    /* 1. parse row to a record_t object */
    record = parser.parse_row(buf);

    /* 2. convert the object to a table_row_t */

    /* id */
    tuple.set_value(0, record.first.C_C_ID);
    /* d_id */
    tuple.set_value(1, record.first.C_D_ID);
    /* w_id */
    tuple.set_value(2, record.first.C_W_ID);
    /* first */
    tuple.set_value(3, record.second.C_FIRST);
    /* middle */
    tuple.set_value(4, record.second.C_MIDDLE);
    /* last */
    tuple.set_value(5, record.second.C_LAST);
    /* street1 */
    tuple.set_value(6, record.second.C_STREET_1);
    /* street2 */
    tuple.set_value(7, record.second.C_STREET_2);
    /* city */
    tuple.set_value(8, record.second.C_CITY);
    /* state */
    tuple.set_value(9, record.second.C_STATE);
    /* zip */
    tuple.set_value(10, record.second.C_ZIP);
    /* phone */
    tuple.set_value(11, record.second.C_PHONE);
    /* since */
    tuple.set_value(12, record.second.C_SINCE);
    /* credit */
    tuple.set_value(13, record.second.C_CREDIT);
    /* credit_limit */
    tuple.set_value(14, record.second.C_CREDIT_LIM.to_double());
    /* discount */
    tuple.set_value(15, record.second.C_DISCOUNT.to_double());
    /* balance */
    tuple.set_value(16, record.second.C_BALANCE.to_double());
    /* ytd_payment */
    tuple.set_value(17, record.second.C_YTD_PAYMENT.to_double());
    /* payment_cnt */
    tuple.set_value(18, record.second.C_LAST_PAYMENT.to_double());
    /* delivery_cnt */
    tuple.set_value(19, record.second.C_PAYMENT_CNT);
    /* data_1 */
    tuple.set_value(20, record.second.C_DATA_1);
    /* data_2 */
    tuple.set_value(21, record.second.C_DATA_2);

    return (true);
}


/*********************************************************************
 *
 * class history_t
 *
 *********************************************************************/

bool history_t::read_tuple_from_line(table_row_t& tuple, char* buf)
{
    static parse_tpcc_HISTORY parser;
    parse_tpcc_HISTORY::record_t record;

    /* 1. parse row to a record_t object */
    record = parser.parse_row(buf);

    /* 2. convert the object to a table_row_t */

    /* c_id */
    tuple.set_value(0, record.first.H_C_ID);
    /* c_d_id */
    tuple.set_value(1, record.first.H_C_D_ID);
    /* c_w_id */
    tuple.set_value(2, record.first.H_C_W_ID);
    /* d_id */
    tuple.set_value(3, record.first.H_D_ID);
    /* w_id */
    tuple.set_value(4, record.first.H_W_ID);
    /* date */
    tuple.set_value(5, record.first.H_DATE);
    /* amount */
    tuple.set_value(6, record.second.H_AMOUNT.to_double());
    /* data */
    tuple.set_value(7, record.second.H_DATA);

    return (true);
}


/*********************************************************************
 *
 * class new_order_t
 *
 *********************************************************************/

bool new_order_t::read_tuple_from_line(table_row_t& tuple, char* buf)
{
    static parse_tpcc_NEW_ORDER parser;
    parse_tpcc_NEW_ORDER::record_t record;

    /* 1. parse row to a record_t object */
    record = parser.parse_row(buf);

    /* 2. convert the object to a table_row_t */

    /* o_id */
    tuple.set_value(0, record.first.NO_O_ID);
    /* d_id */
    tuple.set_value(1, record.first.NO_D_ID);
    /* w_id */
    tuple.set_value(2, record.first.NO_W_ID);

    return (true);
}


/*********************************************************************
 *
 * class order_t
 *
 *********************************************************************/

bool order_t::read_tuple_from_line(table_row_t& tuple, char* buf)
{
    static parse_tpcc_ORDER parser;
    parse_tpcc_ORDER::record_t record;

    /* 1. parse row to a record_t object */
    record = parser.parse_row(buf);

    /* 2. convert the object to a table_row_t */

    /* id */
    tuple.set_value(0, record.first.O_ID);
    /* d_id */
    tuple.set_value(1, record.first.O_C_ID);
    /* w_id */
    tuple.set_value(2, record.first.O_D_ID);
    /* c_id */
    tuple.set_value(3, record.first.O_W_ID);
    /* entry_d */
    tuple.set_value(4, record.second.O_ENTRY_D);
    /* carrier_id */
    tuple.set_value(5, record.second.O_CARRIER_ID);
    /* ol_cnt */
    tuple.set_value(6, record.second.O_OL_CNT);
    /* all_local */
    tuple.set_value(7, record.second.O_ALL_LOCAL);

    return (true);
}


/*********************************************************************
 *
 * class order_line_t
 *
 *********************************************************************/

bool order_line_t::read_tuple_from_line(table_row_t& tuple, char* buf)
{
    static parse_tpcc_ORDERLINE parser;
    parse_tpcc_ORDERLINE::record_t record;

    /* 1. parse row to a record_t object */
    record = parser.parse_row(buf);

    /* 2. convert the object to a table_row_t */

    /* o_id */
    tuple.set_value(0, record.first.OL_O_ID);
    /* d_id */
    tuple.set_value(1, record.first.OL_D_ID);
    /* w_id */
    tuple.set_value(2, record.first.OL_W_ID);
    /* number */
    tuple.set_value(3, record.first.OL_NUMBER);
    /* i_id */
    tuple.set_value(4, record.second.OL_I_ID);
    /* supply_w_id */
    tuple.set_value(5, record.second.OL_SUPPLY_W_ID);
    /* delivery */
    tuple.set_value(6, record.second.OL_DELIVERY_D);
    /* quantity */
    tuple.set_value(7, record.second.OL_QUANTITY);
    /* amount */
    tuple.set_value(8, record.second.OL_AMOUNT);
    /* dist_info */
    tuple.set_value(9, record.second.OL_DIST_INFO);

    return (true);
}


/*********************************************************************
 *
 * class item_t
 *
 *********************************************************************/

bool item_t::read_tuple_from_line(table_row_t& tuple, char* buf)
{
    static parse_tpcc_ITEM parser;
    parse_tpcc_ITEM::record_t record;

    /* 1. parse row to a record_t object */
    record = parser.parse_row(buf);

    /* 2. convert the object to a table_row_t */

    /* id */
    tuple.set_value(0, record.second.I_ID);
    /* im_id */
    tuple.set_value(1, record.second.I_IM_ID);
    /* name */
    tuple.set_value(2, record.second.I_NAME);
    /* price */
    tuple.set_value(3, record.second.I_PRICE);
    /* data */
    tuple.set_value(4, record.second.I_DATA);

    return (true);
}


/*********************************************************************
 *
 * class stock_t
 *
 *********************************************************************/

bool stock_t::read_tuple_from_line(table_row_t& tuple, char* buf)
{
    static parse_tpcc_STOCK parser;
    parse_tpcc_STOCK::record_t record;

    /* 1. parse row to a record_t object */
    record = parser.parse_row(buf);

    /* 2. convert the object to a table_row_t */

    /* i_id */
    tuple.set_value(0, record.second.S_I_ID);
    /* w_id */
    tuple.set_value(1, record.second.S_W_ID);
    /* quantity */
    tuple.set_value(2, record.second.S_REMOTE_CNT);
    /* dist0 */
    tuple.set_value(3, record.second.S_QUANTITY);
    /* dist1 */
    tuple.set_value(4, record.second.S_ORDER_CNT);
    /* dist2 */
    tuple.set_value(5, record.second.S_YTD);
    /* dist3 */
    tuple.set_value(6, record.second.S_DIST[0]);
    /* dist4 */
    tuple.set_value(7, record.second.S_DIST[1]);
    /* dist5 */
    tuple.set_value(8, record.second.S_DIST[2]);
    /* dist6 */
    tuple.set_value(9, record.second.S_DIST[3]);
    /* dist7 */
    tuple.set_value(10, record.second.S_DIST[4]);
    /* dist8 */
    tuple.set_value(11, record.second.S_DIST[5]);
    /* dist9 */ 
    tuple.set_value(12, record.second.S_DIST[6]);
    /* ytd */
    tuple.set_value(13, record.second.S_DIST[7]);
    /* order_cnt */
    tuple.set_value(14, record.second.S_DIST[8]);
    /* remote_cnt */
    tuple.set_value(15, record.second.S_DIST[9]);
    /* data */
    tuple.set_value(16, record.second.S_DATA);

    return (true);
}
