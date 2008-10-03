/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_env.cpp
 *
 *  @brief:  Declaration of the Shore TPC-C environment (database)
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"


using namespace shore;


ENTER_NAMESPACE(tpcc);


/** Exported variables */

ShoreTPCCEnv* _g_shore_env;


/** Exported functions */


/******************************************************************** 
 *
 *  @fn:    print_trx_stats
 *
 *  @brief: Prints trx statistics
 *
 ********************************************************************/

void tpcc_stats_t::print_trx_stats() 
{   
    TRACE( TRACE_STATISTICS, "=====================================\n");
    TRACE( TRACE_STATISTICS, "TPC-C Database transaction statistics\n");
    TRACE( TRACE_STATISTICS, "NEW-ORDER\n");
    CRITICAL_SECTION(no_cs,  _no_lock);
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _no_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _no_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_no_att-_no_com));
    TRACE( TRACE_STATISTICS, "PAYMENT\n");
    CRITICAL_SECTION(pay_cs, _pay_lock);
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _pay_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _pay_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_pay_att-_pay_com));
    TRACE( TRACE_STATISTICS, "ORDER-STATUS\n");
    CRITICAL_SECTION(ord_cs, _ord_lock);
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _ord_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _ord_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_ord_att-_ord_com));
    TRACE( TRACE_STATISTICS, "DELIVERY\n");
    CRITICAL_SECTION(del_cs, _del_lock);
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _del_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _del_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_del_att-_del_com));
    TRACE( TRACE_STATISTICS, "STOCK-LEVEL\n");
    CRITICAL_SECTION(sto_cs, _sto_lock);
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _sto_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _sto_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_sto_att-_sto_com));
    TRACE( TRACE_STATISTICS, "=====================================\n");
}



/********
 ******** Caution: The functions below should be invoked inside
 ********          the context of a smthread
 ********/


/****************************************************************** 
 *
 * @fn:    loaddata()
 *
 * @brief: Loads the data for all the TPCC tables, given the current
 *         scaling factor value. During the loading the SF cannot be
 *         changed.
 *
 ******************************************************************/

w_rc_t ShoreTPCCEnv::loaddata() 
{
    /* 0. lock the loading status and the scaling factor */
    CRITICAL_SECTION(load_cs, _load_mutex);
    if (_loaded) {
        TRACE( TRACE_TRX_FLOW, 
               "Env already loaded. Doing nothing...\n");
        return (RCOK);
    }        
    CRITICAL_SECTION(scale_cs, _scaling_mutex);

    /* 1. create the loader threads */

    int num_tbl = _table_desc_list.size();
    const char* loaddatadir = _dev_opts[SHORE_DEF_DEV_OPTIONS[3][0]].c_str();
    int cnt = 0;

    TRACE( TRACE_DEBUG, "Loaddir (%s)\n", loaddatadir);

    guard<table_loading_smt_t> loaders[SHORE_TPCC_TABLES];

    // manually create the loading threads
    loaders[0] = new wh_loader_t(c_str("ld-WH"), _pssm, _pwarehouse_man,
                                 &_warehouse_desc, _scaling_factor, loaddatadir);
    loaders[1] = new dist_loader_t(c_str("ld-DIST"), _pssm, _pdistrict_man,
                                   &_district_desc, _scaling_factor, loaddatadir);
    loaders[2] = new st_loader_t(c_str("ld-ST"), _pssm, _pstock_man,
                                 &_stock_desc, _scaling_factor, loaddatadir);
    loaders[3] = new ol_loader_t(c_str("ld-OL"), _pssm, _porder_line_man,
                                 &_order_line_desc, _scaling_factor, loaddatadir);
    loaders[4] = new cust_loader_t(c_str("ld-CUST"), _pssm, _pcustomer_man,
                                   &_customer_desc, _scaling_factor, loaddatadir);
    loaders[5] = new hist_loader_t(c_str("ld-HIST"), _pssm, _phistory_man,
                                   &_history_desc, _scaling_factor, loaddatadir);
    loaders[6] = new ord_loader_t(c_str("ld-ORD"), _pssm, _porder_man,
                                  &_order_desc, _scaling_factor, loaddatadir);
    loaders[7] = new no_loader_t(c_str("ld-NO"), _pssm, _pnew_order_man,
                                 &_new_order_desc, _scaling_factor, loaddatadir);
    loaders[8] = new it_loader_t(c_str("ld-IT"), _pssm, _pitem_man,
                                 &_item_desc, _scaling_factor, loaddatadir);

    time_t tstart = time(NULL);    
    

//     tpcc_table_t* ptable   = NULL;
//     table_man_t*  pmanager = NULL;
//     tpcc_table_list_iter table_desc_iter;
//     table_man_list_iter table_man_iter;
//     for ( table_desc_iter = _table_desc_list.begin() ,
//               table_man_iter = _table_man_list.begin(); 
//           table_desc_iter != _table_desc_list.end(); 
//           table_desc_iter++, table_man_iter++)
//         {
//             ptable   = *table_desc_iter;
//             pmanager = *table_man_iter;

//             loaders[cnt] = new table_loading_smt_t(c_str("ld%d", cnt), 
//                                                    _pssm, 
//                                                    pmanager, 
//                                                    ptable, 
//                                                    _scaling_factor, 
//                                                    loaddatadir);
//             cnt++;
//        }

#if 0
    /* 3. fork the loading threads (PARALLEL) */
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->fork();
    }

    /* 4. join the loading threads */
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->join();        
        if (loaders[i]->rv()) {
            TRACE( TRACE_ALWAYS, "Error while loading (%s) *****\n",
                   loaders[i]->table()->name());
            delete loaders[i];
            assert (0); // should not happen
            return RC(se_ERROR_IN_LOAD);
        }
        TRACE( TRACE_TRX_FLOW, "Loader (%d) [%s] joined...\n", 
               i, loaders[i]->table()->name());
        //        delete loaders[i];
    }    
#else 
    /* 3. fork & join the loading threads SERIALLY */
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->fork();
	loaders[i]->join();        
        if (loaders[i]->rv()) {
            TRACE( TRACE_ALWAYS, "Error while loading (%s) *****\n",
                   loaders[i]->table()->name());
            //            delete loaders[i];
            assert (0); // should not happen
            return RC(se_ERROR_IN_LOAD);
        }        
        //        delete loaders[i];
    }
#endif
    time_t tstop = time(NULL);

    /* 5. print stats */
    TRACE( TRACE_STATISTICS, "Loading finished. %d table loaded in (%d) secs...\n",
           num_tbl, (tstop - tstart));

    /* 6. notify that the env is loaded */
    _loaded = true;

    return (RCOK);
}



/****************************************************************** 
 *
 * @fn:    check_consistency()
 *
 * @brief: Iterates over all tables and checks consistency between
 *         the values stored in the base table (file) and the 
 *         corresponding indexes.
 *
 ******************************************************************/

w_rc_t ShoreTPCCEnv::check_consistency()
{
    /* 1. create the checker threads */
    int num_tbl = _table_desc_list.size();
    int cnt = 0;

    guard<thread_t> checkers[SHORE_TPCC_TABLES];

    // manually create the loading threads
    checkers[0] = new wh_checker_t(c_str("chk-WH"), _pssm, 
                                   _pwarehouse_man, &_warehouse_desc);
    checkers[1] = new dist_checker_t(c_str("chk-DIST"), _pssm, 
                                     _pdistrict_man, &_district_desc);
    checkers[2] = new st_checker_t(c_str("chk-ST"), _pssm, 
                                   _pstock_man, &_stock_desc);
    checkers[3] = new ol_checker_t(c_str("chk-OL"), _pssm, 
                                   _porder_line_man, &_order_line_desc);
    checkers[4] = new cust_checker_t(c_str("chk-CUST"), _pssm, 
                                     _pcustomer_man, &_customer_desc);
    checkers[5] = new hist_checker_t(c_str("chk-HIST"), _pssm, 
                                     _phistory_man, &_history_desc);
    checkers[6] = new ord_checker_t(c_str("chk-ORD"), _pssm, 
                                    _porder_man, &_order_desc);
    checkers[7] = new no_checker_t(c_str("chk-NO"), _pssm, 
                                   _pnew_order_man, &_new_order_desc);
    checkers[8] = new it_checker_t(c_str("chk-IT"), _pssm, 
                                   _pitem_man, &_item_desc);


//     tpcc_table_t* ptable   = NULL;
//     table_man_t*  pmanager = NULL;
//     tpcc_table_list_iter table_desc_iter;
//     table_man_list_iter table_man_iter;
//     for ( table_desc_iter = _table_desc_list.begin() ,
//               table_man_iter = _table_man_list.begin(); 
//           table_desc_iter != _table_desc_list.end(); 
//           table_desc_iter++, table_man_iter++)
//         {
//             ptable   = *table_desc_iter;
//             pmanager = *table_man_iter;

//             checkers[cnt] = new table_checking_smt_t(c_str("chk%d", cnt), 
//                                                      _pssm, pmanager, ptable);
//             cnt++;
//         }

#if 1
    /* 2. fork the threads */
    cnt = 0;
    time_t tstart = time(NULL);
    for(int i=0; i<num_tbl; i++) {
	checkers[i]->fork();
    }

    /* 3. join the threads */
    cnt = 0;
    for(int i=0; i < num_tbl; i++) {
	checkers[i]->join();
    }    
    time_t tstop = time(NULL);
#else
    /* 2. fork & join the threads SERIALLY */
    cnt = 0;
    time_t tstart = time(NULL);
    for(int i=0; i<num_tbl; i++) {
	checkers[i]->fork();
	checkers[i]->join();
    }
    time_t tstop = time(NULL);
#endif
    /* 4. print stats */
    TRACE( TRACE_DEBUG, "Checking finished in (%d) secs...\n",
           (tstop - tstart));
    TRACE( TRACE_DEBUG, "%d tables checked...\n", num_tbl);
    return (RCOK);
}


