/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_schema.h
 *
 *  @brief:  Implementation of the TPC-C tables
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */


#include <math.h>

#include "stages/tpcc/shore/shore_tpcc_schema.h"
#include "stages/tpcc/shore/shore_tpcc_random.h"


using namespace shore;
using namespace tpcc;

#define  MAX_INT_LEN     60
#define  MAX_LONG_LEN    550



/*============== Access methods on tables =====================*/



/* ----------------- */
/* --- WAREHOUSE --- */
/* ----------------- */



w_rc_t warehouse_t::index_probe(ss_m* db,
                                table_row_t* ptuple,
                                const int w_id)
{
    ptuple->set_value(0, w_id);
    W_DO(table_desc_t::index_probe(db, "W_INDEX", ptuple));
    return (RCOK);
}

w_rc_t warehouse_t::index_probe_forupdate(ss_m* db,
                                          table_row_t* ptuple,
                                          const int w_id)
{
    ptuple->set_value(0, w_id);
    W_DO(table_desc_t::index_probe_forupdate(db, "W_INDEX", ptuple));
    return (RCOK);
}

w_rc_t warehouse_t::update_ytd(ss_m* db,
                               table_row_t* ptuple,
                               const int w_id,
                               const double amount)
{
    ptuple->set_value(0, w_id);
    
    // W_DO(table_desc_t::index_probe(shore, "W_INDEX"));
    // cout << "APP: " << xct()->tid() << " Update warehouse " << w_id << endl;
    W_DO(table_desc_t::index_probe_forupdate(db, "W_INDEX", ptuple));

    double ytd;
    ptuple->get_value(8, ytd);
    ytd += amount;
    ptuple->set_value(8, ytd);
    W_DO(update_tuple(db, ptuple));

    return (RCOK);
}



/* ---------------- */
/* --- DISTRICT --- */
/* ---------------- */

/* get the district tuple by following the index on (d_id, w_id) */
w_rc_t district_t::index_probe(ss_m* db,
                               table_row_t* ptuple,
                               const int d_id,
                               const int w_id)
{
    /* probing to get the tuple */
    ptuple->set_value(0, d_id);
    ptuple->set_value(1, w_id);
    return (table_desc_t::index_probe(db, "D_INDEX", ptuple));
}

w_rc_t district_t::index_probe_forupdate(ss_m* db,
                                         table_row_t* ptuple,
                                         const int d_id,
                                         const int w_id)
{
    /* probing to get the tuple */
    ptuple->set_value(0, d_id);
    ptuple->set_value(1, w_id);
    return (table_desc_t::index_probe_forupdate(db, "D_INDEX", ptuple));
}

w_rc_t district_t::update_ytd(ss_m* db,
                              table_row_t* ptuple,
                              const int d_id,
                              const int w_id,
                              const double amount)
{
    ptuple->set_value(0, d_id);
    ptuple->set_value(1, w_id);


    W_DO(table_desc_t::index_probe_forupdate(db, "D_INDEX", ptuple));

    double d_ytd;
    ptuple->get_value(9, d_ytd);
    d_ytd += amount;
    ptuple->set_value(9, d_ytd);
    W_DO(update_tuple(db, ptuple));

    return (RCOK);
}

w_rc_t district_t::update_next_o_id(ss_m* db,
                                    table_row_t* ptuple,
                                    const int next_o_id)
{
    ptuple->set_value(10, next_o_id);

    W_DO(update_tuple(db, ptuple));
    return (RCOK);
}



/* ---------------- */
/* --- CUSTOMER --- */
/* ---------------- */


w_rc_t customer_t::get_iter_by_index(ss_m* db,
                                     index_scan_iter_impl* &iter,
                                     table_row_t* ptuple,
                                     const int w_id,
                                     const int d_id,
                                     const char* c_last)
{
    index_desc_t* index = find_index("C_NAME_INDEX");
    assert (index);

    // C_NAME_INDEX: {2 - 1 - 5 - 3 - 0}

    // prepare the key to be probed
    ptuple->set_value(0, 0);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    ptuple->set_value(3, "");
    ptuple->set_value(5, c_last);

    char* lowkey = NULL;
    int   lobufsz = 0;
    int   lowsz  = format_key(index, ptuple, lowkey, lobufsz);
    assert (lowkey);

    char   temp[2];
    temp[0] = MAX('z', 'Z')+1;
    temp[1] = '\0';
    ptuple->set_value(3, temp);

    char* highkey = NULL;
    int   hibufsz = 0;
    int   highsz  = format_key(index, ptuple, highkey, hibufsz);
    assert (highkey);
    

    /* index only access */
    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge, vec_t(lowkey, lowsz),
				 scan_index_i::lt, vec_t(highkey, highsz),
				 false));

    delete [] lowkey;
    delete [] highkey;

    return (RCOK);
}

