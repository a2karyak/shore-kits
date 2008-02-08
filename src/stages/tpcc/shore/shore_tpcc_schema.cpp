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

#define  MAX_SHORT_LEN     60
#define  MAX_LONG_LEN     550



/*============== Access methods on tables =====================*/



/* ----------------- */
/* --- WAREHOUSE --- */
/* ----------------- */



w_rc_t warehouse_t::index_probe(ss_m* db,
                                table_row_t* ptuple,
                                const short w_id)
{
    ptuple->set_value(0, w_id);
    W_DO(table_desc_t::index_probe(db, "W_INDEX", ptuple));
    return (RCOK);
}

w_rc_t warehouse_t::index_probe_forupdate(ss_m* db,
                                          table_row_t* ptuple,
                                          const short w_id)
{
    ptuple->set_value(0, w_id);
    W_DO(table_desc_t::index_probe_forupdate(db, "W_INDEX", ptuple));
    return (RCOK);
}

w_rc_t warehouse_t::update_ytd(ss_m* db,
                               table_row_t* ptuple,
                               const short w_id,
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
                               const short d_id,
                               const short w_id)
{
    /* probing to get the tuple */
    ptuple->set_value(0, d_id);
    ptuple->set_value(1, w_id);
    return (table_desc_t::index_probe(db, "D_INDEX", ptuple));
}

w_rc_t district_t::index_probe_forupdate(ss_m* db,
                                         table_row_t* ptuple,
                                         const short d_id,
                                         const short w_id)
{
    /* probing to get the tuple */
    ptuple->set_value(0, d_id);
    ptuple->set_value(1, w_id);
    return (table_desc_t::index_probe_forupdate(db, "D_INDEX", ptuple));
}

w_rc_t district_t::update_ytd(ss_m* db,
                              table_row_t* ptuple,
                              const short d_id,
                              const short w_id,
                              const double amount)
{
    ptuple->set_value(0, d_id);
    ptuple->set_value(1, w_id);

    // W_DO(table_desc_t::index_probe(shore, "D_INDEX"));
    // cout << "APP: " << xct()->tid() << " Update district " << w_id << " " << d_id << endl;
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
    // cout << "APP: " << xct()->tid() << " Update district.next-o-id " << next_o_id << endl;
    W_DO(update_tuple(db, ptuple));
    return (RCOK);
}



/* ---------------- */
/* --- CUSTOMER --- */
/* ---------------- */

w_rc_t customer_t::get_iter_by_index(ss_m* db,
                                     index_scan_iter_impl* &iter,
                                     table_row_t* ptuple,
                                     const short w_id,
                                     const short d_id,
                                     const char* c_last)
{
    index_desc_t* index = find_index("C_NAME_INDEX");
    assert (index);

    ptuple->set_value(2, w_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(5, c_last);
    ptuple->set_value(3, "");
    ptuple->set_value(0, 0);
    int   lowsize = key_size(index, ptuple);
    char* lowkey  = new char [lowsize];
    memcpy(lowkey, format_key(index, ptuple), lowsize);

    char   temp[2];
    temp[0] = MAX('z', 'Z')+1;
    temp[1] = '\0';
    ptuple->set_value(3, temp);
    int   highsize = key_size(index, ptuple);
    char* highkey  = new char[highsize];
    memcpy(highkey, format_key(index, ptuple), highsize);

    /* index only access */
    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge, vec_t(lowkey, lowsize),
				 scan_index_i::lt, vec_t(highkey, highsize),
				 false));

    return (RCOK);
}