/****************************************************************** 
 *
 * @fn:    warmup()
 *
 * @brief: Touches the entire database - For memory-fitting databases
 *         this is enough to bring it to load it to memory
 *
 ******************************************************************/

w_rc_t ShoreTPCCEnv::warmup()
{
//     int num_tbl = _table_desc_list.size();
//     table_man_t*  pmanager = NULL;
//     table_man_list_iter table_man_iter;

//     time_t tstart = time(NULL);

//     for ( table_man_iter = _table_man_list.begin(); 
//           table_man_iter != _table_man_list.end(); 
//           table_man_iter++)
//         {
//             pmanager = *table_man_iter;
//             W_DO(pmanager->check_all_indexes_together(db()));
//         }

//     time_t tstop = time(NULL);

//     /* 2. print stats */
//     TRACE( TRACE_DEBUG, "Checking of (%d) tables finished in (%d) secs...\n",
//            num_tbl, (tstop - tstart));

    return (check_consistency());
}


/******************************************************************** 
 *
 *  @fn:    set_sf/qf
 *
 *  @brief: Set the scaling and queried factors
 *
 ********************************************************************/

void ShoreTPCCEnv::set_qf(const int aQF)
{
    if ((aQF >= 0) && (aQF <= _scaling_factor)) {
        CRITICAL_SECTION( cs, _queried_mutex);
        TRACE( TRACE_ALWAYS, "New Queried Factor: %d\n", aQF);
        _queried_factor = aQF;
    }
    else {
        TRACE( TRACE_ALWAYS, "Invalid queried factor input: %d\n", aQF);
    }
}


void ShoreTPCCEnv::set_sf(const int aSF)
{
    if (aSF > 0) {
        CRITICAL_SECTION( cs, _scaling_mutex);
        TRACE( TRACE_ALWAYS, "New Scaling factor: %d\n", aSF);
        _scaling_factor = aSF;
    }
    else {
        TRACE( TRACE_ALWAYS, "Invalid scaling factor input: %d\n", aSF);
    }
}



/******************************************************************** 
 *
 *  @fn:    dump
 *
 *  @brief: Print infor for all the tables in the environment
 *
 ********************************************************************/

void ShoreTPCCEnv::dump()
{
    table_man_t* ptable_man = NULL;
    int cnt = 0;
    for(table_man_list_iter table_man_iter = _table_man_list.begin(); 
        table_man_iter != _table_man_list.end(); table_man_iter++)
        {
            ptable_man = *table_man_iter;
            ptable_man->print_table(this->_pssm);
            cnt++;
        }
    
}


/********************************************************************
 *
 * Make sure the WH table is padded to one record per page
 *
 * For the dataset sizes we can afford to run, all WH records fit on a
 * single page, leading to massive latch contention even though each
 * thread updates a different WH tuple.
 *
 * If the WH records are big enough, do nothing; otherwise replicate
 * the existing WH table and index with padding, drop the originals,
 * and install the new files in the directory.
 *
 *********************************************************************/

int ShoreTPCCEnv::post_init() 
{
    TRACE( TRACE_ALWAYS, "Checking for WH record padding...\n");

    W_COERCE(db()->begin_xct());
    w_rc_t rc = _post_init_impl();
    if(rc.is_error()) {
	cerr << "-> WH padding failed with: " << rc << endl;
	db()->abort_xct();
	return (rc.err_num());
    }
    else {
	TRACE( TRACE_ALWAYS, "-> Done\n");
	db()->commit_xct();
	return (0);
    }
}


/********************************************************************* 
 *
 *  @fn:    _post_init_impl
 *
 *  @brief: Makes sure the WH table is padded to one record per page
 *
 *********************************************************************/ 

w_rc_t ShoreTPCCEnv::_post_init_impl() 
{
    ss_m* db = this->db();
    
    // lock the WH table 
    warehouse_t* wh = warehouse();
    index_desc_t* idx = wh->indexes();
    int icount = wh->index_count();
    W_DO(wh->find_fid(db));
    stid_t wh_fid = wh->fid();

    // lock the table and index(es) for exclusive access
    W_DO(db->lock(wh_fid, EX));
    for(int i=0; i < icount; i++) {
	W_DO(idx[i].check_fid(db));
	W_DO(db->lock(idx[i].fid(), EX));
    }

    guard<ats_char_t> pts = new ats_char_t(wh->maxsize());
    
    /* copy and pad all tuples smaller than 4k

       WARNING: this code assumes that existing tuples are packed
       densly so that all padded tuples are added after the last
       unpadded one
    */
    bool eof;
    static int const PADDED_SIZE = 4096; // we know you can't fit two 4k records on a single page
    array_guard_t<char> padding = new char[PADDED_SIZE];
    std::vector<rid_t> hit_list;
    {
	guard<warehouse_man_impl::table_iter> iter;
	{
	    warehouse_man_impl::table_iter* tmp;
	    W_DO(warehouse_man()->get_iter_for_file_scan(db, tmp));
	    iter = tmp;
	}

	int count = 0;
	warehouse_man_impl::table_tuple row(wh);
	rep_row_t arep(pts);
	int psize = wh->maxsize()+1;

	W_DO(iter->next(db, eof, row));	
	while (1) {
	    pin_i* handle = iter->cursor();
	    if (!handle) {
		TRACE(TRACE_ALWAYS, "\n-> Reached EOF. Search complete\n");
		break;
	    }

	    // figure out how big the old record is
	    int hsize = handle->hdr_size();
	    int bsize = handle->body_size();
	    if(bsize == psize) {
		TRACE(TRACE_ALWAYS, "\n-> Found padded WH record. Stopping search\n");
		break;
	    }
	    else if (bsize > psize) {
		// too big... shrink it down to save on logging
		handle->truncate_rec(bsize - psize);
	    }
	    else {
		// copy and pad the record (and mark the old one for deletion)
		rid_t new_rid;
		vec_t hvec(handle->hdr(), hsize);
		vec_t dvec(handle->body(), bsize);
		vec_t pvec(padding, psize-bsize);
		W_DO(db->create_rec(wh_fid, hvec, PADDED_SIZE, dvec, new_rid));
		W_DO(db->append_rec(new_rid, pvec, false));

                // mark the old record for deletion
		hit_list.push_back(handle->rid());

		// update the index(es)
		vec_t rvec(&row._rid, sizeof(rid_t));
		vec_t nrvec(&new_rid, sizeof(new_rid));
		for(int i=0; i < icount; i++) {
		    int key_sz = warehouse_man()->format_key(idx+i, &row, arep);
		    vec_t kvec(arep._dest, key_sz);

		    /* destroy the old mapping and replace it with the new
		       one.  If it turns out this is super-slow, we can
		       look into probing the index with a cursor and
		       updating it directly.
		    */
		    stid_t fid = idx[i].fid();
		    W_DO(db->destroy_assoc(fid, kvec, rvec));

		    // now put the entry back with the new rid
		    W_DO(db->create_assoc(fid, kvec, nrvec));
		}
	    }
	    
	    // next!
	    count++;
	    fprintf(stderr, ".");
	    W_DO(iter->next(db, eof, row));
	}
        fprintf(stderr, "\n");

	// put the iter out of scope
    }

    // delete the old records     
    int hlsize = hit_list.size();
    TRACE(TRACE_ALWAYS, "-> Deleting (%d) old unpadded records\n", hlsize);
    for(int i=0; i < hlsize; i++) {
	W_DO(db->destroy_rec(hit_list[i]));
    }

    return (RCOK);
}