w_rc_t customer_t::index_probe(ss_m* db,
                               table_row_t* ptuple,
                               const int c_id,
                               const int w_id,
                               const int d_id)
{
    ptuple->set_value(0, c_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    return (table_desc_t::index_probe(db, "C_INDEX", ptuple));
}

w_rc_t customer_t::index_probe_forupdate(ss_m * db,
                                         table_row_t* ptuple,                                         
                                         const int c_id,
                                         const int w_id,
                                         const int d_id)
{
    ptuple->set_value(0, c_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    return (table_desc_t::index_probe_forupdate(db, "C_INDEX", ptuple));
}

w_rc_t customer_t::update_tuple(ss_m* db,
                                table_row_t* ptuple,
                                const tpcc_customer_tuple acustomer,
                                const char* adata1,
                                const char* adata2)
{
    ptuple->set_value(16, acustomer.C_BALANCE);
    ptuple->set_value(17, acustomer.C_YTD_PAYMENT);
    ptuple->set_value(19, acustomer.C_PAYMENT_CNT);

    if (adata1)
	ptuple->set_value(20, adata1);

    if (adata2)
	ptuple->set_value(21, adata2);

    W_DO(table_desc_t::update_tuple(db, ptuple));

    return (RCOK);
}




/* ----------------- */
/* --- ORDERLINE --- */
/* ----------------- */



/* index range scan on order_line */
w_rc_t order_line_t::get_iter_by_index(ss_m* db,
                                       index_scan_iter_impl* & iter,
                                       table_row_t* ptuple,
                                       const int w_id,
                                       const int d_id,
                                       const int low_o_id,
                                       const int high_o_id)
{
    /* pointer to the index */
    index_desc_t * index = find_index("OL_INDEX");
    assert (index);

    /* get the lowest key value */
    ptuple->set_value(0, low_o_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    ptuple->set_value(3, (int)0);  /* assuming that ol_number starts from 1 */

    char* lowkey = NULL;
    int   lobufsz = 0;
    int   lowsz  = format_key(index, ptuple, lowkey, lobufsz);
    assert (lowkey);

    /* get the highest key value */
    ptuple->set_value(0, high_o_id+1);

    char* highkey = NULL;
    int   hibufsz = 0;
    int   highsz  = format_key(index, ptuple, highkey, hibufsz);
    assert (highkey);
    
    /* get the tuple iterator (not index only scan) */
    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge, vec_t(lowkey, lowsz),
				 scan_index_i::lt, vec_t(highkey, highsz),
				 true));

    delete [] lowkey;
    delete [] highkey;

    return (RCOK);
}


w_rc_t order_line_t::get_iter_by_index(ss_m* db,
                                       index_scan_iter_impl* & iter,
                                       table_row_t* ptuple,
                                       const int w_id,
                                       const int d_id,
                                       const int o_id)
{
    index_desc_t  * index = find_index("OL_INDEX");
    assert (index);

    ptuple->set_value(0, o_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    ptuple->set_value(3, 0);

    char* lowkey = NULL;
    int   lobufsz = 0;
    int   lowsz  = format_key(index, ptuple, lowkey, lobufsz);
    assert (lowkey);

    /* get the highest key value */
    ptuple->set_value(0, o_id+1);

    char* highkey = NULL;
    int   hibufsz = 0;
    int   highsz  = format_key(index, ptuple, highkey, hibufsz);
    assert (highkey);

    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge,
				 vec_t(lowkey, lowsz),
				 scan_index_i::lt,
				 vec_t(highkey, highsz),
				 true));

    delete [] lowkey;
    delete [] highkey;

    return (RCOK);
}



/* ------------- */
/* --- ORDER --- */
/* ------------- */