w_rc_t customer_t::index_probe(ss_m* db,
                               table_row_t* ptuple,
                               const int   c_id,
                               const short w_id,
                               const short d_id)
{
    ptuple->set_value(0, c_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    return (table_desc_t::index_probe(db, "C_INDEX", ptuple));
}

w_rc_t customer_t::index_probe_forupdate(ss_m * db,
                                         table_row_t* ptuple,
                                         const int   c_id,
                                         const short w_id,
                                         const short d_id)
{
    ptuple->set_value(0, c_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    return (table_desc_t::index_probe_forupdate(db, "C_INDEX", ptuple));
}

w_rc_t customer_t::update_tuple(ss_m* db,
                                table_row_t* ptuple,
                                const double balance,
                                const double ytd_payment,
                                const short  payment_cnt,
                                const char * data)
{
    ptuple->set_value(16, balance);
    ptuple->set_value(17, ytd_payment);
    ptuple->set_value(18, payment_cnt);

    if (data)
	ptuple->set_value(20, data);

    int c_id;
    short w_id;
    short d_id;
    ptuple->get_value(0,c_id);
    ptuple->get_value(1,d_id);
    ptuple->get_value(2,w_id);

    // cout << "APP: " << xct()->tid() << " Update customer-tuple " << w_id << " " << d_id << " " << c_id << endl;
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
                                       const short w_id,
                                       const short d_id,
                                       const int   low_o_id,
                                       const int   high_o_id)
{
    /* pointer to the index */
    index_desc_t * index = find_index("OL_INDEX");
    assert (index);

    /* get the lowest key value */
    ptuple->set_value(0, low_o_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    ptuple->set_value(3, (short)0);  /* assuming that ol_number starts from 1 */
    int   lowsize = key_size(index, ptuple);
    char* lowkey  = new char [lowsize];
    memcpy(lowkey, format_key(index, ptuple), lowsize);

    /* get the highest key value */
    ptuple->set_value(0, high_o_id+1);
    int   highsize = key_size(index, ptuple);
    char* highkey = new char [highsize];
    memcpy(highkey, format_key(index, ptuple), highsize);
    
    /* get the tuple iterator (not index only scan) */
    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge, vec_t(lowkey, lowsize),
				 scan_index_i::lt, vec_t(highkey, highsize),
				 true));
    delete (lowkey);
    delete (highkey);
    return (RCOK);
}


w_rc_t order_line_t::get_iter_by_index(ss_m* db,
                                       index_scan_iter_impl* & iter,
                                       table_row_t* ptuple,
                                       const short w_id,
                                       const short d_id,
                                       const int   o_id)
{
    index_desc_t  * index = find_index("OL_INDEX");
    assert (index);

    ptuple->set_value(0, o_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    ptuple->set_value(3, 0);
    int   lowsize = key_size(index, ptuple);
    char* lowkey = new char [lowsize];
    memcpy(lowkey, format_key(index, ptuple), lowsize);
    
    ptuple->set_value(0, o_id+1);
    int   highsize = key_size(index, ptuple);
    char* highkey = new char [highsize];
    memcpy(highkey, format_key(index, ptuple), highsize);

    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge,
				 vec_t(lowkey, lowsize),
				 scan_index_i::lt,
				 vec_t(highkey, highsize),
				 true));
    return (RCOK);
}

w_rc_t order_t::update_carrier_by_index(ss_m* db,
                                        table_row_t* ptuple,
                                        const short w_id,
                                        const short d_id,
                                        const int   o_id,
                                        const short carrier_id)
{
    ptuple->set_value(0, o_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);

    // W_DO(index_probe(shore, "O_INDEX"));
    // cout << "APP: " << xct()->tid() << " Update order " << w_id << " " << d_id << " " << o_id << endl;
    W_DO(index_probe_forupdate(db, "O_INDEX", ptuple));

    ptuple->set_value(5, carrier_id);
    W_DO(update_tuple(db, ptuple));

    return (RCOK);
}

