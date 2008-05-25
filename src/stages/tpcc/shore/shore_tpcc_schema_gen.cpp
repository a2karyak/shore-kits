/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_schema.h
 *
 *  @brief:  Implementation of the bulkloading of the TPC-C tables
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "stages/tpcc/shore/shore_tpcc_schema_man.h"
#include "stages/tpcc/shore/shore_tpcc_random.h"


using namespace shore;
using namespace tpcc;


/*********************************************************************
 *
 * loading utility
 *
 *********************************************************************/

/*
 * There is one method for each table:
 *
 * 1. bulkload(), which populates the table and inserts
 * tuples to Shore files.
 *
 */

/* 
 * Implementation of bulkloading.
 *
 * For each table, we need to load two pieces of information:
 *
 * 1. tuples themselves.
 * 2. indexes on the table.
 *
 * The loading process consists of two parts.
 *
 * 1. Building the base table. This involves calling
 * begin_create_table() before adding tuples to the table and
 * calling end_create_table() after all the tuples have been
 * added to the tuple. 
 *
 * 2. Building the indexes. The bulkloading functionality is
 * implemented in table_man_impl.  The program can just call
 * bulkload_index() to populate all the indexes defined on
 * the table.
 *
 */


/*********************************************************************
 *
 * class warehouse_man_impl
 *
 *********************************************************************/

w_rc_t warehouse_man_impl::bulkload(ss_m* db, int w_num)
{
    assert (_ptable);
    TRACE( TRACE_ALWAYS, "Loading (%s) table ...\n", _ptable->name());

    assert (false); // not implemented
    //_ptable->_tpccrnd.init_random(23);

    W_DO(db->begin_xct());

    /* 1. create the warehouse table */
    W_DO(_ptable->create_table(db));

    /* 2. append the tuples */
    append_file_i  file_append(_ptable->_fid);
    
    // add tuples to table and index
    register int count = 0;
    register int mark = COMMIT_ACTION_COUNT;
    warehouse_tuple awh_tuple(_ptable);

    rep_row_t reprow(_pts);    
    int tsz = 0; 

    for (int w_id = 1; w_id <= w_num; w_id++) {
        // generate a random tuple
        assert (false); // not implemented
	//_ptable->random(&awh_tuple, w_id);

        // append it to the table
        tsz = format(&awh_tuple, reprow);
        assert (reprow._dest);
	W_DO(file_append.create_rec(vec_t(), (smsize_t)0,
				    vec_t(reprow._dest, tsz),
				    awh_tuple._rid));

	if (count >= mark) {
	    W_DO(db->commit_xct());
            TRACE( TRACE_DEBUG, "table(%s): %d\n", _ptable->name(), count);
	    W_DO(db->begin_xct());
            mark += COMMIT_ACTION_COUNT;
	}
	count++;
    }
    W_DO(db->commit_xct());

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _ptable->name(), count);

    return (bulkload_all_indexes(db));
}


/*********************************************************************
 *
 * class district_man_impl
 *
 *********************************************************************/

w_rc_t district_man_impl::bulkload(ss_m* db, int w_num)
{
    assert (_ptable);
    TRACE( TRACE_ALWAYS, "Loading (%s) table ...\n", _ptable->name());

    assert (false); // not implemented
    //_ptable->_tpccrnd.init_random(44);

    W_DO(db->begin_xct());

    /* 1. create the district table */
    W_DO(_ptable->create_table(db));

    /* 2. append the tuples */
    append_file_i file_append(_ptable->_fid);
    
    // add tuples to table and index
    register int count = 0;
    register int mark = COMMIT_ACTION_COUNT;
    district_tuple ad_tuple(_ptable);

    rep_row_t reprow(_pts);
    int tsz = 0; 

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
            // generate a random district tuple
            assert (false); // not implemented
            //_ptable->random(&ad_tuple, d_id, w_id, CUSTOMERS_PER_DISTRICT+1);

            // append it to the table
            tsz = format(&ad_tuple, reprow);
            assert (reprow._dest);
	    W_DO(file_append.create_rec(vec_t(), 0,
					vec_t(reprow._dest, tsz),
					ad_tuple._rid));
	    count++;
	    if (count >= mark) {
		W_DO(db->commit_xct());
                TRACE( TRACE_DEBUG, "table(%s): %d\n", _ptable->name(), count);
		W_DO(db->begin_xct());
                mark += COMMIT_ACTION_COUNT;
	    }
	}
    }
    W_DO(db->commit_xct());

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _ptable->name(), count);

    return (bulkload_all_indexes(db));
}


/*********************************************************************
 *
 * class customer_man_impl
 *
 *********************************************************************/