/******************************************************************** 
 *
 * TPC-C TRXS
 *
 * (1) The run_XXX functions are wrappers to the real transactions
 * (2) The xct_XXX functions are the implementation of the transactions
 *
 ********************************************************************/


/******************************************************************** 
 *
 * TPC-C TRXs Wrappers
 *
 * @brief: They are wrappers to the functions that execute the transaction
 *         body. Their responsibility is to:
 *
 *         1. Prepare the corresponding input
 *         2. Check the return of the trx function and abort the trx,
 *            if something went wrong
 *         3. Update the tpcc db environment statistics
 *
 ********************************************************************/


/* --- with input specified --- */

w_rc_t ShoreTPCCEnv::run_new_order(const int xct_id, 
                                   new_order_input_t& anoin,
                                   trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. NEW-ORDER...\n", xct_id);     
    
    w_rc_t e = xct_new_order(&anoin, xct_id, atrt);
    if (e.is_error()) {
        TRACE( TRACE_ALWAYS, "Xct (%d) NewOrder aborted [0x%x]\n", 
               xct_id, e.err_num());
        
        if (_measure == MST_MEASURE) {
            _tpcc_stats.inc_no_att();
            _tmp_tpcc_stats.inc_no_att();
            _env_stats.inc_trx_att();
        }

	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) NewOrder abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

        // (ip) could retry
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) NewOrder completed\n", xct_id);

    if (_measure == MST_MEASURE) {
        _tpcc_stats.inc_no_com();
        _tmp_tpcc_stats.inc_no_com();
        _env_stats.inc_trx_com();
    }

    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::run_payment(const int xct_id, 
                                 payment_input_t& apin,
                                 trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. PAYMENT...\n", xct_id);     
    
    w_rc_t e = xct_payment(&apin, xct_id, atrt);
    if (e.is_error()) {
        TRACE( TRACE_ALWAYS, "Xct (%d) Payment aborted [0x%x]\n", 
               xct_id, e.err_num());

        if (_measure == MST_MEASURE) {
            _tpcc_stats.inc_pay_att();
            _tmp_tpcc_stats.inc_pay_att();
            _env_stats.inc_trx_att();
        }
	
	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) Payment abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

        // (ip) could retry
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) Payment completed\n", xct_id);

    if (_measure == MST_MEASURE) {
        _tpcc_stats.inc_pay_com();
        _tmp_tpcc_stats.inc_pay_com();
        _env_stats.inc_trx_com();
    }

    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::run_order_status(const int xct_id, 
                                      order_status_input_t& aordstin,
                                      trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. ORDER-STATUS...\n", xct_id);     
    
    w_rc_t e = xct_order_status(&aordstin, xct_id, atrt);
    if (e.is_error()) {
        TRACE( TRACE_ALWAYS, "Xct (%d) OrderStatus aborted [0x%x]\n", 
               xct_id, e.err_num());

        if (_measure == MST_MEASURE) {
            _tpcc_stats.inc_ord_att();
            _tmp_tpcc_stats.inc_ord_att();
            _env_stats.inc_trx_att();
        }
	
	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) OrderStatus abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

        // (ip) could retry
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) OrderStatus completed\n", xct_id);

    if (_measure == MST_MEASURE) {
        _tpcc_stats.inc_ord_com();
        _tmp_tpcc_stats.inc_ord_com();
        _env_stats.inc_trx_com();
    }

    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::run_delivery(const int xct_id, 
                                  delivery_input_t& adelin,
                                  trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. DELIVERY...\n", xct_id);     
    
    w_rc_t e = xct_delivery(&adelin, xct_id, atrt);
    if (e.is_error()) {
        TRACE( TRACE_ALWAYS, "Xct (%d) Delivery aborted [0x%x]\n", 
               xct_id, e.err_num());

        if (_measure == MST_MEASURE) {        
            _tpcc_stats.inc_del_att();
            _tmp_tpcc_stats.inc_del_att();
            _env_stats.inc_trx_att();
        }

	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) Delivery abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

        // (ip) could retry
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) Delivery completed\n", xct_id);

    if (_measure == MST_MEASURE) {    
        _tpcc_stats.inc_del_com();
        _tmp_tpcc_stats.inc_del_com();
        _env_stats.inc_trx_com();
    }

    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::run_stock_level(const int xct_id, 
                                     stock_level_input_t& astoin,
                                     trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. STOCK-LEVEL...\n", xct_id);     
    
    w_rc_t e = xct_stock_level(&astoin, xct_id, atrt);
    if (e.is_error()) {
        TRACE( TRACE_ALWAYS, "Xct (%d) StockLevel aborted [0x%x]\n", 
               xct_id, e.err_num());

        if (_measure == MST_MEASURE) {
            _tpcc_stats.inc_sto_att();
            _tmp_tpcc_stats.inc_sto_att();
            _env_stats.inc_trx_att();
        }

	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) StockLevel abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

        // (ip) could retry
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) StockLevel completed\n", xct_id);

    if (_measure == MST_MEASURE) {
        _tpcc_stats.inc_sto_com();
        _tmp_tpcc_stats.inc_sto_com();
        _env_stats.inc_trx_com();
    }

    return (RCOK); 
}



/* --- without input specified --- */

w_rc_t ShoreTPCCEnv::run_new_order(const int xct_id, 
                                   trx_result_tuple_t& atrt,
                                   int specificWH)
{
    new_order_input_t noin = create_no_input(_queried_factor, specificWH);
    return (run_new_order(xct_id, noin, atrt));
}


w_rc_t ShoreTPCCEnv::run_payment(const int xct_id, 
                                 trx_result_tuple_t& atrt,
                                   int specificWH)
{
    payment_input_t pin = create_payment_input(_queried_factor, specificWH);
    return (run_payment(xct_id, pin, atrt));
}


w_rc_t ShoreTPCCEnv::run_order_status(const int xct_id, 
                                      trx_result_tuple_t& atrt,
                                      int specificWH)
{
    order_status_input_t ordin = create_order_status_input(_queried_factor, specificWH);
    return (run_order_status(xct_id, ordin, atrt));
}


w_rc_t ShoreTPCCEnv::run_delivery(const int xct_id, 
                                  trx_result_tuple_t& atrt,
                                  int specificWH)
{
    delivery_input_t delin = create_delivery_input(_queried_factor, specificWH);
    return (run_delivery(xct_id, delin, atrt));
}


w_rc_t ShoreTPCCEnv::run_stock_level(const int xct_id, 
                                     trx_result_tuple_t& atrt,
                                     int specificWH)
{
    stock_level_input_t slin = create_stock_level_input(_queried_factor, specificWH);
    return (run_stock_level(xct_id, slin, atrt));
 }



/******************************************************************** 
 *
 * @note: The functions below are private, the corresponding run_XXX are
 *        their public wrappers. The run_XXX are required because they
 *        do the trx abort in case something goes wrong inside the body
 *        of each of the transactions.
 *
 ********************************************************************/


// uncomment the line below if want to dump (part of) the trx results
//#define PRINT_TRX_RESULTS