w_rc_t order_t::get_iter_by_index(ss_m* db,
                                  index_scan_iter_impl* & iter,
                                  table_row_t* ptuple,
                                  const short w_id,
                                  const short d_id,
                                  const int   c_id)
{
    index_desc_t * index = find_index("O_CUST_INDEX");
    assert (index);

    ptuple->set_value(0, 0);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    ptuple->set_value(3, c_id);
    int   lowsize = key_size(index, ptuple);
    char* lowkey  = new char [lowsize];
    memcpy(lowkey, format_key(index, ptuple), lowsize);
    
    ptuple->set_value(3, c_id+1);
    int   highsize = key_size(index, ptuple);
    char* highkey = new char [highsize];
    memcpy(highkey, format_key(index, ptuple), highsize);

    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge, vec_t(lowkey, lowsize),
				 scan_index_i::lt, vec_t(highkey, highsize),
				 true));

    delete (lowkey);
    delete (highkey);
    return (RCOK);
}



/* ----------------- */
/* --- NEW_ORDER --- */
/* ----------------- */

				    

w_rc_t   new_order_t::get_iter_by_index(ss_m* db,
					index_scan_iter_impl * & iter,
                                        table_row_t* ptuple,
					const short w_id,
					const short d_id)
{
    /* find the index structure */
    index_desc_t * index = find_index("NO_INDEX");
    assert (index);

    /* get the lowest key value */
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    ptuple->set_value(0, 0);
    int   lowsize = key_size(index, ptuple);
    char* lowkey = new char [lowsize];
    memcpy(lowkey, format_key(index, ptuple), lowsize);

    /* get the highest key value */
    ptuple->set_value(1, d_id+1);
    int   highsize = key_size(index, ptuple);
    char* highkey = new char [highsize];
    memcpy(highkey, format_key(index, ptuple), highsize);
    
    /* get the tuple iterator (index only scan) */
    W_DO(get_iter_for_index_scan(db, index, iter,
				 scan_index_i::ge, vec_t(lowkey, lowsize),
				 scan_index_i::lt, vec_t(highkey, highsize)));

    delete (lowkey);
    delete (highkey);
    return (RCOK);
}

