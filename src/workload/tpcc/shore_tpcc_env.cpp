/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_env.cpp
 *
 *  @brief:  Declaration of the Shore TPC-C environment (database)
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#include "workload/tpcc/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"

#include "stages/tpcc/common/tpcc_random.h"


using namespace shore;


ENTER_NAMESPACE(tpcc);



/** Exported functions */


/******************************************************************** 
 *
 *  @fn:    print_trx_stats
 *
 *  @brief: Prints trx statistics
 *
 ********************************************************************/

void tpcc_stats_t::print_trx_stats() const
{   
    TRACE( TRACE_STATISTICS, "=====================================\n");
    TRACE( TRACE_STATISTICS, "TPC-C Database transaction statistics\n");
    TRACE( TRACE_STATISTICS, "NEW-ORDER\n");
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _no_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _no_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_no_att-_no_com));
    TRACE( TRACE_STATISTICS, "PAYMENT\n");
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _pay_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _pay_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_pay_att-_pay_com));
    TRACE( TRACE_STATISTICS, "ORDER-STATUS\n");
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _ord_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _ord_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_ord_att-_ord_com));
    TRACE( TRACE_STATISTICS, "DELIVERY\n");
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _del_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _del_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_del_att-_del_com));
    TRACE( TRACE_STATISTICS, "STOCK-LEVEL\n");
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _sto_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _sto_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_sto_att-_sto_com));
    TRACE( TRACE_STATISTICS, "OTHER\n");
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _other_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _other_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_other_att-_other_com));
    TRACE( TRACE_STATISTICS, "=====================================\n");
}



/******************************************************************** 
 *
 * ShoreTPCCEnv functions
 *
 ********************************************************************/ 

const int ShoreTPCCEnv::load_schema()
{
    // get the sysname type from the configuration
    _sysname = _dev_opts[SHORE_DB_OPTIONS[4][0]];
    TRACE( TRACE_ALWAYS, "Sysname (%s)\n", _sysname.c_str());

    // create the schema
    _pwarehouse_desc  = new warehouse_t(_sysname);
    _pdistrict_desc   = new district_t(_sysname);
    _pcustomer_desc   = new customer_t(_sysname);
    _phistory_desc    = new history_t(_sysname);
    _pnew_order_desc  = new new_order_t(_sysname);
    _porder_desc      = new order_t(_sysname);
    _porder_line_desc = new order_line_t(_sysname);
    _pitem_desc       = new item_t(_sysname);
    _pstock_desc      = new stock_t(_sysname);


    // initiate the table managers
    _pwarehouse_man  = new warehouse_man_impl(_pwarehouse_desc.get());
    _pdistrict_man   = new district_man_impl(_pdistrict_desc.get());
    _pstock_man      = new stock_man_impl(_pstock_desc.get());
    _porder_line_man = new order_line_man_impl(_porder_line_desc.get());
    _pcustomer_man   = new customer_man_impl(_pcustomer_desc.get());
    _phistory_man    = new history_man_impl(_phistory_desc.get());
    _porder_man      = new order_man_impl(_porder_desc.get());
    _pnew_order_man  = new new_order_man_impl(_pnew_order_desc.get());
    _pitem_man       = new item_man_impl(_pitem_desc.get());

    // XXX: !!! Warning !!!
    //
    // The two lists should have the description and the manager
    // of the same table in the same position
    //

    //// add the table managers to a list

    // (ip) Adding them in descending file order, so that the large
    //      files to be loaded at the begining. Expection is the
    //      WH and DISTR which are always the first two.
    _table_man_list.push_back(_pwarehouse_man);
    _table_man_list.push_back(_pdistrict_man);
    _table_man_list.push_back(_pstock_man);
    _table_man_list.push_back(_porder_line_man);
    _table_man_list.push_back(_pcustomer_man);
    _table_man_list.push_back(_phistory_man);
    _table_man_list.push_back(_porder_man);
    _table_man_list.push_back(_pnew_order_man);
    _table_man_list.push_back(_pitem_man);

    assert (_table_man_list.size() == SHORE_TPCC_TABLES);
        
    //// add the table descriptions to a list
    _table_desc_list.push_back(_pwarehouse_desc.get());
    _table_desc_list.push_back(_pdistrict_desc.get());
    _table_desc_list.push_back(_pstock_desc.get());
    _table_desc_list.push_back(_porder_line_desc.get());
    _table_desc_list.push_back(_pcustomer_desc.get());
    _table_desc_list.push_back(_phistory_desc.get());
    _table_desc_list.push_back(_porder_desc.get());
    _table_desc_list.push_back(_pnew_order_desc.get());
    _table_desc_list.push_back(_pitem_desc.get());

    assert (_table_desc_list.size() == SHORE_TPCC_TABLES);
        
    return (0);
}