/******************************************************************** 
 *
 * TPC-C NEW_ORDER
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::xct_new_order(new_order_input_t* pnoin, 
                                   const int xct_id, 
                                   trx_result_tuple_t& trt)
{
    // ensure a valid environment
    assert (pnoin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // get a timestamp
    time_t tstamp = time(NULL);

    // new_order trx touches 8 tables:
    // warehouse, district, customer, neworder, order, item, stock, orderline
    row_impl<warehouse_t>* prwh = _pwarehouse_man->get_tuple();
    assert (prwh);

    row_impl<district_t>* prdist = _pdistrict_man->get_tuple();
    assert (prdist);

    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

    row_impl<new_order_t>* prno = _pnew_order_man->get_tuple();
    assert (prno);

    row_impl<order_t>* prord = _porder_man->get_tuple();
    assert (prord);

    row_impl<item_t>* pritem = _pitem_man->get_tuple();
    assert (pritem);

    row_impl<stock_t>* prst = _pstock_man->get_tuple();
    assert (prst);

    row_impl<order_line_t>* prol = _porder_line_man->get_tuple();
    assert (prol);

    
    trt.reset(UNSUBMITTED, xct_id);
    rep_row_t areprow(_pcustomer_man->ts());

    // allocate space for the biggest of the 8 table representations
    areprow.set(_customer_desc.maxsize()); 

    prwh->_rep = &areprow;
    prdist->_rep = &areprow;

    prcust->_rep = &areprow;
    prno->_rep = &areprow;
    prord->_rep = &areprow;
    pritem->_rep = &areprow;
    prst->_rep = &areprow;
    prol->_rep = &areprow;


    /* 0. initiate transaction */
    W_DO(_pssm->begin_xct());


    /* SELECT c_discount, c_last, c_credit, w_tax
     * FROM customer, warehouse
     * WHERE w_id = :w_id AND c_w_id = w_id AND c_d_id = :d_id AND c_id = :c_id
     *
     * plan: index probe on "W_INDEX", index probe on "C_INDEX"
     */

    /* 1. retrieve warehouse (read-only) */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d NO:warehouse-idx-probe (%d)\n", 
           xct_id, pnoin->_wh_id);
    W_DO(_pwarehouse_man->wh_index_probe(_pssm, prwh, pnoin->_wh_id));

    tpcc_warehouse_tuple awh;
    prwh->get_value(7, awh.W_TAX);


    /* 2. retrieve district for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d NO:district-idx-probe (%d) (%d)\n", 
           xct_id, pnoin->_wh_id, pnoin->_d_id);
    W_DO(_pdistrict_man->dist_index_probe_forupdate(_pssm, prdist, 
                                                    pnoin->_d_id, pnoin->_wh_id));

    /* SELECT d_tax, d_next_o_id
     * FROM district
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * plan: index probe on "D_INDEX"
     */

    tpcc_district_tuple adist;
    prdist->get_value(8, adist.D_TAX);
    prdist->get_value(10, adist.D_NEXT_O_ID);
    adist.D_NEXT_O_ID++;


    /* 3. retrieve customer */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d NO:customer-idx-probe (%d) (%d) (%d)\n", 
           xct_id, pnoin->_wh_id, pnoin->_d_id, pnoin->_c_id);
    W_DO(_pcustomer_man->cust_index_probe(_pssm, prcust, pnoin->_c_id, 
                                          pnoin->_wh_id, pnoin->_d_id));

    tpcc_customer_tuple  acust;
    prcust->get_value(15, acust.C_DISCOUNT);
    prcust->get_value(13, acust.C_CREDIT, 3);
    prcust->get_value(5, acust.C_LAST, 17);


    /* UPDATE district
     * SET d_next_o_id = :next_o_id+1
     * WHERE CURRENT OF dist_cur
     */

    TRACE( TRACE_TRX_FLOW, "App: %d NO:district-upd-next-o-id\n", xct_id);
    W_DO(_pdistrict_man->dist_update_next_o_id(_pssm, prdist, adist.D_NEXT_O_ID));
    double total_amount = 0;
    int all_local = 0;

    for (int item_cnt = 0; item_cnt < pnoin->_ol_cnt; item_cnt++) {

        /* 4. for all items update item, stock, and order line */
        register int ol_i_id = pnoin->items[item_cnt]._ol_i_id;
        register int ol_supply_w_id = pnoin->items[item_cnt]._ol_supply_wh_id;


        /* SELECT i_price, i_name, i_data
         * FROM item
         * WHERE i_id = :ol_i_id
         *
         * plan: index probe on "I_INDEX"
         */

        tpcc_item_tuple aitem;
        TRACE( TRACE_TRX_FLOW, "App: %d NO:item-idx-probe (%d)\n", 
               xct_id, ol_i_id);
        W_DO(_pitem_man->it_index_probe(_pssm, pritem, ol_i_id));

        pritem->get_value(4, aitem.I_DATA, 51);
        pritem->get_value(3, aitem.I_PRICE);
        pritem->get_value(2, aitem.I_NAME, 25);

        int item_amount = aitem.I_PRICE * pnoin->items[item_cnt]._ol_quantity; 
        total_amount += item_amount;
        //	info->items[item_cnt].ol_amount = amount;

        /* SELECT s_quantity, s_remote_cnt, s_data, s_dist0, s_dist1, s_dist2, ...
         * FROM stock
         * WHERE s_i_id = :ol_i_id AND s_w_id = :ol_supply_w_id
         *
         * plan: index probe on "S_INDEX"
         */

        tpcc_stock_tuple astock;
        TRACE( TRACE_TRX_FLOW, "App: %d NO:stock-idx-probe (%d) (%d)\n", 
               xct_id, ol_i_id, ol_supply_w_id);
        W_DO(_pstock_man->st_index_probe_forupdate(_pssm, prst, 
                                                   ol_i_id, ol_supply_w_id));

        prst->get_value(0, astock.S_I_ID);
        prst->get_value(1, astock.S_W_ID);
        prst->get_value(5, astock.S_YTD);
        astock.S_YTD += pnoin->items[item_cnt]._ol_quantity;
        prst->get_value(2, astock.S_REMOTE_CNT);        
        prst->get_value(3, astock.S_QUANTITY);
        astock.S_QUANTITY -= pnoin->items[item_cnt]._ol_quantity;
        if (astock.S_QUANTITY < 10) astock.S_QUANTITY += 91;
        prst->get_value(6+pnoin->_d_id, astock.S_DIST[6+pnoin->_d_id], 25);
        prst->get_value(16, astock.S_DATA, 51);

        char c_s_brand_generic;
        if (strstr(aitem.I_DATA, "ORIGINAL") != NULL && 
            strstr(astock.S_DATA, "ORIGINAL") != NULL)
            c_s_brand_generic = 'B';
        else c_s_brand_generic = 'G';

        prst->get_value(4, astock.S_ORDER_CNT);
        astock.S_ORDER_CNT++;

        if (pnoin->_wh_id != ol_supply_w_id) {
            astock.S_REMOTE_CNT++;
            all_local = 1;
        }


        /* UPDATE stock
         * SET s_quantity = :s_quantity, s_order_cnt = :s_order_cnt
         * WHERE s_w_id = :w_id AND s_i_id = :ol_i_id;
         */

        TRACE( TRACE_TRX_FLOW, "App: %d NO:stock-upd-tuple (%d) (%d)\n", 
               xct_id, astock.S_W_ID, astock.S_I_ID);
        W_DO(_pstock_man->st_update_tuple(_pssm, prst, &astock));


        /* INSERT INTO order_line
         * VALUES (o_id, d_id, w_id, ol_ln, ol_i_id, supply_w_id,
         *        '0001-01-01-00.00.01.000000', ol_quantity, iol_amount, dist)
         */

        prol->set_value(0, adist.D_NEXT_O_ID);
        prol->set_value(1, pnoin->_d_id);
        prol->set_value(2, pnoin->_wh_id);
        prol->set_value(3, item_cnt+1);
        prol->set_value(4, ol_i_id);
        prol->set_value(5, ol_supply_w_id);
        prol->set_value(6, tstamp);
        prol->set_value(7, pnoin->items[item_cnt]._ol_quantity);
        prol->set_value(8, item_amount);
        prol->set_value(9, astock.S_DIST[6+pnoin->_d_id]);

        TRACE( TRACE_TRX_FLOW, "App: %d NO:orderline-add-tuple (%d)\n", 
               xct_id, adist.D_NEXT_O_ID);
        W_DO(_porder_line_man->add_tuple(_pssm, prol));

    } /* end for loop */


    /* 5. insert row to orders and new_order */

    /* INSERT INTO orders
     * VALUES (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_ol_cnt, o_all_local)
     */

    prord->set_value(0, adist.D_NEXT_O_ID);
    prord->set_value(1, pnoin->_c_id);
    prord->set_value(2, pnoin->_d_id);
    prord->set_value(3, pnoin->_wh_id);
    prord->set_value(4, tstamp);
    prord->set_value(5, 0);
    prord->set_value(6, pnoin->_ol_cnt);
    prord->set_value(7, all_local);

    TRACE( TRACE_TRX_FLOW, "App: %d NO:order-add-tuple (%d)\n", 
           xct_id, adist.D_NEXT_O_ID);
    W_DO(_porder_man->add_tuple(_pssm, prord));


    /* INSERT INTO new_order VALUES (o_id, d_id, w_id)
     */

    prno->set_value(0, adist.D_NEXT_O_ID);
    prno->set_value(1, pnoin->_d_id);
    prno->set_value(2, pnoin->_wh_id);

    TRACE( TRACE_TRX_FLOW, "App: %d NO:new-order-add-tuple (%d) (%d) (%d)\n", 
           xct_id, pnoin->_wh_id, pnoin->_d_id, adist.D_NEXT_O_ID);
    W_DO(_pnew_order_man->add_tuple(_pssm, prno));

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prwh->print_tuple();
    prdist->print_tuple();
    prcust->print_tuple();
    prno->print_tuple();
    prord->print_tuple();
    pritem->print_tuple();
    prst->print_tuple();
    prol->print_tuple();