w_rc_t customer_man_impl::bulkload(ss_m* db, int w_num)
{
    assert (_ptable);
    TRACE( TRACE_ALWAYS, "Loading (%s) table ...\n", _ptable->name());

    assert (false); // not implemented
    //_ptable->_tpccrnd.init_random(10);

    W_DO(db->begin_xct());
  
    /* 1. create the customer table */
    W_DO(_ptable->create_table(db));

    /* 2. append the tuples */
    append_file_i file_append(_ptable->_fid);

    // add tuples to the table 
    int count = 0;
    int mark = COMMIT_ACTION_COUNT;
    customer_tuple ac_tuple(_ptable);

    rep_row_t reprow(_pts);
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    for (int c_id = 1; c_id <= CUSTOMERS_PER_DISTRICT; c_id++) {
                // generate a random customer tuple
                assert (false); // not implemented
		//_ptable->random(&ac_tuple, c_id, d_id, w_id);

                // append it to the table
                tsz = format(&ac_tuple, reprow);
                assert (reprow._dest);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(reprow._dest, tsz),
					    ac_tuple._rid));

		if (count >= mark) {
		    W_DO(db->commit_xct());
                    TRACE( TRACE_DEBUG, "table(%s): %d\n", _ptable->name(), count);
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;
	    }
	}
    }
    W_DO(db->commit_xct());

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _ptable->name(), count);

    return (bulkload_all_indexes(db));
}


/*********************************************************************
 *
 * class history_man_impl
 *
 *********************************************************************/

w_rc_t history_man_impl::bulkload(ss_m* db, int w_num)
{
    assert (_ptable);
    TRACE( TRACE_ALWAYS, "Loading (%s) table ...\n", _ptable->name());

    assert (false); // not implemented
    //_ptable->_tpccrnd.init_random(15);

    W_DO(db->begin_xct());

    /* 1. create the history table */
    W_DO(_ptable->create_table(db));

    /* 2. append the tuples */
    append_file_i file_append(_ptable->_fid);

    // add tuples to the table 
    register int count = 1;
    register int mark = COMMIT_ACTION_COUNT;
    history_tuple ah_tuple(_ptable);

    rep_row_t reprow(_pts);
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    for (int c_id = 1; c_id <= CUSTOMERS_PER_DISTRICT; c_id++) {

                assert (false); // not implemented
		//_ptable->random(&ah_tuple, c_id, d_id, w_id);

                // append it to the table
                tsz = format(&ah_tuple, reprow);
                assert (reprow._dest);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(reprow._dest, tsz),
					    ah_tuple._rid));

		if (count >= mark) {
		    W_DO(db->commit_xct());
                    TRACE( TRACE_DEBUG, "table(%s): %d\n", _ptable->name(), count);
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;
	    }
	}
    }
    W_DO(db->commit_xct());

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _ptable->name(), count);

    return (bulkload_all_indexes(db));
}


/*********************************************************************
 *
 * class new_order_man_impl
 *
 *********************************************************************/

w_rc_t new_order_man_impl::bulkload(ss_m* db, int w_num)
{
    int o_id_lo = CUSTOMERS_PER_DISTRICT - NU_ORDERS_PER_DISTRICT + 1;
    if (o_id_lo < 0) {
	o_id_lo = CUSTOMERS_PER_DISTRICT - (CUSTOMERS_PER_DISTRICT / 3) + 1;
	cerr << "**** WARNING **** NU_ORDERS_PER_DISTRICT is > CUSTOMERS_PER_DISTRICT" << endl;
	cerr << "                  Loading New-Order with 1/3 of CUSTOMERS_PER_DISTRICT" << endl;
    }

    assert (_ptable);
    TRACE( TRACE_ALWAYS, "Loading (%s) table ...\n", _ptable->name());

    assert (false); // not implemented
    //_ptable->_tpccrnd.init_random(99);

    W_DO(db->begin_xct());

    /* 1. create the new_order table */
    W_DO(_ptable->create_table(db));

    /* 2. append tuples */
    append_file_i file_append(_ptable->_fid);
    // add tuples to the table 
    register int count = 1;
    register int mark = COMMIT_ACTION_COUNT;
    new_order_tuple anu_tuple(_ptable);

    rep_row_t reprow(_pts);
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    for (int o_id = o_id_lo ; o_id <= CUSTOMERS_PER_DISTRICT; o_id++) {

                // generate a random tuple
                assert (false); // not implemented
		//_ptable->random(&anu_tuple, o_id, d_id, w_id);

                // append it to the table
                tsz = format(&anu_tuple, reprow);
                assert (reprow._dest);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(reprow._dest, tsz),
					    anu_tuple._rid));

		if (count >= mark) {
		    W_DO(db->commit_xct());
                    TRACE( TRACE_DEBUG, "table(%s): %d\n", _ptable->name(), count);
		    W_DO(db->begin_xct());
                    mark += COMMIT_ACTION_COUNT;
		}
		count++;
	    }
	}
    }
    W_DO(db->commit_xct());

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _ptable->name(), count);

    return (bulkload_all_indexes(db));
}