/******************************************************************** 
 *
 *  @fn:    info()
 *
 *  @brief: Prints information about the current db instance status
 *
 ********************************************************************/

const int ShoreTPCCEnv::info()
{
    TRACE( TRACE_ALWAYS, "SF      = (%d)\n", _scaling_factor);
    TRACE( TRACE_ALWAYS, "Workers = (%d)\n", _worker_cnt);
    return (0);
}


/******************************************************************** 
 *
 *  @fn:    start()
 *
 *  @brief: Starts the tpcc env
 *
 ********************************************************************/

const int ShoreTPCCEnv::start()
{
    upd_sf();
    upd_worker_cnt();

    assert (_workers.empty());

    TRACE( TRACE_ALWAYS, "Starting (%s)\n", _sysname.c_str());      
    info();

    // read from env params the loopcnt
    int lc = envVar::instance()->getVarInt("db-queueloops",0);    

    WorkerPtr aworker;
    for (int i=0; i<_worker_cnt; i++) {
        aworker = new Worker(this,this,c_str("work-%d", i));
        _workers.push_back(aworker);
        aworker->init(lc);
        aworker->start();
        aworker->fork();
    }
    return (0);
}


/******************************************************************** 
 *
 *  @fn:    stop()
 *
 *  @brief: Stops the tpcc env
 *
 ********************************************************************/

