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

/** @file:   shore_tpcc_env.cpp
 *
 *  @brief:  Declaration of the Shore TPC-C environment (database)
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#include "workload/tpcc/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"

#include "workload/tpcc/common/tpcc_random.h"


using namespace shore;


ENTER_NAMESPACE(tpcc);



/******************************************************************** 
 *
 * TPC-C Parallel Loading
 *
 ********************************************************************/


struct ShoreTPCCEnv::checkpointer_t : public thread_t 
{
    ShoreTPCCEnv* _env;
    checkpointer_t(ShoreTPCCEnv* env) 
        : thread_t("TPC-C Load Checkpointer"), _env(env) { }
    virtual void work();
};

class ShoreTPCCEnv::table_builder_t : public thread_t 
{
    ShoreTPCCEnv* _env;
    long _start;
    long _count;
    int* _cids;
public:
    table_builder_t(ShoreTPCCEnv* env, long start, long count, int* cids)
	: thread_t("TPC-C Loader"), _env(env), _start(start), _count(count), _cids(cids) { }
    virtual void work();
};

struct ShoreTPCCEnv::table_creator_t : public thread_t 
{
    ShoreTPCCEnv* _env;
    int _sf;
    table_creator_t(ShoreTPCCEnv* env, int sf)
	: thread_t("TPC-C Table Creator"), _env(env), _sf(sf) { }
    virtual void work();
};



void ShoreTPCCEnv::checkpointer_t::work() 
{
    bool volatile* loaded = &_env->_loaded;
    while(!*loaded) {
	_env->set_measure(MST_MEASURE);
	for(int i=0; i < 60 && ! *loaded; i++) 
	    ::sleep(1);
	//	_env->set_measure(MST_PAUSE);
	
        TRACE( TRACE_ALWAYS, "db checkpoint - start\n");
        _env->checkpoint();
        TRACE( TRACE_ALWAYS, "db checkpoint - end\n");
    }
}


void ShoreTPCCEnv::table_creator_t::work() 
{
    /* create the tables */
    W_COERCE(_env->db()->begin_xct());
    W_COERCE(_env->_pwarehouse_desc->create_table(_env->db()));
    W_COERCE(_env->_pdistrict_desc->create_table(_env->db()));
    W_COERCE(_env->_pcustomer_desc->create_table(_env->db()));
    W_COERCE(_env->_phistory_desc->create_table(_env->db()));
    W_COERCE(_env->_pnew_order_desc->create_table(_env->db()));
    W_COERCE(_env->_porder_desc->create_table(_env->db()));
    W_COERCE(_env->_porder_line_desc->create_table(_env->db()));
    W_COERCE(_env->_pitem_desc->create_table(_env->db()));
    W_COERCE(_env->_pstock_desc->create_table(_env->db()));
    W_COERCE(_env->db()->commit_xct());

    /* do the first transaction */
    trx_result_tuple_t out;
    populate_baseline_input_t in = {_sf};
    W_COERCE(_env->db()->begin_xct());
    W_COERCE(_env->xct_populate_baseline(0, out, in));

    W_COERCE(_env->db()->begin_xct());
    W_COERCE(_env->_post_init_impl());
    W_COERCE(_env->db()->commit_xct());
    
#if 0
    /*
      create 10k accounts in each partition to buffer workers from each other
     */
    for(long i=-1; i < _pcount; i++) {
	long a_id = i*_psize;
	populate_db_input_t in(_sf, a_id);
	trx_result_tuple_t out;
	fprintf(stderr, "Populating %d a_ids starting with %d\n", ACCOUNTS_CREATED_PER_POP_XCT, a_id);
	W_COERCE(_env->db()->begin_xct());
	W_COERCE(_env->xct_populate_db(&in, a_id, out));
    }
#endif
}
static void gen_cid_array(int* cid_array) {
    for(int i=0; i < ORDERS_PER_DIST; i++)
	cid_array[i] = i+1;
    for(int i=0; i < ORDERS_PER_DIST; i++) {
	std::swap(cid_array[i], cid_array[i+URand(0,ORDERS_PER_DIST-i-1)]);
                  //sthread_t::randn(ORDERS_PER_DIST-i)]);
    }
}

static unsigned long units_completed = 0;
void ShoreTPCCEnv::table_builder_t::work() 
{
    w_rc_t e = RCOK;

    /* There are up to three parts of my job: the parts at the
       beginning and end which may overlap with other workers (need to
       coordinate cids used for orders) and those which I fully own.

       When coordination is needed we use the passed-in cid
       permutation array; otherwise we generate our own.
    */
    int cid_array[ORDERS_PER_DIST];
    gen_cid_array(cid_array);

    int last_wh = 1;
    for(int i=0 ; i < _count; i++) {
	while(_env->get_measure() != MST_MEASURE)
	    usleep(10000);
	
	long tid = _start + i;
	int my_dist = tid/UNIT_PER_DIST;
	int start_dist = (tid + UNIT_PER_DIST - 1)/UNIT_PER_DIST;
	int end_dist = (tid + UNIT_PER_DIST)/UNIT_PER_DIST;
	bool overlap = (start_dist*UNIT_PER_DIST < _start) || (end_dist*UNIT_PER_DIST >= _start+_count);
	int *cids = overlap? _cids : cid_array+0;
	populate_one_unit_input_t in = {tid, cids};
	trx_result_tuple_t out;
    retry:
	W_COERCE(_env->db()->begin_xct());
	e = _env->xct_populate_one_unit(tid, out, in);
	if(e.is_error()) {
	    W_COERCE(_env->db()->abort_xct());
	    if(e.err_num() == smlevel_0::eDEADLOCK)
		goto retry;
	    
	    stringstream os;
	    os << e << ends;
	    string str = os.str();
	    fprintf(stderr, "Eek! Unable to populate db for index %d due to:\n%s\n",
		    i, str.c_str());
	}
	long nval = atomic_inc_64_nv(&units_completed);
	if(nval % UNIT_PER_WH == 0)
	    fprintf(stderr, ".\n");
    }
    fprintf(stderr, "Finished loading units %d .. %d \n", _start, _start+_count);

}