w_rc_t order_t::update_carrier_by_index(ss_m* db,
                                        table_row_t* ptuple,
                                        const int carrier_id)
{
    W_DO(index_probe_forupdate(db, "O_INDEX", ptuple));

    ptuple->set_value(5, carrier_id);
    W_DO(update_tuple(db, ptuple));

    return (RCOK);
}

w_rc_t order_t::get_iter_by_index(ss_m* db,
                                  index_scan_iter_impl* &iter,
                                  table_row_t* ptuple,
                                  const int w_id,
                                  const int d_id,
                                  const int c_id)
{
    index_desc_t * index = find_index("O_CUST_INDEX");
    assert (index);

    ptuple->set_value(0, 0);
    ptuple->set_value(1, c_id);
    ptuple->set_value(2, d_id);
    ptuple->set_value(3, w_id);

//     ptuple->set_value(0, 0);
//     ptuple->set_value(1, d_id);
//     ptuple->set_value(2, w_id);
//     ptuple->set_value(3, c_id);

    char* lowkey = NULL;
    int   lobufsz = 0;
    int   lowsz  = format_key(index, ptuple, lowkey, lobufsz);
    assert (lowkey);

    /* get the highest key value */
    ptuple->set_value(1, c_id+1);

    char* highkey = NULL;
    int   hibufsz = 0;
    int   highsz  = format_key(index, ptuple, highkey, hibufsz);
    assert (highkey);

    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge, vec_t(lowkey, lowsz),
				 scan_index_i::lt, vec_t(highkey, highsz),
				 true));

    delete [] lowkey;
    delete [] highkey;

    return (RCOK);
}



/* ----------------- */
/* --- NEW_ORDER --- */
/* ----------------- */

				    

w_rc_t new_order_t::get_iter_by_index(ss_m* db,
                                      index_scan_iter_impl* &iter,
                                      table_row_t* ptuple,
                                      const int w_id,
                                      const int d_id)
{
    /* find the index structure */
    index_desc_t * index = find_index("NO_INDEX");
    assert (index);

    /* get the lowest key value */
    ptuple->set_value(0, 0);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);


    char* lowkey = NULL;
    int   lobufsz = 0;
    int   lowsz  = format_key(index, ptuple, lowkey, lobufsz);
    assert (lowkey);

    /* get the highest key value */
    ptuple->set_value(1, d_id+1);

    char* highkey = NULL;
    int   hibufsz = 0;
    int   highsz  = format_key(index, ptuple, highkey, hibufsz);
    assert (highkey);

    /* get the tuple iterator (index only scan) */
    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge, vec_t(lowkey, lowsz),
				 scan_index_i::lt, vec_t(highkey, highsz)));

    delete [] lowkey;
    delete [] highkey;

    return (RCOK);
}