const int ShoreTPCCEnv::stop()
{
    TRACE( TRACE_ALWAYS, "Stopping (%s)\n", _sysname.c_str());
    info();

    int i=0;
    for (WorkerIt it = _workers.begin(); it != _workers.end(); ++it) {
        i++;
        TRACE( TRACE_DEBUG, "Stopping worker (%d)\n", i);
        if (*it) {
            (*it)->stop();
            (*it)->join();
            delete (*it);
        }
    }
    _workers.clear();
    return (0);
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

const int ShoreTPCCEnv::upd_sf()
{
    // update whs
    int tmp_sf = envVar::instance()->getSysVarInt("sf");
    assert (tmp_sf);
    set_sf(tmp_sf);
    set_qf(tmp_sf);
    //print_sf();
    return (_scaling_factor);
}


void ShoreTPCCEnv::print_sf(void)
{
    TRACE( TRACE_ALWAYS, "*** ShoreTPCCEnv ***\n");
    TRACE( TRACE_ALWAYS, "Scaling Factor = (%d)\n", get_sf());
    TRACE( TRACE_ALWAYS, "Queried Factor = (%d)\n", get_qf());
}


const int ShoreTPCCEnv::upd_worker_cnt()
{
    // update worker thread cnt
    int workers = envVar::instance()->getVarInt("db-workers",0);
    assert (workers);
    _worker_cnt = workers;
    return (_worker_cnt);
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
    //const char* loaddatadir = _dev_opts[SHORE_DB_OPTIONS[3][0]].c_str();
    string loaddatadir = envVar::instance()->getSysVar("loadatadir");
    int cnt = 0;

    TRACE( TRACE_DEBUG, "Loaddir (%s)\n", loaddatadir.c_str());

    guard<table_loading_smt_t> loaders[SHORE_TPCC_TABLES];

    // manually create the loading threads
    loaders[0] = new wh_loader_t(c_str("ld-WH"), _pssm, _pwarehouse_man,
                                 _pwarehouse_desc.get(), _scaling_factor, loaddatadir.c_str());
    loaders[1] = new dist_loader_t(c_str("ld-DIST"), _pssm, _pdistrict_man,
                                   _pdistrict_desc.get(), _scaling_factor, loaddatadir.c_str());
    loaders[2] = new st_loader_t(c_str("ld-ST"), _pssm, _pstock_man,
                                 _pstock_desc.get(), _scaling_factor, loaddatadir.c_str());
    loaders[3] = new ol_loader_t(c_str("ld-OL"), _pssm, _porder_line_man,
                                 _porder_line_desc.get(), _scaling_factor, loaddatadir.c_str());
    loaders[4] = new cust_loader_t(c_str("ld-CUST"), _pssm, _pcustomer_man,
                                   _pcustomer_desc.get(), _scaling_factor, loaddatadir.c_str());
    loaders[5] = new hist_loader_t(c_str("ld-HIST"), _pssm, _phistory_man,
                                   _phistory_desc.get(), _scaling_factor, loaddatadir.c_str());
    loaders[6] = new ord_loader_t(c_str("ld-ORD"), _pssm, _porder_man,
                                  _porder_desc.get(), _scaling_factor, loaddatadir.c_str());
    loaders[7] = new no_loader_t(c_str("ld-NO"), _pssm, _pnew_order_man,
                                 _pnew_order_desc.get(), _scaling_factor, loaddatadir.c_str());
    loaders[8] = new it_loader_t(c_str("ld-IT"), _pssm, _pitem_man,
                                 _pitem_desc.get(), _scaling_factor, loaddatadir.c_str());

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
//                                                    loaddatadir.c_str());
//             cnt++;
//        }

#if 1
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
                                   _pwarehouse_man, _pwarehouse_desc.get());
    checkers[1] = new dist_checker_t(c_str("chk-DIST"), _pssm, 
                                     _pdistrict_man, _pdistrict_desc.get());
    checkers[2] = new st_checker_t(c_str("chk-ST"), _pssm, 
                                   _pstock_man, _pstock_desc.get());
    checkers[3] = new ol_checker_t(c_str("chk-OL"), _pssm, 
                                   _porder_line_man, _porder_line_desc.get());
    checkers[4] = new cust_checker_t(c_str("chk-CUST"), _pssm, 
                                     _pcustomer_man, _pcustomer_desc.get());
    checkers[5] = new hist_checker_t(c_str("chk-HIST"), _pssm, 
                                     _phistory_man, _phistory_desc.get());
    checkers[6] = new ord_checker_t(c_str("chk-ORD"), _pssm, 
                                    _porder_man, _porder_desc.get());
    checkers[7] = new no_checker_t(c_str("chk-NO"), _pssm, 
                                   _pnew_order_man, _pnew_order_desc.get());
    checkers[8] = new it_checker_t(c_str("chk-IT"), _pssm, 
                                   _pitem_man, _pitem_desc.get());


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
 *  @fn:    dump
 *
 *  @brief: Print information for all the tables in the environment
 *
 ********************************************************************/

const int ShoreTPCCEnv::dump()
{
    table_man_t* ptable_man = NULL;
    for(table_man_list_iter table_man_iter = _table_man_list.begin(); 
        table_man_iter != _table_man_list.end(); table_man_iter++)
        {
            ptable_man = *table_man_iter;
            ptable_man->print_table(this->_pssm);
        }
    return (0);
}


const int ShoreTPCCEnv::conf()
{
    // reread the params
    ShoreEnv::conf();
    upd_sf();
    upd_worker_cnt();
    return (0);
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

const int ShoreTPCCEnv::post_init() 
{
    conf();
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
		TRACE(TRACE_ALWAYS, " -> Reached EOF. Search complete (%d)\n", count);
		break;
	    }

	    // figure out how big the old record is
	    int hsize = handle->hdr_size();
	    int bsize = handle->body_size();
	    if (bsize == psize) {
		TRACE(TRACE_ALWAYS, " -> Found padded WH record. Stopping search (%d)\n", count);
		break;
	    }
	    else if (bsize > psize) {
		// too big... shrink it down to save on logging
		handle->truncate_rec(bsize - psize);
                fprintf(stderr, "+");
	    }
	    else {
		// copy and pad the record (and mark the old one for deletion)
		rid_t new_rid;
		vec_t hvec(handle->hdr(), hsize);
		vec_t dvec(handle->body(), bsize);
		vec_t pvec(padding, PADDED_SIZE-bsize);
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
                fprintf(stderr, ".");
	    }
	    
	    // next!
	    count++;
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


EXIT_NAMESPACE(tpcc);