/*********************************************************************
 *
 * class order_man_impl
 *
 *********************************************************************/

void order_man_impl::produce_cnt_array(int w_num, pthread_mutex_t* parray_mutex)
{
    TRACE( TRACE_DEBUG, "building cnt_array...\n");

    // first init and lock the mutex
    assert (parray_mutex);
    _pcnt_array_mutex = parray_mutex;
    pthread_mutex_lock(_pcnt_array_mutex);
    
    // then create the cnt_array
    _pcnt_array = (int*)malloc(sizeof(int)*w_num*DISTRICTS_PER_WAREHOUSE*CUSTOMERS_PER_DISTRICT);
    assert(_pcnt_array);
    for (int i=0; i<w_num; i++) {
        assert (false); // not implemented	
        //_ptable->_tpccrnd.random_ol_cnt((short*)_pcnt_array+i*DISTRICTS_PER_WAREHOUSE*CUSTOMERS_PER_DISTRICT);
    }
    
}

w_rc_t order_man_impl::bulkload(ss_m* db, int w_num)
{
    assert (_pcnt_array);
    return (bulkload(db, w_num, _pcnt_array));
}


w_rc_t order_man_impl::bulkload(ss_m* db, int w_num, int* cnt_array)
{
    assert (_ptable);
    TRACE( TRACE_ALWAYS, "Loading (%s) table ...\n", _ptable->name());

    int index = 0;

    assert (false); // not implemented
    //_ptable->_tpccrnd.init_random(42);

    W_DO(db->begin_xct());

    /* 1. create the order table */
    W_DO(_ptable->create_table(db));

    /* 2. append tuples */
    append_file_i file_append(_ptable->_fid);

    // add tuples to the table 
    register int count = 1;
    register int mark = COMMIT_ACTION_COUNT;
    order_tuple ao_tuple(_ptable);

    rep_row_t reprow(_pts);
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {

            assert (false); // not implemented
	    //_ptable->_tpccrnd.seed_1_3000();

	    for (int o_id = 1; o_id <= CUSTOMERS_PER_DISTRICT; o_id++) {
		int ol_cnt = cnt_array[index++];

                // generate a random tuple
                assert (false); // not implemented                                
		//_ptable->random(&ao_tuple, o_id, _ptable->_tpccrnd.random_1_3000(), d_id, w_id, ol_cnt);

                // append it to the table
                tsz = format(&ao_tuple, reprow);
                assert (reprow._dest);
		W_DO(file_append.create_rec(vec_t(), 0,
					    vec_t(reprow._dest, tsz),
					    ao_tuple._rid));

		if (count >= mark) {
		    W_DO(db->commit_xct());
                    TRACE( TRACE_DEBUG, "table(%s): %d\n", _ptable->name(), count);
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
    
    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _ptable->name(), count);

    return (bulkload_all_indexes(db));
}


/*********************************************************************
 *
 * class order_line_man_impl
 *
 *********************************************************************/

w_rc_t order_line_man_impl::bulkload(ss_m* db, int w_num)
{
    // get cnt_array mutex 
    assert (_pcnt_array_mutex);
    CRITICAL_SECTION(cs, *_pcnt_array_mutex);
    assert (_pcnt_array);

    // do the loading
    return (bulkload(db, w_num, _pcnt_array));
}


w_rc_t order_line_man_impl::bulkload(ss_m* db, int w_num, int* cnt_array)
{
    assert (_ptable);
    TRACE( TRACE_ALWAYS, "Loading (%s) table ...\n", _ptable->name());

    int index = 0;
    
    //  init_random(42, 13);
    assert (false); // not implemented
    //_ptable->_tpccrnd.init_random(47);

    W_DO(db->begin_xct());

    /* 1. create the order_line table */
    W_DO(_ptable->create_table(db));

    /* 2. append tuples */
    append_file_i file_append(_ptable->_fid);

    // add tuples to the table 
    int count = 1;
    int mark = COMMIT_ACTION_COUNT;
    order_line_tuple aol_tuple(_ptable);
                    
    rep_row_t reprow(_pts);
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id++) {
	    
            assert (false); // not implemented
            //_ptable->_tpccrnd.seed_1_3000();

	    for (int o_id = 1; o_id <= CUSTOMERS_PER_DISTRICT; o_id++) {
		int ol_cnt = cnt_array[index++];
	  
		for (int ol_id = 1; ol_id <= ol_cnt; ol_id++) {
                    // generate a random tuple
		    if (o_id < 2101) {
                        assert (false); // not implemented
			//_ptable->random(&aol_tuple, o_id, d_id, w_id, ol_id);
		    }
		    else {
                        assert (false); // not implemented
			//_ptable->random(&aol_tuple, o_id, d_id, w_id, ol_id, false);
		    }

                    // append it to the table
                    tsz = format(&aol_tuple, reprow);
                    assert (reprow._dest);
		    W_DO(file_append.create_rec(vec_t(), 0,
						vec_t(reprow._dest, tsz),
						aol_tuple._rid));

		    if (count >= mark) {
			W_DO(db->commit_xct());
                        TRACE( TRACE_DEBUG, "table(%s): %d\n", _ptable->name(), count);
			W_DO(db->begin_xct());
                        mark += COMMIT_ACTION_COUNT;
		    }
		    count++;
		}
	    }
	}
    }
    W_DO(db->commit_xct());

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _ptable->name(), count);

    return (bulkload_all_indexes(db));
}