w_rc_t new_order_t::delete_by_index(ss_m* db,
                                    table_row_t* ptuple,
				    const int w_id,
				    const int d_id,
				    const int o_id)
{
    ptuple->set_value(0, o_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    W_DO(table_desc_t::index_probe(db, "NO_INDEX", ptuple));
    W_DO(table_desc_t::delete_tuple(db, ptuple));

    return (RCOK);
}



/* ------------ */
/* --- ITEM --- */
/* ------------ */


w_rc_t item_t::index_probe(ss_m* db, 
                           table_row_t* ptuple,
                           const int i_id)
{
    index_desc_t* index = find_index("I_INDEX");
    assert (index);

    ptuple->set_value(0, i_id);
    W_DO(table_desc_t::index_probe(db, index, ptuple));

    return (RCOK);
}

w_rc_t  item_t::index_probe_forupdate(ss_m* db, 
                                      table_row_t* ptuple,
                                      const int i_id)
{
    index_desc_t * index = find_index("I_INDEX");
    assert (index);

    ptuple->set_value(0, i_id);
    W_DO(table_desc_t::index_probe_forupdate(db, index, ptuple));

    return (RCOK);
}



/* ------------- */
/* --- STOCK --- */
/* ------------- */


w_rc_t stock_t::index_probe(ss_m* db,
                            table_row_t* ptuple,
                            const int  i_id,
                            const int  w_id)
{
    index_desc_t * index = find_index("S_INDEX");
    assert (index);

    ptuple->set_value(0, i_id);
    ptuple->set_value(1, w_id);
    W_DO(table_desc_t::index_probe(db, index, ptuple));

    return (RCOK);
}

w_rc_t stock_t::index_probe_forupdate(ss_m* db,
                                      table_row_t* ptuple,
                                      const int  i_id,
                                      const int  w_id)
{
    index_desc_t * index = find_index("S_INDEX");
    assert (index);

    ptuple->set_value(0, i_id);
    ptuple->set_value(1, w_id);
    W_DO(table_desc_t::index_probe_forupdate(db, index, ptuple));

    return (RCOK);
}

w_rc_t  stock_t::update_tuple(ss_m* db,
                              table_row_t* ptuple,
                              const tpcc_stock_tuple* pstock)
{
    ptuple->set_value(2, pstock->S_REMOTE_CNT);
    ptuple->set_value(3, pstock->S_QUANTITY);
    ptuple->set_value(4, pstock->S_ORDER_CNT);
    ptuple->set_value(5, pstock->S_YTD);

    W_DO(table_desc_t::update_tuple(db, ptuple));
    return (RCOK);
}


/*=================== loading utility =========================*/

/*
 * There are two methods for each table:
 *
 * 1. random(), which creates a random tuple.
 *
 * 2. bulkload(), which populates the table and inserts
 * tuples to Shore files.
 *
 */

/* 
 * Implementation of bulkloading.
 *
 * For each table, we need to load two pieces of information:
 *
 * 1. tuples themselves.
 * 2. indices on the table.
 *
 * The loading process consists of two parts.
 *
 * 1. Building the base table. This involves calling
 * begin_create_table() before adding tuples to the table and
 * calling end_create_table() after all the tuples have been
 * added to the tuple. 
 *
 * 2. Building the indices. The bulkloading functionality is
 * implemented in table_desc_t.  The program can just call
 * bulkload_index() to populate all the indices defined on
 * the table.
 *
 */

/*==========================================================
 * class warehouse_t
 *==========================================================
 */

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

w_rc_t  warehouse_t::bulkload(ss_m* db, int w_num)
{
    cout << "Loading " << _name << " table ..." << endl;

    _tpccrnd.init_random(23);

    W_DO(db->begin_xct());

    /* 1. create the warehouse table */
    W_DO(create_table(db));

    /* 2. append the tuples */
    append_file_i  file_append(_fid);
    
    // add tuples to table and index
    register int count = 0;
    register int mark = COMMIT_ACTION_COUNT;
    table_row_t awh_tuple(this);
    
    char* pdest = NULL;
    int   bufsz = 0;
    int   tsz   = 0; 

    for (int w_id = 1; w_id <= w_num; w_id++) {
        // generate a random tuple
	random(&awh_tuple, w_id);

        // append it to the table
        tsz = awh_tuple.format(pdest, bufsz);
        assert (pdest);
	W_DO(file_append.create_rec(vec_t(), (smsize_t)0,
				    vec_t(pdest, tsz),
				    awh_tuple._rid));

	if (count >= mark) {
	    W_DO(db->commit_xct());
// 	    if (ss_m::num_active_xcts() != 0)
// 		return RC(se_LOAD_NOT_EXCLUSIVE);
            cerr << "table(" << _name << "): " << count << endl;
	    W_DO(db->begin_xct());
            mark += COMMIT_ACTION_COUNT;
	}
	count++;
    }
    W_DO(db->commit_xct());

    if (pdest)
        delete [] pdest;

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _name, count);

    return (bulkload_all_indexes(db));
}

/*==========================================================
 * class district_t
 *==========================================================
 */


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

w_rc_t district_t::bulkload(ss_m* db, int w_num)
{
    cout << "Loading " << _name << " table ..." << endl;

    _tpccrnd.init_random(44);

    W_DO(db->begin_xct());

    /* 1. create the district table */
    W_DO(create_table(db));

    /* 2. append the tuples */
    append_file_i  file_append(_fid);
    
    // add tuples to table and index
    register int count = 0;
    register int mark = COMMIT_ACTION_COUNT;
    table_row_t ad_tuple(this);

    char* pdest = NULL;
    int   bufsz = 0;
    int   tsz   = 0; 

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
            // generate a random district tuple
	    random(&ad_tuple, d_id, w_id, CUSTOMERS_PER_DISTRICT+1);

            // append it to the table
            tsz = ad_tuple.format(pdest, bufsz);
            assert (pdest);
	    W_DO(file_append.create_rec(vec_t(), 0,
					vec_t(pdest, tsz),
					ad_tuple._rid));
	    count++;
	    if (count >= mark) {
		W_DO(db->commit_xct());
// 		if (ss_m::num_active_xcts() != 0)
// 		    return RC(se_LOAD_NOT_EXCLUSIVE);
                cerr << "table(" << _name << "): " << count << endl;
		W_DO(db->begin_xct());
                mark += COMMIT_ACTION_COUNT;
	    }
	}
    }
    W_DO(db->commit_xct());

    if (pdest)
        delete [] pdest;

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _name, count);

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class customer_t
 *==========================================================
 */

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