#endif

    _pwarehouse_man->give_tuple(prwh);
    _pdistrict_man->give_tuple(prdist);
    _pcustomer_man->give_tuple(prcust);
    _pnew_order_man->give_tuple(prno);
    _porder_man->give_tuple(prord);
    _pitem_man->give_tuple(pritem);
    _pstock_man->give_tuple(prst);
    _porder_line_man->give_tuple(prol);


    /* 6. finalize trx */
    W_DO(_pssm->commit_xct());

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);

    return (RCOK);

} // EOF: NEW_ORDER




/******************************************************************** 
 *
 * TPC-C PAYMENT
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::xct_payment(payment_input_t* ppin, 
                                 const int xct_id, 
                                 trx_result_tuple_t& trt)
{
    // ensure a valid environment    
    assert (ppin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // payment trx touches 4 tables: 
    // warehouse, district, customer, and history

    // get table tuples from the caches
    row_impl<warehouse_t>* prwh = _pwarehouse_man->get_tuple();
    assert (prwh);

    row_impl<district_t>* prdist = _pdistrict_man->get_tuple();
    assert (prdist);

    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

    row_impl<history_t>* prhist = _phistory_man->get_tuple();
    assert (prhist);


    trt.reset(UNSUBMITTED, xct_id);
    rep_row_t areprow(_pcustomer_man->ts());

    // allocate space for the biggest of the 4 table representations
    areprow.set(_customer_desc.maxsize()); 

    prwh->_rep = &areprow;
    prdist->_rep = &areprow;
    prcust->_rep = &areprow;
    prhist->_rep = &areprow;

    /* 0. initiate transaction */
    W_DO(_pssm->begin_xct());


    /* 1. retrieve warehouse for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:warehouse-idx-probe (%d)\n", 
           xct_id, ppin->_home_wh_id);

    W_DO(_pwarehouse_man->wh_index_probe_forupdate(_pssm, prwh, ppin->_home_wh_id));   


    /* 2. retrieve district for update */
    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:district-idx-probe (%d) (%d)\n", 
           xct_id, ppin->_home_wh_id, ppin->_home_d_id);

    W_DO(_pdistrict_man->dist_index_probe_forupdate(_pssm, prdist,
                                                    ppin->_home_d_id, 
                                                    ppin->_home_wh_id));


    /* 3. retrieve customer for update */

    // find the customer wh and d
    int c_w = ( ppin->_v_cust_wh_selection > 85 ? ppin->_home_wh_id : ppin->_remote_wh_id);
    int c_d = ( ppin->_v_cust_wh_selection > 85 ? ppin->_home_d_id : ppin->_remote_d_id);

    if (ppin->_v_cust_ident_selection <= 60) {

        // if (ppin->_c_id == 0) {

        /* 3a. if no customer selected already use the index on the customer name */

        /* SELECT  c_id, c_first
         * FROM customer
         * WHERE c_last = :c_last AND c_w_id = :c_w_id AND c_d_id = :c_d_id
         * ORDER BY c_first
         *
         * plan: index only scan on "C_NAME_INDEX"
         */

        assert (ppin->_v_cust_ident_selection <= 60);
        assert (ppin->_c_id == 0); // (ip) just checks the generator output

        rep_row_t lowrep(_pcustomer_man->ts());
        rep_row_t highrep(_pcustomer_man->ts());

        index_scan_iter_impl<customer_t>* c_iter;
        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-get-iter-by-name-index (%s)\n", 
               xct_id, ppin->_c_last);
        W_DO(_pcustomer_man->cust_get_iter_by_index(_pssm, c_iter, prcust, 
                                                    lowrep, highrep,
                                                    c_w, c_d, ppin->_c_last));

        vector<int> v_c_id;
        int a_c_id = 0;
        int count = 0;
        bool eof;

        W_DO(c_iter->next(_pssm, eof, *prcust));
        while (!eof) {
            count++;
            prcust->get_value(0, a_c_id);
            v_c_id.push_back(a_c_id);

            TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-iter-next (%d)\n", 
                   xct_id, a_c_id);
            W_DO(c_iter->next(_pssm, eof, *prcust));
        }
        delete c_iter;
        assert (count);

        /* find the customer id in the middle of the list */
        ppin->_c_id = v_c_id[(count+1)/2-1];
    }
    assert (ppin->_c_id>0);


    /* SELECT c_first, c_middle, c_last, c_street_1, c_street_2, c_city, 
     * c_state, c_zip, c_phone, c_since, c_credit, c_credit_lim, 
     * c_discount, c_balance, c_ytd_payment, c_payment_cnt 
     * FROM customer 
     * WHERE c_id = :c_id AND c_w_id = :c_w_id AND c_d_id = :c_d_id 
     * FOR UPDATE OF c_balance, c_ytd_payment, c_payment_cnt
     *
     * plan: index probe on "C_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, 
           "App: %d PAY:cust-idx-probe-forupdate (%d) (%d) (%d)\n", 
           xct_id, c_w, c_d, ppin->_c_id);

    W_DO(_pcustomer_man->cust_index_probe_forupdate(_pssm, prcust, 
                                                    ppin->_c_id, c_w, c_d));

    double c_balance, c_ytd_payment;
    int    c_payment_cnt;
    tpcc_customer_tuple acust;

    // retrieve customer
    prcust->get_value(3,  acust.C_FIRST, 17);
    prcust->get_value(4,  acust.C_MIDDLE, 3);
    prcust->get_value(5,  acust.C_LAST, 17);
    prcust->get_value(6,  acust.C_STREET_1, 21);
    prcust->get_value(7,  acust.C_STREET_2, 21);
    prcust->get_value(8,  acust.C_CITY, 21);
    prcust->get_value(9,  acust.C_STATE, 3);
    prcust->get_value(10, acust.C_ZIP, 10);
    prcust->get_value(11, acust.C_PHONE, 17);
    prcust->get_value(12, acust.C_SINCE);
    prcust->get_value(13, acust.C_CREDIT, 3);
    prcust->get_value(14, acust.C_CREDIT_LIM);
    prcust->get_value(15, acust.C_DISCOUNT);
    prcust->get_value(16, acust.C_BALANCE);
    prcust->get_value(17, acust.C_YTD_PAYMENT);
    prcust->get_value(18, acust.C_LAST_PAYMENT);
    prcust->get_value(19, acust.C_PAYMENT_CNT);
    prcust->get_value(20, acust.C_DATA_1, 251);
    prcust->get_value(21, acust.C_DATA_2, 251);

    // update customer fields
    acust.C_BALANCE -= ppin->_h_amount;
    acust.C_YTD_PAYMENT += ppin->_h_amount;
    acust.C_PAYMENT_CNT++;

    // if bad customer
    if (acust.C_CREDIT[0] == 'B' && acust.C_CREDIT[1] == 'C') { 
        /* 10% of customers */

        /* SELECT c_data
         * FROM customer 
         * WHERE c_id = :c_id AND c_w_id = :c_w_id AND c_d_id = :c_d_id
         * FOR UPDATE OF c_balance, c_ytd_payment, c_payment_cnt, c_data
         *
         * plan: index probe on "C_INDEX"
         */

        // update the data
        char c_new_data_1[251];
        char c_new_data_2[251];
        sprintf(c_new_data_1, "%d,%d,%d,%d,%d,%1.2f",
                ppin->_c_id, c_d, c_w, ppin->_home_d_id, 
                ppin->_home_wh_id, ppin->_h_amount);

        int len = strlen(c_new_data_1);
        strncat(c_new_data_1, acust.C_DATA_1, 250-len);
        strncpy(c_new_data_2, &acust.C_DATA_1[250-len], len);
        strncpy(c_new_data_2, acust.C_DATA_2, 250-len);

        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-update-tuple\n", xct_id);
        W_DO(_pcustomer_man->cust_update_tuple(_pssm, prcust, acust, c_new_data_1, c_new_data_2));
    }
    else { /* good customer */
        TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-update-tuple\n", xct_id);
        W_DO(_pcustomer_man->cust_update_tuple(_pssm, prcust, acust, NULL, NULL));
    }

    /* UPDATE district SET d_ytd = d_ytd + :h_amount
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * SELECT d_street_1, d_street_2, d_city, d_state, d_zip, d_name
     * FROM district
     * WHERE d_id = :d_id AND d_w_id = :w_id
     *
     * plan: index probe on "D_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:distr-upd-ytd (%d) (%d)\n", 
           xct_id, ppin->_home_wh_id, ppin->_home_d_id);
    W_DO(_pdistrict_man->dist_update_ytd(_pssm, prdist, ppin->_h_amount));

    tpcc_district_tuple adistr;
    prdist->get_value(2, adistr.D_NAME, 11);
    prdist->get_value(3, adistr.D_STREET_1, 21);
    prdist->get_value(4, adistr.D_STREET_2, 21);
    prdist->get_value(5, adistr.D_CITY, 21);
    prdist->get_value(6, adistr.D_STATE, 3);
    prdist->get_value(7, adistr.D_ZIP, 10);
    

    /* UPDATE warehouse SET w_ytd = wytd + :h_amount
     * WHERE w_id = :w_id
     *
     * SELECT w_name, w_street_1, w_street_2, w_city, w_state, w_zip
     * FROM warehouse
     * WHERE w_id = :w_id
     *
     * plan: index probe on "W_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:wh-update-ytd (%d)\n", 
           xct_id, ppin->_home_wh_id);
    W_DO(_pwarehouse_man->wh_update_ytd(_pssm, prwh, ppin->_h_amount));

    tpcc_warehouse_tuple awh;
    prwh->get_value(1, awh.W_NAME, 11);
    prwh->get_value(2, awh.W_STREET_1, 21);
    prwh->get_value(3, awh.W_STREET_2, 21);
    prwh->get_value(4, awh.W_CITY, 21);
    prwh->get_value(5, awh.W_STATE, 3);
    prwh->get_value(6, awh.W_ZIP, 10);


    /* INSERT INTO history
     * VALUES (:c_id, :c_d_id, :c_w_id, :d_id, :w_id, 
     *         :curr_tmstmp, :ih_amount, :h_data)
     */

    tpcc_history_tuple ahist;
    sprintf(ahist.H_DATA, "%s   %s", awh.W_NAME, adistr.D_NAME);
    ahist.H_DATE = time(NULL);

    prhist->set_value(0, ppin->_c_id);
    prhist->set_value(1, c_d);
    prhist->set_value(2, c_w);
    prhist->set_value(3, ppin->_home_d_id);
    prhist->set_value(4, ppin->_home_wh_id);
    prhist->set_value(5, ahist.H_DATE);
    prhist->set_value(6, ppin->_h_amount * 100.0);
    prhist->set_value(7, ahist.H_DATA);

    TRACE( TRACE_TRX_FLOW, "App: %d PAY:hist-add-tuple\n", xct_id);
    W_DO(_phistory_man->add_tuple(_pssm, prhist));