w_rc_t new_order_t::delete_by_index(ss_m* db,
                                    table_row_t* ptuple,
				    const short w_id,
				    const short d_id,
				    const int   o_id)
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
                            const int    i_id,
                            const short  w_id)
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
                                      const int    i_id,
                                      const short  w_id)
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
			      const short  quantity,
			      const short  order_cnt,
			      const double ytd,
			      const short  remote_cnt)
{
    ptuple->set_value(2, quantity);
    ptuple->set_value(14, order_cnt);
    ptuple->set_value(13, ytd);
    ptuple->set_value(15, remote_cnt);
    int i_id;
    short w_id;
    ptuple->get_value(0,i_id);
    ptuple->get_value(1,w_id);

    // cout << "APP: " << xct()->tid() << " Update stock-tuple " << w_id << " " << i_id << endl;
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

void  warehouse_t::random(table_row_t* ptuple, short w_id)
{
    char    short_string[MAX_SHORT_LEN];
    double  double_number;

    /* id */
    ptuple->set_value(0, w_id);

    /* name */
    _tpccrnd.random_a_string(short_string, 6, 10);
    ptuple->set_value(1, short_string);

    /* street1 */
    _tpccrnd.random_a_string(short_string, 10, 20);
    ptuple->set_value(2, short_string);

    /* street2 */
    _tpccrnd.random_a_string(short_string, 10, 20);
    ptuple->set_value(3, short_string);

    /* city */
    _tpccrnd.random_a_string(short_string, 10, 20);
    ptuple->set_value(4, short_string);

    /* state */
    _tpccrnd.random_a_string(short_string, 2, 2);
    ptuple->set_value(5, short_string);

    /* zip */
    _tpccrnd.random_n_string(short_string, 4, 4);
    strcat(short_string, "11111");
    ptuple->set_value(6, short_string);

    /* tax */
    double_number = ((double) _tpccrnd.random_integer(0,2000))/(double)10000.0;
    ptuple->set_value(7, double_number);

    /* ytd */
    ptuple->set_value(8, (double)300000);
}

w_rc_t  warehouse_t::bulkload(ss_m* db, int w_num)
{
#ifdef   DEBUG
    cout << "Loading " << _name << " table ..." << endl;
#endif

    _tpccrnd.init_random(23);

    W_DO(db->begin_xct());

    /* 1. create the warehouse table */
    W_DO(create_table(db));

    /* 2. append the tuples */
    append_file_i  file_append(_fid);
    
    // add tuples to table and index
    int count = 0;
    int mark = COMMIT_ACTION_COUNT;
    table_row_t awh_tuple(this);

    for (int w_id = 1; w_id <= w_num; w_id++) {
	random(&awh_tuple, w_id);
	W_DO(file_append.create_rec(vec_t(), (smsize_t)0,
				    vec_t(awh_tuple.format(), awh_tuple.size()),
				    awh_tuple._rid));

	if ((count >= mark) == 0) {
	    W_DO(db->commit_xct());
// 	    if (ss_m::num_active_xcts() != 0)
// 		return RC(se_LOAD_NOT_EXCLUSIVE);
	    W_DO(db->begin_xct());
            mark += COMMIT_ACTION_COUNT;
	}
	count++;
    }
    W_DO(db->commit_xct());

#ifdef DEBUG
    cout << "# of records inserted: " << count << endl;
    cout << "Building indices ... " << endl;
#endif

    return (bulkload_all_indexes(db));
}

/*==========================================================
 * class district_t
 *==========================================================
 */

void district_t::random(table_row_t* ptuple, short id, short w_id, int next_o_id)
{
    char   short_string[MAX_SHORT_LEN] = "test";
    double double_number;

    /* id */
    ptuple->set_value(0, id);

    /* w_id */
    ptuple->set_value(1, w_id);

    /* name */
    _tpccrnd.random_a_string(short_string, 6, 10);
    ptuple->set_value(2, short_string);

    /* street1 */
    _tpccrnd.random_a_string(short_string, 10, 20);
    ptuple->set_value(3, short_string);

    /* street2 */
    _tpccrnd.random_a_string(short_string, 10, 20);
    ptuple->set_value(4, short_string);

    /* city */
    _tpccrnd.random_a_string(short_string, 10, 20);
    ptuple->set_value(5, short_string);

    /* state */
    _tpccrnd.random_a_string(short_string, 2, 2);
    ptuple->set_value(6, short_string);

    /* zip */
    _tpccrnd.random_n_string(short_string, 4, 4);
    strcat(short_string, "11111");
    ptuple->set_value(7, short_string);

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
#ifdef   DEBUG
    cout << "Loading " << _name << " table ..." << endl;
#endif

    _tpccrnd.init_random(44);

    W_DO(db->begin_xct());

    /* 1. create the district table */
    W_DO(create_table(db));

    /* 2. append the tuples */
    append_file_i  file_append(_fid);
    
    // add tuples to table and index
    int count = 0;
    int mark = COMMIT_ACTION_COUNT;
    table_row_t ad_tuple(this);

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    random(&ad_tuple, d_id, w_id, CUSTOMERS_PER_DISTRICT+1);
	    W_DO(file_append.create_rec(vec_t(), 0,
					vec_t(ad_tuple.format(), ad_tuple.size()),
					ad_tuple._rid));
	    count++;
	    if ((count >= mark) == 0) {
		W_DO(db->commit_xct());
// 		if (ss_m::num_active_xcts() != 0)
// 		    return RC(se_LOAD_NOT_EXCLUSIVE);
		W_DO(db->begin_xct());
                mark += COMMIT_ACTION_COUNT;
	    }
	}
    }
    W_DO(db->commit_xct());

#ifdef DEBUG
    cout << "# of records inserted: " << count << endl;
    cout << "Building indices ... " << endl;
#endif

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class customer_t
 *==========================================================
 */

void  customer_t::random(table_row_t* ptuple, int id, short d_id, short w_id)
{
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
    ptuple->set_value(18, (short)10);

    /* delivery_cnt */
    ptuple->set_value(19, (short)1);

    /* data */
    _tpccrnd.random_a_string(string, 300, 500);
    ptuple->set_value(20, string);
}