w_rc_t customer_t::bulkload(ss_m* db, int w_num)
{
    cout << "Loading " << _name << " table ..." << endl;

    _tpccrnd.init_random(10);

    W_DO(db->begin_xct());
  
    /* 1. create the customer table */
    W_DO(create_table(db));

    /* 2. append the tuples */
    append_file_i  file_append(_fid);

    // add tuples to the table 
    int count = 0;
    int mark = COMMIT_ACTION_COUNT;
    table_row_t ac_tuple(this);

    char* pdest = NULL;
    int   bufsz = 0;
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    for (int c_id = 1; c_id <= CUSTOMERS_PER_DISTRICT; c_id++) {
                // generate a random customer tuple
		random(&ac_tuple, c_id, d_id, w_id);

                // append it to the table
                tsz = ac_tuple.format(pdest, bufsz);
                assert (pdest);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(pdest, tsz),
					    ac_tuple._rid));

		if (count >= mark) {
		    W_DO(db->commit_xct());
// 		    if (ss_m::num_active_xcts() != 0)
// 			return RC(se_LOAD_NOT_EXCLUSIVE);
                    cerr << "table(" << _name << "): " << count << endl;
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;
	    }
	}
    }
    W_DO(db->commit_xct());

    if (pdest)
        delete [] pdest;

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _name, count);

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class history_t
 *==========================================================
 */

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

w_rc_t  history_t::bulkload(ss_m* db, int w_num)
{
    cout << "Loading " << _name << " table ..." << endl;

    _tpccrnd.init_random(15);

    W_DO(db->begin_xct());

    /* 1. create the history table */
    W_DO(create_table(db));

    /* 2. append the tuples */
    append_file_i  file_append(_fid);

    // add tuples to the table 
    register int count = 1;
    register int mark = COMMIT_ACTION_COUNT;
    table_row_t ah_tuple(this);

    char* pdest = NULL;
    int   bufsz = 0;
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    for (int c_id = 1; c_id <= CUSTOMERS_PER_DISTRICT; c_id++) {
		random(&ah_tuple, c_id, d_id, w_id);

                // append it to the table
                tsz = ah_tuple.format(pdest, bufsz);
                assert (pdest);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(pdest, tsz),
					    ah_tuple._rid));

		if (count >= mark) {
		    W_DO(db->commit_xct());
// 		    if (ss_m::num_active_xcts() != 0)
// 			return RC(se_LOAD_NOT_EXCLUSIVE);
                    cerr << "table(" << _name << "): " << count << endl;
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;
	    }
	}
    }
    W_DO(db->commit_xct());

    if (pdest)
        delete [] pdest;

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _name, count);

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class new_order_t
 *==========================================================
 */

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

w_rc_t new_order_t::bulkload(ss_m* db, int w_num)
{
    int o_id_lo = CUSTOMERS_PER_DISTRICT - NU_ORDERS_PER_DISTRICT + 1;
    if (o_id_lo < 0) {
	o_id_lo = CUSTOMERS_PER_DISTRICT - (CUSTOMERS_PER_DISTRICT / 3) + 1;
	cerr << "**** WARNING **** NU_ORDERS_PER_DISTRICT is > CUSTOMERS_PER_DISTRICT" << endl;
	cerr << "                  Loading New-Order with 1/3 of CUSTOMERS_PER_DISTRICT" << endl;
    }

    cout << "Loading " << _name << " table ..." << endl;

    _tpccrnd.init_random(99);

    W_DO(db->begin_xct());

    /* 1. create the new_order table */
    W_DO(create_table(db));

    /* 2. append tuples */
    append_file_i  file_append(_fid);
    // add tuples to the table 
    register int count = 1;
    register int mark = COMMIT_ACTION_COUNT;
    table_row_t anu_tuple(this);

    char* pdest = NULL;
    int   bufsz = 0;
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    for (int o_id = o_id_lo ; o_id <= CUSTOMERS_PER_DISTRICT; o_id++) {
		random(&anu_tuple, o_id, d_id, w_id);

                // append it to the table
                tsz = anu_tuple.format(pdest, bufsz);
                assert (pdest);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(pdest, tsz),
					    anu_tuple._rid));

		if (count >= mark) {
		    W_DO(db->commit_xct());
// 		    if (ss_m::num_active_xcts() != 0)
// 			return RC(se_LOAD_NOT_EXCLUSIVE);
                    cerr << "table(" << _name << "): " << count << endl;
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;
	    }
	}
    }
    W_DO(db->commit_xct());

    if (pdest)
        delete [] pdest;

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _name, count);

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class order_t
 *==========================================================
 */

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