#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prwh->print_tuple();
    prdist->print_tuple();
    prcust->print_tuple();
    prhist->print_tuple();
#endif

    // give back the tuples
    _pwarehouse_man->give_tuple(prwh);
    _pdistrict_man->give_tuple(prdist);
    _pcustomer_man->give_tuple(prcust);
    _phistory_man->give_tuple(prhist);


    /* 4. commit */
    W_DO(_pssm->commit_xct());

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);

    return (RCOK);

} // EOF: PAYMENT



/******************************************************************** 
 *
 * TPC-C ORDER STATUS
 *
 * Input: w_id, d_id, c_id (use c_last if set to null), c_last
 *
 * @note: Read-Only trx
 *
 ********************************************************************/


w_rc_t ShoreTPCCEnv::xct_order_status(order_status_input_t* pstin, 
                                      const int xct_id, 
                                      trx_result_tuple_t& trt)
{
    // ensure a valid environment    
    assert (pstin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    register int w_id = pstin->_wh_id;
    register int d_id = pstin->_d_id;

    // order_status trx touches 3 tables: 
    // customer, order and orderline

//     customer_node_t* pcust_node = _pcustomer_man->get_tuple();
//     assert (pcust_node);
//     row_impl<customer_t>* prcust = pcust_node->_tuple;
    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

//     order_node_t* pord_node = _porder_man->get_tuple();
//     assert (pord_node);
//     row_impl<order_t>* prord = pord_node->_tuple;
    row_impl<order_t>* prord = _porder_man->get_tuple();
    assert (prord);

//     order_line_node_t* pol_node = _porder_line_man->get_tuple();
//     assert (pol_node);
//     row_impl<order_line_t>* prol = pol_node->_tuple;
    row_impl<order_line_t>* prol = _porder_line_man->get_tuple();
    assert (prol);


    trt.reset(UNSUBMITTED, xct_id);
    rep_row_t areprow(_pcustomer_man->ts());

    // allocate space for the biggest of the 3 table representations
    areprow.set(_customer_desc.maxsize()); 

    prcust->_rep = &areprow;
    prord->_rep = &areprow;
    prol->_rep = &areprow;


    rep_row_t lowrep(_pcustomer_man->ts());
    rep_row_t highrep(_pcustomer_man->ts());

    // allocate space for the biggest of the (customer) and (order)
    // table representations
    lowrep.set(_customer_desc.maxsize()); 
    highrep.set(_customer_desc.maxsize()); 

    /* 0. initiate transaction */
    W_DO(_pssm->begin_xct());

    /* 1a. select customer based on name */
    if (pstin->_c_id == 0) {
        /* SELECT  c_id, c_first
         * FROM customer
         * WHERE c_last = :c_last AND c_w_id = :w_id AND c_d_id = :d_id
         * ORDER BY c_first
         *
         * plan: index only scan on "C_NAME_INDEX"
         */

        assert (pstin->_c_select <= 60);
        assert (pstin->_c_last);

        index_scan_iter_impl<customer_t>* c_iter;
        TRACE( TRACE_TRX_FLOW, "App: %d ORDST:cust-get-iter-by-name-idx\n", xct_id);
        W_DO(_pcustomer_man->cust_get_iter_by_index(_pssm, c_iter, prcust, lowrep, highrep,
                                                    w_id, d_id, pstin->_c_last));

        int  c_id_list[17];
        int  count = 0;
        bool eof;

        W_DO(c_iter->next(_pssm, eof, *prcust));
        while (!eof) {
            prcust->get_value(0, c_id_list[count++]);            
            TRACE( TRACE_TRX_FLOW, "App: %d ORDST:iter-next\n", xct_id);
            W_DO(c_iter->next(_pssm, eof, *prcust));
        }
        delete c_iter;

        /* find the customer id in the middle of the list */
        pstin->_c_id = c_id_list[(count+1)/2-1];
    }
    assert (pstin->_c_id>0);


    /* 1. probe the customer */

    /* SELECT c_first, c_middle, c_last, c_balance
     * FROM customer
     * WHERE c_id = :c_id AND c_w_id = :w_id AND c_d_id = :d_id
     *
     * plan: index probe on "C_INDEX"
     */

    TRACE( TRACE_TRX_FLOW, 
           "App: %d ORDST:cust-idx-probe-forupdate (%d) (%d) (%d)\n", 
           xct_id, w_id, d_id, pstin->_c_id);
    W_DO(_pcustomer_man->cust_index_probe(_pssm, prcust, 
                                          pstin->_c_id, w_id, d_id));

    tpcc_customer_tuple acust;
    prcust->get_value(3,  acust.C_FIRST, 17);
    prcust->get_value(4,  acust.C_MIDDLE, 3);
    prcust->get_value(5,  acust.C_LAST, 17);
    prcust->get_value(16, acust.C_BALANCE);


    /* 2. retrieve the last order of this customer */

    /* SELECT o_id, o_entry_d, o_carrier_id
     * FROM orders
     * WHERE o_w_id = :w_id AND o_d_id = :d_id AND o_c_id = :o_c_id
     * ORDER BY o_id DESC
     *
     * plan: index scan on "C_CUST_INDEX"
     */
     
    index_scan_iter_impl<order_t>* o_iter;
    TRACE( TRACE_TRX_FLOW, "App: %d ORDST:get-order-iter-by-index\n", xct_id);
    W_DO(_porder_man->ord_get_iter_by_index(_pssm, o_iter, prord, lowrep, highrep,
                                            w_id, d_id, pstin->_c_id));

    tpcc_order_tuple aorder;
    bool eof;
    W_DO(o_iter->next(_pssm, eof, *prord));
    while (!eof) {
        prord->get_value(0, aorder.O_ID);
        prord->get_value(4, aorder.O_ENTRY_D);
        prord->get_value(5, aorder.O_CARRIER_ID);
        prord->get_value(6, aorder.O_OL_CNT);

        //        rord.print_tuple();

        W_DO(o_iter->next(_pssm, eof, *prord));
    }
    delete o_iter;
     
    // we should have retrieved a valid id and ol_cnt for the order               
    assert (aorder.O_ID);
    assert (aorder.O_OL_CNT);
     
    /* 3. retrieve all the orderlines that correspond to the last order */
     
    /* SELECT ol_i_id, ol_supply_w_id, ol_quantity, ol_amount, ol_delivery_d 
     * FROM order_line 
     * WHERE ol_w_id = :H00003 AND ol_d_id = :H00004 AND ol_o_id = :H00016 
     *
     * plan: index scan on "OL_INDEX"
     */

    index_scan_iter_impl<order_line_t>* ol_iter;
    TRACE( TRACE_TRX_FLOW, "App: %d ORDST:get-iter-by-index\n", xct_id);
    W_DO(_porder_line_man->ol_get_iter_by_index(_pssm, ol_iter, prol,
                                                lowrep, highrep,
                                                w_id, d_id, aorder.O_ID));

    tpcc_orderline_tuple* porderlines = new tpcc_orderline_tuple[aorder.O_OL_CNT];
    int i=0;

    W_DO(ol_iter->next(_pssm, eof, *prol));
    while (!eof) {
	prol->get_value(4, porderlines[i].OL_I_ID);
	prol->get_value(5, porderlines[i].OL_SUPPLY_W_ID);
	prol->get_value(6, porderlines[i].OL_DELIVERY_D);
	prol->get_value(7, porderlines[i].OL_QUANTITY);
	prol->get_value(8, porderlines[i].OL_AMOUNT);
	i++;

	W_DO(ol_iter->next(_pssm, eof, *prol));
    }
    delete ol_iter;
    delete [] porderlines;


#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    rcust.print_tuple();
    rord.print_tuple();
    rordline.print_tuple();
#endif

    _pcustomer_man->give_tuple(prcust);
    _porder_man->give_tuple(prord);  
    _porder_line_man->give_tuple(prol);


    /* 4. commit */
    W_DO(_pssm->commit_xct());

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);

    return (RCOK);

}; // EOF: ORDER-STATUS