w_rc_t customer_t::bulkload(ss_m* db, int w_num)
{
#ifdef   DEBUG
    cout << "Loading " << _name << " table ..." << endl;
#endif

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

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    for (int c_id = 1; c_id <= CUSTOMERS_PER_DISTRICT; c_id++) {
		random(&ac_tuple, c_id, d_id, w_id);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(ac_tuple.format(), ac_tuple.size()),
					    ac_tuple._rid));

		if ((count >= mark) == 0) {
		    W_DO(db->commit_xct());
// 		    if (ss_m::num_active_xcts() != 0)
// 			return RC(se_LOAD_NOT_EXCLUSIVE);
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;
	    }
	}
    }
    W_DO(db->commit_xct());

#ifdef DEBUG
    cout << "# of records inserted: " << count << endl;
    cout << "Building indices ... " << endl;
#endif

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class history_t
 *==========================================================
 */

void  history_t::random(table_row_t* ptuple, int c_id, short c_d_id, short c_w_id)
{
    char  string[MAX_SHORT_LEN];
    timestamp_t  time;

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
#ifdef   DEBUG
    cout << "Loading " << _name << " table ..." << endl;
#endif

    _tpccrnd.init_random(15);

    W_DO(db->begin_xct());

    /* 1. create the history table */
    W_DO(create_table(db));

    /* 2. append the tuples */
    append_file_i  file_append(_fid);

    // add tuples to the table 
    int count = 1;
    int mark = COMMIT_ACTION_COUNT;
    table_row_t ah_tuple(this);

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    for (int c_id = 1; c_id <= CUSTOMERS_PER_DISTRICT; c_id++) {
		random(&ah_tuple, c_id, d_id, w_id);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(ah_tuple.format(), ah_tuple.size()),
					    ah_tuple._rid));

		if ((count >= mark) == 0) {
		    W_DO(db->commit_xct());
// 		    if (ss_m::num_active_xcts() != 0)
// 			return RC(se_LOAD_NOT_EXCLUSIVE);
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;
	    }
	}
    }
    W_DO(db->commit_xct());

#ifdef DEBUG
    cout << "# of records inserted: " << count << endl;
    cout << "Building indices ... " << endl;
#endif

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class new_order_t
 *==========================================================
 */

void new_order_t::random(table_row_t* ptuple, int id, short d_id, short w_id)
{
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

#ifdef   DEBUG
    cout << "Loading " << _name << " table ..." << endl;
#endif

    _tpccrnd.init_random(99);

    W_DO(db->begin_xct());

    /* 1. create the new_order table */
    W_DO(create_table(db));

    /* 2. append tuples */
    append_file_i  file_append(_fid);
    // add tuples to the table 
    int count = 1;
    int mark = COMMIT_ACTION_COUNT;
    table_row_t anu_tuple(this);

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    for (int o_id = o_id_lo ; o_id <= CUSTOMERS_PER_DISTRICT; o_id++) {
		random(&anu_tuple, o_id, d_id, w_id);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(anu_tuple.format(), anu_tuple.size()),
					    anu_tuple._rid));

		if ((count >= mark) == 0) {
		    W_DO(db->commit_xct());
// 		    if (ss_m::num_active_xcts() != 0)
// 			return RC(se_LOAD_NOT_EXCLUSIVE);
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;
	    }
	}
    }
    W_DO(db->commit_xct());

#ifdef DEBUG
    cout << "# of records inserted: " << count << endl;
    cout << "Building indices ... " << endl;
#endif

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class order_t
 *==========================================================
 */

void order_t::random(table_row_t* ptuple, int id, int c_id, short d_id, short w_id, short ol_cnt)
{
    short short_num;
    timestamp_t  time;
    char   string[MAX_SHORT_LEN];

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
    if (id < 2101) short_num = _tpccrnd.random_integer(1,10);
    else short_num = 0;
    ptuple->set_value(5, short_num);

    /* ol_cnt */
    ptuple->set_value(6, ol_cnt);

    /* all_local */
    ptuple->set_value(7, 1);
}