void order_t::random(table_row_t* ptuple, int id, int c_id, int d_id, int w_id, int ol_cnt)
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


void order_t::produce_cnt_array(int w_num, pthread_mutex_t* parray_mutex)
{
    cerr << "building cnt_array... " << endl;
    // first init and lock the mutex
    assert (parray_mutex);
    _pcnt_array_mutex = parray_mutex;
    pthread_mutex_lock(_pcnt_array_mutex);
    
    // then create the cnt_array
    _pcnt_array = (int*)malloc(sizeof(int)*w_num*DISTRICTS_PER_WAREHOUSE*CUSTOMERS_PER_DISTRICT);
    assert(_pcnt_array);
    for (int i=0; i<w_num; i++)
	_tpccrnd.random_ol_cnt((short*)_pcnt_array+i*DISTRICTS_PER_WAREHOUSE*CUSTOMERS_PER_DISTRICT);
    
}

w_rc_t order_t::bulkload(ss_m* db, int w_num)
{
    assert (_pcnt_array);
    return (bulkload(db, w_num, _pcnt_array));
}


w_rc_t order_t::bulkload(ss_m* db, int w_num, int* cnt_array)
{
    cout << "Loading " << _name << " table ..." << endl;

    int index = 0;

    _tpccrnd.init_random(42);

    W_DO(db->begin_xct());

    /* 1. create the order table */
    W_DO(create_table(db));

    /* 2. append tuples */
    append_file_i  file_append(_fid);

    // add tuples to the table 
    register int count = 1;
    register int mark = COMMIT_ACTION_COUNT;
    table_row_t ao_tuple(this);

    char* pdest = NULL;
    int   bufsz = 0;
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    _tpccrnd.seed_1_3000();

	    for (int o_id = 1; o_id <= CUSTOMERS_PER_DISTRICT; o_id++) {
		int ol_cnt = cnt_array[index++];
		random(&ao_tuple, o_id, _tpccrnd.random_1_3000(), d_id, w_id, ol_cnt);

                // append it to the table
                tsz = ao_tuple.format(pdest, bufsz);
                assert (pdest);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(pdest, tsz),
					    ao_tuple._rid));

		if (count >= mark) {
		    W_DO(db->commit_xct());
// 		    if (ss_m::num_active_xcts() != 0)
// 			return RC(se_LOAD_NOT_EXCLUSIVE);
                    cerr << "table(" << _name << "): " << count << endl;
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;                
	    }
	}
    }
    W_DO(db->commit_xct());

    // release cnt_array mutex
    pthread_mutex_unlock(_pcnt_array_mutex);

    if (pdest)
        delete [] pdest;
    
    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _name, count);

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class order_line_t
 *==========================================================
 */

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


w_rc_t order_line_t::bulkload(ss_m* db, int w_num)
{
    // get cnt_array mutex 
    assert (_pcnt_array_mutex);
    critical_section_t cs(*_pcnt_array_mutex);
    assert (_pcnt_array);

    // do the loading
    return (bulkload(db, w_num, _pcnt_array));
}


