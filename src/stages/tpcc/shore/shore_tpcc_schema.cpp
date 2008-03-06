/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_schema.h
 *
 *  @brief:  Implementation of the TPC-C tables
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */


#include "stages/tpcc/shore/shore_tpcc_schema.h"


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

void  warehouse_t::random(table_row_t* ptuple, int w_id)
{
    char    int_string[MAX_INT_LEN];
    double  double_number;

    assert (false); // (ip) deprecated

    /* id */
    ptuple->set_value(0, w_id);

    /* name */
    _tpccrnd.random_a_string(int_string, 6, 10);
    ptuple->set_value(1, int_string);

    /* street1 */
    _tpccrnd.random_a_string(int_string, 10, 20);
    ptuple->set_value(2, int_string);

    /* street2 */
    _tpccrnd.random_a_string(int_string, 10, 20);
    ptuple->set_value(3, int_string);

    /* city */
    _tpccrnd.random_a_string(int_string, 10, 20);
    ptuple->set_value(4, int_string);

    /* state */
    _tpccrnd.random_a_string(int_string, 2, 2);
    ptuple->set_value(5, int_string);

    /* zip */
    _tpccrnd.random_n_string(int_string, 4, 4);
    strcat(int_string, "11111");
    ptuple->set_value(6, int_string);

    /* tax */
    double_number = ((double) _tpccrnd.random_integer(0,2000))/(double)10000.0;
    ptuple->set_value(7, double_number);

    /* ytd */
    ptuple->set_value(8, (double)300000);
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

void district_t::random(table_row_t* ptuple, int id, int w_id, int next_o_id)
{
    char   int_string[MAX_INT_LEN] = "test";
    double double_number;

    assert (false); // (ip) deprecated

    /* id */
    ptuple->set_value(0, id);

    /* w_id */
    ptuple->set_value(1, w_id);

    /* name */
    _tpccrnd.random_a_string(int_string, 6, 10);
    ptuple->set_value(2, int_string);

    /* street1 */
    _tpccrnd.random_a_string(int_string, 10, 20);
    ptuple->set_value(3, int_string);

    /* street2 */
    _tpccrnd.random_a_string(int_string, 10, 20);
    ptuple->set_value(4, int_string);

    /* city */
    _tpccrnd.random_a_string(int_string, 10, 20);
    ptuple->set_value(5, int_string);

    /* state */
    _tpccrnd.random_a_string(int_string, 2, 2);
    ptuple->set_value(6, int_string);

    /* zip */
    _tpccrnd.random_n_string(int_string, 4, 4);
    strcat(int_string, "11111");
    ptuple->set_value(7, int_string);

    /* tax */
    double_number = ((double)_tpccrnd.random_integer(0, 2000))/(double)10000.0;
    ptuple->set_value(8, double_number);

    /* ytd */
    ptuple->set_value(9, (double)300000);

    /* next_o_id */
    ptuple->set_value(10, next_o_id);
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

void  customer_t::random(table_row_t* ptuple, int id, int d_id, int w_id)
{

    assert (false); // (ip) modified the schema
    assert (false); // (ip) deprecated


    double  double_number;
    char    string[MAX_LONG_LEN];
    timestamp_t  time;

    /* id */
    ptuple->set_value(0, id);

    /* d_id */
    ptuple->set_value(1, d_id);

    /* w_id */
    ptuple->set_value(2, w_id);

    /* first */
    _tpccrnd.random_a_string(string, 8, 16);
    ptuple->set_value(3, string);

    /* middle */
    ptuple->set_value(4, string);

    /* last */
    _tpccrnd.random_last_name(string, id);
    ptuple->set_value(5, string);

    /* street1 */
    _tpccrnd.random_a_string(string, 10, 20);
    ptuple->set_value(6, string);

    /* street2 */
    _tpccrnd.random_a_string(string, 10, 20);
    ptuple->set_value(7, string);

    /* city */
    _tpccrnd.random_a_string(string, 10, 20);
    ptuple->set_value(8, string);

    /* state */
    _tpccrnd.random_a_string(string, 2, 2);
    ptuple->set_value(9, string);

    /* zip */
    _tpccrnd.random_n_string(string, 4, 4);
    strcat(string, "11111");
    ptuple->set_value(10, string);

    /* phone */
    _tpccrnd.random_n_string(string, 16, 16);
    ptuple->set_value(11, string);

    /* since */
    ptuple->set_value(12, time);

    /* credit */
    if (_tpccrnd.random_float_val_return(0.0, 100.0) < 10.2)
	strcpy(string, "BC");
    else strcpy(string, "GC");
    ptuple->set_value(13, string);

    /* credit_limit */
    ptuple->set_value(14, (double)50000);

    /* discount */
    double_number = ((double)_tpccrnd.random_integer(0, 5000))/(double)10000.0;
    ptuple->set_value(15, double_number);

    /* balance */
    ptuple->set_value(16, (double)0);

    /* ytd_payment */
    ptuple->set_value(17, (double)-10);

    /* payment_cnt */
    ptuple->set_value(18, (int)10);

    /* delivery_cnt */
    ptuple->set_value(19, (int)1);

    /* data */
    _tpccrnd.random_a_string(string, 300, 500);
    ptuple->set_value(20, string);
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

void  history_t::random(table_row_t* ptuple, int c_id, int c_d_id, int c_w_id)
{
    char  string[MAX_INT_LEN];
    timestamp_t  time;

    assert (false); // (ip) Modified schema
    assert (false); // (ip) deprecated

    /* c_id */
    ptuple->set_value(0, c_id);

    /* c_d_id */
    ptuple->set_value(1, c_d_id);

    /* c_w_id */
    ptuple->set_value(2, c_w_id);

    /* d_id */
    ptuple->set_value(3, c_d_id);

    /* w_id */
    ptuple->set_value(4, c_w_id);

    /* date */
    ptuple->set_value(5, time);

    /* amount */
    ptuple->set_value(6, 1000);

    /* data */
    _tpccrnd.random_a_string(string, 12, 24);
    ptuple->set_value(7, string);
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

void new_order_t::random(table_row_t* ptuple, int id, int d_id, int w_id)
{

    assert (false); // (ip) deprecated

    /* o_id */
    ptuple->set_value(0, id);

    /* d_id */
    ptuple->set_value(1, d_id);

    /* w_id */
    ptuple->set_value(2, w_id);
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

void order_t::random(table_row_t* ptuple, int id, int c_id, int d_id, 
                     int w_id, int ol_cnt)
{
    int int_num;
    timestamp_t  time;
    char   string[MAX_INT_LEN];

    assert (false); // (ip) modified schema
    assert (false); // (ip) deprecated


    /* id */
    ptuple->set_value(0, id);

    /* d_id */
    ptuple->set_value(1, d_id);

    /* w_id */
    ptuple->set_value(2, w_id);

    /* c_id */
    ptuple->set_value(3, c_id);

    /* entry_d */
    ptuple->set_value(4, time);

    /* carrier_id */
    if (id < 2101) int_num = _tpccrnd.random_integer(1,10);
    else int_num = 0;
    ptuple->set_value(5, int_num);

    /* ol_cnt */
    ptuple->set_value(6, ol_cnt);

    /* all_local */
    ptuple->set_value(7, 1);
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

void order_line_t::random(table_row_t* ptuple, int id,
			  int d_id,
			  int w_id,
			  int ol_index,
			  bool delivery)
{
    char   string[MAX_INT_LEN];
    timestamp_t  time;

    assert (false); // (ip) deprecated

    /* o_id */
    ptuple->set_value(0, id);

    /* d_id */
    ptuple->set_value(1, d_id);

    /* w_id */
    ptuple->set_value(2, w_id);

    /* number */
    ptuple->set_value(3, ol_index);

    /* i_id */
    ptuple->set_value(4, _tpccrnd.random_integer(1, ITEMS));

    /* supply_w_id */
    ptuple->set_value(5, w_id);

    /* delivery */
    if (delivery) {
	ptuple->set_value(6, time);
    }
    else 
        ptuple->set_null(6);

    /* quantity */
    ptuple->set_value(7, (int)5);

    /* amount */
    ptuple->set_value(8, _tpccrnd.random_integer(1, 999999));

    /* dist_info */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(9, string);
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

void item_t::random(table_row_t* ptuple, int id)
{
    char   string[MAX_INT_LEN];
    int    hit;

    assert (false); // (ip) deprecated

    /* id */
    ptuple->set_value(0, id);

    /* im_id */
    ptuple->set_value(1, (int)_tpccrnd.random_integer(1, 10000));

    /* name */
    _tpccrnd.random_a_string(string, 14, 24);
    ptuple->set_value(2, string);

    /* price */
    ptuple->set_value(3, _tpccrnd.random_integer(100, 10000));

    /* data */
    _tpccrnd.string_with_original(string, 26, 50, 10.5, &hit);
    ptuple->set_value(4, string);
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

void stock_t::random(table_row_t* ptuple, int id, int w_id)
{
    char   string[MAX_INT_LEN];
    int    hit;

    assert (false); // (ip) Modified schema
    assert (false); // (ip) deprecated

    /* i_id */
    ptuple->set_value(0, id);

    /* w_id */
    ptuple->set_value(1, w_id);

    /* quantity */
    ptuple->set_value(2, (int)_tpccrnd.random_integer(10, 100));

    /* dist0 */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(3, string);

    /* dist1 */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(4, string);

    /* dist2 */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(5, string);

    /* dist3 */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(6, string);

    /* dist4 */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(7, string);

    /* dist5 */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(8, string);

    /* dist6 */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(9, string);

    /* dist7 */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(10, string);

    /* dist8 */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(11, string);

    /* dist9 */ 
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(12, string);

    /* ytd */
    ptuple->set_value(13, (int)0);

    /* order_cnt */
    ptuple->set_value(14, (int)0);

    /* remote_cnt */
    ptuple->set_value(15, (int)0);

    /* data */
    _tpccrnd.string_with_original(string, 26, 50, 10.5, &hit);
    ptuple->set_value(16, string);
}