void order_t::produce_cnt_array(int w_num)
{
    cerr << "building cnt_array... " << endl;
    _pcnt_array = (short*)malloc(sizeof(short)*w_num*DISTRICTS_PER_WAREHOUSE*CUSTOMERS_PER_DISTRICT);
    assert(_pcnt_array);
    for (int i=0; i<w_num; i++)
	_tpccrnd.random_ol_cnt(_pcnt_array+i*DISTRICTS_PER_WAREHOUSE*CUSTOMERS_PER_DISTRICT);
    
}

w_rc_t order_t::bulkload(ss_m* db, int w_num)
{
    assert (_pcnt_array);
    return (bulkload(db, w_num, _pcnt_array));
}


w_rc_t order_t::bulkload(ss_m* db, int w_num, short* cnt_array)
{
#ifdef   DEBUG
    cout << "Loading " << _name << " table ..." << endl;
#endif

    int index = 0;

    _tpccrnd.init_random(42);

    W_DO(db->begin_xct());

    /* 1. create the order table */
    W_DO(create_table(db));

    /* 2. append tuples */
    append_file_i  file_append(_fid);

    // add tuples to the table 
    int count = 1;
    int mark = COMMIT_ACTION_COUNT;
    table_row_t ao_tuple(this);

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    _tpccrnd.seed_1_3000();

	    for (int o_id = 1; o_id <= CUSTOMERS_PER_DISTRICT; o_id++) {
		int ol_cnt = cnt_array[index++];
		random(&ao_tuple, o_id, _tpccrnd.random_1_3000(), d_id, w_id, ol_cnt);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(ao_tuple.format(), ao_tuple.size()),
					    ao_tuple._rid));

		if ((count >= mark) == 0) {
		    W_DO(db->commit_xct());
// 		    if (ss_m::num_active_xcts() != 0)
// 			return RC(se_LOAD_NOT_EXCLUSIVE);
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;                
	    }
	}
    }
    W_DO(db->commit_xct());

#ifdef DEBUG
    cout << "# of records inserted: " << count << endl;
    cout << "Building indices ... " << endl;
#endif

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class order_line_t
 *==========================================================
 */

void order_line_t::random(table_row_t* ptuple, int id,
			  short d_id,
			  short w_id,
			  short ol_index,
			  bool delivery)
{
    char   string[MAX_SHORT_LEN];
    timestamp_t  time;

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
    ptuple->set_value(7, (short)5);

    /* amount */
    ptuple->set_value(8, _tpccrnd.random_integer(1, 999999));

    /* dist_info */
    _tpccrnd.random_a_string(string, 24, 24);
    ptuple->set_value(9, string);
}


w_rc_t order_line_t::bulkload(ss_m* db, int w_num)
{
    assert (_pcnt_array);
    return (bulkload(db, w_num, _pcnt_array));
}


w_rc_t order_line_t::bulkload(ss_m* db, int w_num, short* cnt_array)
{
#ifdef   DEBUG
    cout << "Loading " << _name << " table ..." << endl;
#endif

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
    table_row_t aol_tuple(this);

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    _tpccrnd.seed_1_3000();

	    for (int o_id = 1; o_id <= CUSTOMERS_PER_DISTRICT; o_id++) {
		int ol_cnt = cnt_array[index++];
	  
		for (short ol_id = 1; ol_id <= ol_cnt; ol_id++) {
		    if (o_id < 2101) {
			random(&aol_tuple, o_id, d_id, w_id, ol_id);
		    }
		    else {
			random(&aol_tuple, o_id, d_id, w_id, ol_id, false);
		    }

		    W_DO(file_append.create_rec(vec_t(), 0,
						vec_t(aol_tuple.format(), aol_tuple.size()),
						aol_tuple._rid));

		    if ((count >= mark) == 0) {
			W_DO(db->commit_xct());
// 			if (ss_m::num_active_xcts() != 0)
// 			    return RC(se_LOAD_NOT_EXCLUSIVE);
			W_DO(db->begin_xct());
                        mark += COMMIT_ACTION_COUNT;
		    }
		    count++;
		}
	    }
	}
    }
    W_DO(db->commit_xct());