w_rc_t order_line_t::bulkload(ss_m* db, int w_num, int* cnt_array)
{
    cout << "Loading " << _name << " table ..." << endl;

    int index = 0;

    //  init_random(42, 13);
    _tpccrnd.init_random(47);

    W_DO(db->begin_xct());

    /* 1. create the order_line table */
    W_DO(create_table(db));

    /* 2. append tuples */
    append_file_i  file_append(_fid);

    // add tuples to the table 
    int count = 1;
    int mark = COMMIT_ACTION_COUNT;

    char* pdest = NULL;
    int   bufsz = 0;
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    _tpccrnd.seed_1_3000();

	    for (int o_id = 1; o_id <= CUSTOMERS_PER_DISTRICT; o_id++) {
		int ol_cnt = cnt_array[index++];
	  
		for (int ol_id = 1; ol_id <= ol_cnt; ol_id++) {
                    table_row_t aol_tuple(this);
		    if (o_id < 2101) {
			random(&aol_tuple, o_id, d_id, w_id, ol_id);
		    }
		    else {
			random(&aol_tuple, o_id, d_id, w_id, ol_id, false);
		    }

                    // append it to the table
                    tsz = aol_tuple.format(pdest, bufsz);
                    assert (pdest);                    
		    W_DO(file_append.create_rec(vec_t(), 0,
						vec_t(pdest, tsz),
						aol_tuple._rid));

		    if (count >= mark) {
			W_DO(db->commit_xct());
// 			if (ss_m::num_active_xcts() != 0)
// 			    return RC(se_LOAD_NOT_EXCLUSIVE);
                        cerr << "table(" << _name << "): " << count << endl;
			W_DO(db->begin_xct());
                        mark += COMMIT_ACTION_COUNT;
		    }
		    count++;
		}
	    }
	}
    }
    W_DO(db->commit_xct());

    if (pdest)
        delete [] pdest;

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _name, count);

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class item_t
 *==========================================================
 */

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

w_rc_t item_t::bulkload(ss_m* db, int /* w_num */)
{
    cout << "Loading " << _name << " table ..." << endl;

    _tpccrnd.init_random(13);

    W_DO(db->begin_xct());

    /* 1. create the item table */
    W_DO(create_table(db));

    /* 2. append tuples */
    append_file_i  file_append(_fid);

    // add tuples to the table 
    register int count = 0;
    register int mark = COMMIT_ACTION_COUNT;
    table_row_t ai_tuple(this);

    char* pdest = NULL;
    int   bufsz = 0;
    int tsz = 0;

    for (int i_id = 1; i_id <= ITEMS; i_id++) {
	random(&ai_tuple, i_id);

        // append it to the table
        tsz = ai_tuple.format(pdest, bufsz);
        assert (pdest);
	W_DO(file_append.create_rec(vec_t(), 0,
				    vec_t(pdest, tsz),
				    ai_tuple._rid));

	if (count >= mark) {
	    W_DO(db->commit_xct());
// 	    if (ss_m::num_active_xcts() != 0)
// 		return RC(se_LOAD_NOT_EXCLUSIVE);
            cerr << "table(" << _name << "): " << count << endl;
	    W_DO(db->begin_xct());
            mark += COMMIT_ACTION_COUNT;
	}
	count++;
    }
    W_DO(db->commit_xct());

    if (pdest)
        delete [] pdest;

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _name, count);

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class stock_t
 *==========================================================
 */

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

w_rc_t stock_t::bulkload(ss_m* db, int w_num)
{
    cout << "Loading " << _name << " table ..." << endl;

    _tpccrnd.init_random(7);

    W_DO(db->begin_xct());

    /* 1. create the stock table */
    W_DO(create_table(db));

    /* 2. append tuples */
    append_file_i  file_append(_fid);

    // add tuples to the table 
    register int count = 0;
    register int mark = COMMIT_ACTION_COUNT;
    table_row_t as_tuple(this);

    char* pdest = NULL;
    int   bufsz = 0;
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int s_id = 1; s_id <= STOCK_PER_WAREHOUSE; s_id++) {
	    random(&as_tuple, s_id, w_id);

            // append it to the table
            tsz = as_tuple.format(pdest, bufsz);
            assert (pdest);
	    W_DO(file_append.create_rec(vec_t(), 0,
					vec_t(pdest, tsz),
					as_tuple._rid));

	    if (count >= mark) {
		W_DO(db->commit_xct());
                cerr << "table(" << _name << "): " << count << endl;
		W_DO(db->begin_xct());
                mark += COMMIT_ACTION_COUNT;
	    }
	    count++;
	}
    }
    W_DO(db->commit_xct());

    if (pdest)
        delete [] pdest;

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _name, count);

    return bulkload_all_indexes(db);
}