/******************************************************************** 
 *
 * TPC-C DELIVERY
 *
 * Input data: w_id, carrier_id
 *
 * @note: Delivers one new_order (undelivered order) from each district
 *
 ********************************************************************/


w_rc_t ShoreTPCCEnv::xct_delivery(delivery_input_t* pdin, 
                                  const int xct_id, 
                                  trx_result_tuple_t& trt)
{
    // ensure a valid environment    
    assert (pdin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    register int w_id       = pdin->_wh_id;
    register int carrier_id = pdin->_carrier_id; 
    time_t ts_start = time(NULL);

    // delivery trx touches 4 tables: 
    // new_order, order, orderline, and customer
//     customer_node_t* pcust_node = _pcustomer_man->get_tuple();
//     assert (pcust_node);
//     row_impl<customer_t>* prcust = pcust_node->_tuple;
    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

//     new_order_node_t* pno_node = _pnew_order_man->get_tuple();
//     assert (pno_node);
//     row_impl<new_order_t>* prno = pno_node->_tuple;
    row_impl<new_order_t>* prno = _pnew_order_man->get_tuple();
    assert (prno);

//     order_node_t* pord_node = _porder_man->get_tuple();
//     assert (pord_node);
//     row_impl<order_t>* prord = pord_node->_tuple;
    row_impl<order_t>* prord = _porder_man->get_tuple();
    assert (prord);

//     order_line_node_t* pol_node = _porder_line_man->get_tuple();
//     assert (pol_node);
//     row_impl<order_line_t>* prol = pol_node->_tuple;
    row_impl<order_line_t>* prol = _porder_line_man->get_tuple();
    assert (prol);
    

    trt.reset(UNSUBMITTED, xct_id);
    rep_row_t areprow(_pcustomer_man->ts());

    // allocate space for the biggest of the 4 table representations
    areprow.set(_customer_desc.maxsize()); 

    prno->_rep = &areprow;
    prord->_rep = &areprow;
    prol->_rep = &areprow;
    prcust->_rep = &areprow;

    /* 0. initiate transaction */
    W_DO(_pssm->begin_xct());

    rep_row_t lowrep(_porder_line_man->ts());
    rep_row_t highrep(_porder_line_man->ts());
    rep_row_t sortrep(_porder_line_man->ts());
    // allocate space for the biggest of the (new_order) and (orderline)
    // table representations
    lowrep.set(_order_line_desc.maxsize()); 
    highrep.set(_order_line_desc.maxsize()); 
    sortrep.set(_order_line_desc.maxsize()); 


    /* process each district separately */
    for (int d_id = 1; d_id <= DISTRICTS_PER_WAREHOUSE; d_id ++) {

        /* 1. Get the new_order of the district, with the min value */

	/* SELECT MIN(no_o_id) INTO :no_o_id:no_o_id_i
	 * FROM new_order
	 * WHERE no_d_id = :d_id AND no_w_id = :w_id
	 *
	 * plan: index scan on "NO_INDEX"
	 */
        TRACE( TRACE_TRX_FLOW, "App: %d DEL:get-new-order-iter-by-index (%d) (%d)\n", 
               xct_id, w_id, d_id);
    
        index_scan_iter_impl<new_order_t>* no_iter;
	W_DO(_pnew_order_man->no_get_iter_by_index(_pssm, no_iter, prno, 
                                                   lowrep, highrep,
                                                   w_id, d_id));
	bool eof;
        // iterate over all new_orders and load their no_o_ids to the sort buffer
	W_DO(no_iter->next(_pssm, eof, *prno));
	if(eof) continue; // skip this district

	int no_o_id;
	prno->get_value(0, no_o_id);
	delete no_iter;
        assert (no_o_id);


        /* 2. Delete the retrieved new order from the new_orders */        

	/* DELETE FROM new_order
	 * WHERE no_w_id = :w_id AND no_d_id = :d_id AND no_o_id = :no_o_id
	 *
	 * plan: index scan on "NO_INDEX"
	 */

        TRACE( TRACE_TRX_FLOW, "App: %d DEL:delete-new-order-by-index (%d) (%d) (%d)\n", 
               xct_id, w_id, d_id, no_o_id);

	W_DO(_pnew_order_man->no_delete_by_index(_pssm, prno, w_id, d_id, no_o_id));


        /* 3a. Update the carrier for the delivered order (in the orders table) */
        /* 3b. Get the customer id of the updated order */

	/* UPDATE orders SET o_carrier_id = :o_carrier_id
         * SELECT o_c_id INTO :o_c_id FROM orders
	 * WHERE o_id = :no_o_id AND o_w_id = :w_id AND o_d_id = :d_id;
	 *
	 * plan: index probe on "O_INDEX"
	 */

        TRACE( TRACE_TRX_FLOW, "App: %d DEL:idx-probe (%d) (%d) (%d)\n", 
               xct_id, w_id, d_id, no_o_id);

	prord->set_value(0, no_o_id);
	prord->set_value(2, d_id);
	prord->set_value(3, w_id);
	W_DO(_porder_man->ord_update_carrier_by_index(_pssm, prord, carrier_id));

	int  c_id;
	prord->get_value(1, c_id);

        
        /* 4a. Calculate the total amount of the orders from orderlines */
        /* 4b. Update all the orderlines with the current timestamp */
           
	/* SELECT SUM(ol_amount) INTO :total_amount FROM order_line
         * UPDATE ORDER_LINE SET ol_delivery_d = :curr_tmstmp
	 * WHERE ol_w_id = :w_id AND ol_d_id = :no_d_id AND ol_o_id = :no_o_id;
	 *
	 * plan: index scan on "OL_INDEX"
	 */


        TRACE( TRACE_TRX_FLOW, 
               "App: %d DEL:get-orderline-iter-by-index (%d) (%d) (%d)\n", 
               xct_id, w_id, d_id, no_o_id);

	int total_amount = 0;
        index_scan_iter_impl<order_line_t>* ol_iter;
	W_DO(_porder_line_man->ol_get_iter_by_index(_pssm, ol_iter, prol, 
                                                    lowrep, highrep,
                                                    w_id, d_id, no_o_id));

        // iterate over all the orderlines for the particular order
	W_DO(ol_iter->next(_pssm, eof, *prol));
	while (!eof) {
	    int current_amount;
	    prol->get_value(8, current_amount);
	    total_amount += current_amount;
            prol->set_value(6, ts_start);
            W_DO(_porder_line_man->update_tuple(_pssm, prol));
	    W_DO(ol_iter->next(_pssm, eof, *prol));
	}
	delete ol_iter;


        /* 5. Update balance of the customer of the order */

	/* UPDATE customer
	 * SET c_balance = c_balance + :total_amount, c_delivery_cnt = c_delivery_cnt + 1
	 * WHERE c_id = :c_id AND c_w_id = :w_id AND c_d_id = :no_d_id;
	 *
	 * plan: index probe on "C_INDEX"
	 */

        TRACE( TRACE_TRX_FLOW, "App: %d DEL:idx-probe (%d) (%d) (%d)\n", 
               xct_id, w_id, d_id, c_id);

	W_DO(_pcustomer_man->cust_index_probe(_pssm, prcust, 
                                              c_id, w_id, d_id));

	double   balance;
	prcust->get_value(16, balance);
	prcust->set_value(16, balance+total_amount);
	W_DO(_pcustomer_man->update_tuple(_pssm, prcust));
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prno->print_tuple();
    prord->print_tuple();
    prordline->print_tuple();
    prcust->print_tuple();
#endif

    _pcustomer_man->give_tuple(prcust);
    _pnew_order_man->give_tuple(prno);
    _porder_man->give_tuple(prord);
    _porder_line_man->give_tuple(prol);

    /* 4. commit */
    W_DO(_pssm->commit_xct());

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);

    return (RCOK);
}




/******************************************************************** 
 *
 * TPC-C STOCK LEVEL
 *
 * Input data: w_id, d_id, threshold
 *
 * @note: Read-only transaction
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::xct_stock_level(stock_level_input_t* pslin, 
                                     const int xct_id, 
                                     trx_result_tuple_t& trt)
{
    // ensure a valid environment    
    assert (pslin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // stock level trx touches 4 tables: 
    // district, orderline, and stock
//     district_node_t* pdistr_node = _pdistrict_man->get_tuple();
//     assert (pdistr_node);
//     row_impl<district_t>* prdist = pdistr_node->_tuple;
    row_impl<district_t>* prdist = _pdistrict_man->get_tuple();
    assert (prdist);

//     stock_node_t* pst_node = _pstock_man->get_tuple();
//     assert (pst_node);
//     row_impl<stock_t>* prst = pst_node->_tuple;
    row_impl<stock_t>* prst = _pstock_man->get_tuple();
    assert (prst);

//     order_line_node_t* pol_node = _porder_line_man->get_tuple();
//     assert (pol_node);
//     row_impl<order_line_t>* prol = pol_node->_tuple;
    row_impl<order_line_t>* prol = _porder_line_man->get_tuple();
    assert (prol);

    trt.reset(UNSUBMITTED, xct_id);
    rep_row_t areprow(_pdistrict_man->ts());

    // allocate space for the biggest of the 3 table representations
    areprow.set(_district_desc.maxsize()); 

    prdist->_rep = &areprow;
    prol->_rep = &areprow;
    prst->_rep = &areprow;

    /* 0. initiate transaction */
    W_DO(_pssm->begin_xct());
    
    /* 1. get next_o_id from the district */

    /* SELECT d_next_o_id INTO :o_id
     * FROM district
     * WHERE d_w_id = :w_id AND d_id = :d_id
     *
     * (index scan on D_INDEX)
     */

    TRACE( TRACE_TRX_FLOW, "App: %d STO:idx-probe (%d) (%d)\n", 
           xct_id, pslin->_d_id, pslin->_wh_id);

    W_DO(_pdistrict_man->dist_index_probe(_pssm, prdist, 
                                          pslin->_d_id, pslin->_wh_id));

    int next_o_id = 0;
    prdist->get_value(10, next_o_id);


    /*
     *   SELECT COUNT(DISTRICT(s_i_id)) INTO :stock_count
     *   FROM order_line, stock
     *   WHERE ol_w_id = :w_id AND ol_d_id = :d_id
     *       AND ol_o_id < :o_id AND ol_o_id >= :o_id-20
     *       AND s_w_id = :w_id AND s_i_id = ol_i_id
     *       AND s_quantity < :threshold;
     *
     *  Plan: 1. index scan on OL_INDEX 
     *        2. sort ol tuples in the order of i_id from 1
     *        3. index scan on S_INDEX
     *        4. fetch stock with sargable on quantity from 3
     *        5. nljoin on 2 and 4
     *        6. unique on 5
     *        7. group by on 6
     */

    /* 2a. Index scan on order_line table. */

    TRACE( TRACE_TRX_FLOW, "App: %d STO:get-iter-by-index (%d) (%d) (%d) (%d)\n", 
           xct_id, pslin->_wh_id, pslin->_d_id, next_o_id-20, next_o_id);
   
    rep_row_t lowrep(_porder_line_man->ts());
    rep_row_t highrep(_porder_line_man->ts());
    rep_row_t sortrep(_porder_line_man->ts());
    // allocate space for the biggest of the (new_order) and (orderline)
    // table representations
    lowrep.set(_order_line_desc.maxsize()); 
    highrep.set(_order_line_desc.maxsize()); 
    sortrep.set(_order_line_desc.maxsize()); 
    

    index_scan_iter_impl<order_line_t>* ol_iter;
    W_DO(_porder_line_man->ol_get_iter_by_index(_pssm, ol_iter, prol,
                                                lowrep, highrep,
                                                pslin->_wh_id, pslin->_d_id,
                                                next_o_id-20, next_o_id));

    sort_buffer_t ol_list(4);
    ol_list.setup(0, SQL_INT);  /* OL_I_ID */
    ol_list.setup(1, SQL_INT);  /* OL_W_ID */
    ol_list.setup(2, SQL_INT);  /* OL_D_ID */
    ol_list.setup(3, SQL_INT);  /* OL_O_ID */
    sort_man_impl ol_sorter(&ol_list, &sortrep, 2);
    row_impl<sort_buffer_t> rsb(&ol_list);

    // iterate over all selected orderlines and add them to the sorted buffer
    bool eof;
    W_DO(ol_iter->next(_pssm, eof, *prol));
    while (!eof) {
	/* put the value into the sorted buffer */
	int temp_oid, temp_iid;
	int temp_wid, temp_did;        

	prol->get_value(4, temp_iid);
	prol->get_value(0, temp_oid);
	prol->get_value(2, temp_wid);
	prol->get_value(1, temp_did);

	rsb.set_value(0, temp_iid);
	rsb.set_value(1, temp_wid);
	rsb.set_value(2, temp_did);
	rsb.set_value(3, temp_oid);

	ol_sorter.add_tuple(rsb);
  
	W_DO(ol_iter->next(_pssm, eof, *prol));
    }
    delete ol_iter;
    assert (ol_sorter.count());

    /* 2b. Sort orderline tuples on i_id */
    sort_iter_impl ol_list_sort_iter(_pssm, &ol_list, &ol_sorter);
    int last_i_id = -1;
    int count = 0;

    /* 2c. Nested loop join order_line with stock */
    W_DO(ol_list_sort_iter.next(_pssm, eof, rsb));
    while (!eof) {

	/* use the index to find the corresponding stock tuple */
	int i_id;
	int w_id;

	rsb.get_value(0, i_id);
	rsb.get_value(1, w_id);

        TRACE( TRACE_TRX_FLOW, "App: %d STO:idx-probe (%d) (%d)\n", 
               xct_id, i_id, w_id);

	W_DO(_pstock_man->st_index_probe(_pssm, prst, i_id, w_id));

        // check if stock quantity below threshold 
	int quantity;
	prst->get_value(3, quantity);

	if (quantity < pslin->_threshold) {
	    /* Do join on the two tuples */

	    /* the work is to count the number of unique item id. We keep
	     * two pieces of information here: the last item id and the
	     * current count.  This is enough because the item id is in
	     * increasing order.
	     */
	    if (last_i_id != i_id) {
		last_i_id = i_id;
		count++;
	    }
	}

	W_DO(ol_list_sort_iter.next(_pssm, eof, rsb));
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    rdist.print_tuple();
    rordline.print_tuple();
    rstock.print_tuple();
#endif

    _pdistrict_man->give_tuple(prdist);
    _pstock_man->give_tuple(prst);
    _porder_line_man->give_tuple(prol);

  
    /* 3. commit */
    W_DO(_pssm->commit_xct());

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);

    return (RCOK);
}



EXIT_NAMESPACE(tpcc);