#ifdef DEBUG
    cout << "# of records inserted: " << count << endl;
    cout << "Building indices ... " << endl;
#endif

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class item_t
 *==========================================================
 */

void item_t::random(table_row_t* ptuple, int id)
{
    char   string[MAX_SHORT_LEN];
    int    hit;

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
#ifdef   DEBUG
    cout << "Loading " << _name << " table ..." << endl;
#endif

    _tpccrnd.init_random(13);

    W_DO(db->begin_xct());

    /* 1. create the item table */
    W_DO(create_table(db));

    /* 2. append tuples */
    append_file_i  file_append(_fid);

    // add tuples to the table 
    int count = 0;
    int mark = COMMIT_ACTION_COUNT;
    table_row_t ai_tuple(this);

    for (int i_id = 1; i_id <= ITEMS; i_id++) {
	random(&ai_tuple, i_id);
	W_DO(file_append.create_rec(vec_t(), 0,
				    vec_t(ai_tuple.format(), ai_tuple.size()),
				    ai_tuple._rid));

	if ((count >= mark) == 0) {
	    W_DO(db->commit_xct());
// 	    if (ss_m::num_active_xcts() != 0)
// 		return RC(se_LOAD_NOT_EXCLUSIVE);
	    W_DO(db->begin_xct());
            mark += COMMIT_ACTION_COUNT;
	}
	count++;
    }
    W_DO(db->commit_xct());

#ifdef DEBUG
    cout << "# of records inserted: " << count << endl;
    cout << "Building indices ... " << endl;
#endif

    return bulkload_all_indexes(db);
}

/*==========================================================
 * class stock_t
 *==========================================================
 */

void stock_t::random(table_row_t* ptuple, int id, short w_id)
{
    char   string[MAX_SHORT_LEN];
    int    hit;

    /* i_id */
    ptuple->set_value(0, id);

    /* w_id */
    ptuple->set_value(1, w_id);

    /* quantity */
    ptuple->set_value(2, (short)_tpccrnd.random_integer(10, 100));

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
    ptuple->set_value(14, (short)0);

    /* remote_cnt */
    ptuple->set_value(15, (short)0);

    /* data */
    _tpccrnd.string_with_original(string, 26, 50, 10.5, &hit);
    ptuple->set_value(16, string);
}

w_rc_t stock_t::bulkload(ss_m* db, int w_num)
{
#ifdef   DEBUG
    cout << "Loading " << _name << " table ..." << endl;
#endif

    _tpccrnd.init_random(7);

    W_DO(db->begin_xct());

    /* 1. create the stock table */
    W_DO(create_table(db));

    /* 2. append tuples */
    append_file_i  file_append(_fid);

    // add tuples to the table 
    int count = 0;
    int mark = COMMIT_ACTION_COUNT;
    table_row_t as_tuple(this);

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int s_id = 1; s_id <= STOCK_PER_WAREHOUSE; s_id++) {
	    random(&as_tuple, s_id, w_id);
	    W_DO(file_append.create_rec(vec_t(), 0,
					vec_t(as_tuple.format(), as_tuple.size()),
					as_tuple._rid));

	    if ((count >= mark) == 0) {
		W_DO(db->commit_xct());
// 		if (ss_m::num_active_xcts() != 0)
// 		    return RC(se_LOAD_NOT_EXCLUSIVE);
		W_DO(db->begin_xct());
                mark += COMMIT_ACTION_COUNT;
	    }
	    count++;
	}
    }
    W_DO(db->commit_xct());

#ifdef DEBUG
    cout << "# of records inserted: " << count << endl;
    cout << "Building indices ... " << endl;
#endif

    return bulkload_all_indexes(db);
}