/******************************************************************** 
 *
 * ShoreTPCCEnv functions
 *
 ********************************************************************/ 

const int ShoreTPCCEnv::load_schema()
{
    // get the sysname type from the configuration
    _sysname = envVar::instance()->getSysname();
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
 *  @fn:    statistics
 *
 *  @brief: Prints statistics for TPC-C 
 *
 ********************************************************************/

const int ShoreTPCCEnv::statistics() 
{
    // read the current trx statistics
    CRITICAL_SECTION(cs, _statmap_mutex);
    ShoreTPCCTrxStats rval;
    rval -= rval; // dirty hack to set all zeros
    for (statmap_t::iterator it=_statmap.begin(); it != _statmap.end(); ++it) 
	rval += *it->second;

    TRACE( TRACE_STATISTICS, "NewOrder. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.new_order,
           rval.failed.new_order,
           rval.deadlocked.new_order);

    TRACE( TRACE_STATISTICS, "Payment. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.payment,
           rval.failed.payment,
           rval.deadlocked.payment);

    TRACE( TRACE_STATISTICS, "OrderStatus. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.order_status,
           rval.failed.order_status,
           rval.deadlocked.order_status);

    TRACE( TRACE_STATISTICS, "Delivery. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.delivery,
           rval.failed.delivery,
           rval.deadlocked.delivery);

    TRACE( TRACE_STATISTICS, "StockLevel. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.stock_level,
           rval.failed.stock_level,
           rval.deadlocked.stock_level);

    TRACE( TRACE_STATISTICS, "MBenchWh. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.mbench_wh,
           rval.failed.mbench_wh,
           rval.deadlocked.mbench_wh);

    TRACE( TRACE_STATISTICS, "MBenchCust. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.mbench_cust,
           rval.failed.mbench_cust,
           rval.deadlocked.mbench_cust);

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
    int lc = envVar::instance()->getVarInt("db-worker-queueloops",0);    

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
    time_t tstart = time(NULL);    
    
    int cid_array[ORDERS_PER_DIST];
    gen_cid_array(cid_array);

    /* 1. create the loader threads */
    int num_tbl = SHORE_TPCC_TABLES;

    // P-Loader
    {
	guard<table_creator_t> tc;
	tc = new table_creator_t(this, _scaling_factor);
	tc->fork();
	tc->join();
    }
    guard<checkpointer_t> chk(new checkpointer_t(this));
    chk->fork();
    
    int loaders_to_use = envVar::instance()->getVarInt("db-loaders",10);
    array_guard_t< guard<table_builder_t> > loaders(new guard<table_builder_t>[loaders_to_use]);
    long total_units = _scaling_factor*UNIT_PER_WH;

    // WARNING: unit_per_thread must divide by ORDERS_PER_UNIT!
    long units_per_thread = (total_units + loaders_to_use-1)/loaders_to_use;
    long divisor = ORDERS_PER_DIST/ORDERS_PER_UNIT;
    units_per_thread = ((units_per_thread + divisor-1)/divisor)*divisor;
    for(int i=0; i < loaders_to_use; i++) {
	long start = i*units_per_thread;
	long count = (start+units_per_thread > total_units)? total_units-start : units_per_thread;
	loaders[i] = new table_builder_t(this, start, count, cid_array);
	loaders[i]->fork();
    }
    for(int i=0; i < loaders_to_use; i++)
	loaders[i]->join();

    time_t tstop = time(NULL);

    /* 5. print stats */
    TRACE( TRACE_STATISTICS, "Loading finished. %d table loaded in (%d) secs...\n",
           num_tbl, (tstop - tstart));

    /* 6. notify that the env is loaded */
    _loaded = true;
    chk->join();

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
    int num_tbl = SHORE_TPCC_TABLES;
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
//     table_man_t* ptable_man = NULL;
//     for(table_man_list_iter table_man_iter = _table_man_list.begin(); 
//         table_man_iter != _table_man_list.end(); table_man_iter++)
//         {
//             ptable_man = *table_man_iter;
//             ptable_man->print_table(this->_pssm);
//         }
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
    warehouse_t* wh = warehouse_desc();
    index_desc_t* idx = wh->indexes();
    int icount = wh->index_count();
    W_DO(wh->find_fid(db));
    stid_t wh_fid = wh->fid();

    // lock the table and index(es) for exclusive access
    W_DO(db->lock(wh_fid, EX));
    for(int i=0; i < icount; i++) {
	W_DO(idx[i].check_fid(db));
	for(int j=0; j < idx[i].get_partition_count(); j++)
	    W_DO(db->lock(idx[i].fid(j), EX));
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
		    int pnum = _pwarehouse_man->get_pnum(&idx[i], &row);
		    stid_t fid = idx[i].fid(pnum);
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