/*********************************************************************
 *
 * class item_man_impl
 *
 *********************************************************************/

w_rc_t item_man_impl::bulkload(ss_m* db, int /* w_num */)
{
    assert (_ptable);
    TRACE( TRACE_ALWAYS, "Loading (%s) table ...\n", _ptable->name());

    assert (false); // not implemented
    //_ptable->_tpccrnd.init_random(13);

    W_DO(db->begin_xct());

    /* 1. create the item table */
    W_DO(_ptable->create_table(db));

    /* 2. append tuples */
    append_file_i file_append(_ptable->_fid);

    // add tuples to the table 
    register int count = 0;
    register int mark = COMMIT_ACTION_COUNT;
    item_tuple ai_tuple(_ptable);

    rep_row_t reprow(_pts);
    int tsz = 0;

    for (int i_id = 1; i_id <= ITEMS; i_id++) {
        
        // generate a random tuple
        assert (false); // not implemented
	//_ptable->random(&ai_tuple, i_id);

        // append it to the table
        tsz = format(&ai_tuple, reprow);
        assert (reprow._dest);
	W_DO(file_append.create_rec(vec_t(), 0,
				    vec_t(reprow._dest, tsz),
				    ai_tuple._rid));

	if (count >= mark) {
	    W_DO(db->commit_xct());
            TRACE( TRACE_DEBUG, "table(%s): %d\n", _ptable->name(), count);
	    W_DO(db->begin_xct());
            mark += COMMIT_ACTION_COUNT;
	}
	count++;
    }
    W_DO(db->commit_xct());

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _ptable->name(), count);

    return (bulkload_all_indexes(db));
}


/*********************************************************************
 *
 * class stock_man_impl
 *
 *********************************************************************/

w_rc_t stock_man_impl::bulkload(ss_m* db, int w_num)
{
    assert (_ptable);
    TRACE( TRACE_ALWAYS, "Loading (%s) table ...\n", _ptable->name());

    assert (false); // not implemented
    //_ptable->_tpccrnd.init_random(7);

    W_DO(db->begin_xct());

    /* 1. create the stock table */
    W_DO(_ptable->create_table(db));

    /* 2. append tuples */
    append_file_i file_append(_ptable->_fid);

    // add tuples to the table 
    register int count = 0;
    register int mark = COMMIT_ACTION_COUNT;
    stock_tuple as_tuple(_ptable);

    rep_row_t reprow(_pts);
    int tsz = 0;

    for (int w_id = 1; w_id <= w_num; w_id++) {
	for (int s_id = 1; s_id <= STOCK_PER_WAREHOUSE; s_id++) {
            
            // generate a random tuple
            assert (false); // not implemented
	    //_ptable->random(&as_tuple, s_id, w_id);

            // append it to the table
            tsz = format(&as_tuple, reprow);
            assert (reprow._dest);
	    W_DO(file_append.create_rec(vec_t(), 0,
					vec_t(reprow._dest, tsz),
					as_tuple._rid));

	    if (count >= mark) {
		W_DO(db->commit_xct());
                TRACE( TRACE_DEBUG, "table(%s): %d\n", _ptable->name(), count);
		W_DO(db->begin_xct());
                mark += COMMIT_ACTION_COUNT;
	    }
	    count++;
	}
    }
    W_DO(db->commit_xct());

    TRACE( TRACE_DEBUG, "(%s) # of records inserted: %d\n",
           _ptable->name(), count);

    return (bulkload_all_indexes(db));
}
