/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcb_env.cpp
 *
 *  @brief:  Declaration of the Shore TPC-C environment (database)
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#include "workload/tpcb/shore_tpcb_env.h"
#include "sm/shore/shore_helper_loader.h"
#include <atomic.h>

using namespace shore;


ENTER_NAMESPACE(tpcb);



/** Exported functions */


/******************************************************************** 
 *
 * ShoreTPCBEnv functions
 *
 ********************************************************************/ 

const int ShoreTPCBEnv::load_schema()
{
    // get the sysname type from the configuration
    _sysname = _dev_opts[SHORE_DB_OPTIONS[4][0]];
    TRACE( TRACE_ALWAYS, "Sysname (%s)\n", _sysname.c_str());

    // create the schema
    _pbranch_desc  = new branch_t(_sysname);
    _pteller_desc   = new teller_t(_sysname);
    _paccount_desc   = new account_t(_sysname);
    _phistory_desc    = new history_t(_sysname);


    // initiate the table managers
    _pbranch_man  = new branch_man_impl(_pbranch_desc.get());
    _pteller_man   = new teller_man_impl(_pteller_desc.get());
    _paccount_man      = new account_man_impl(_paccount_desc.get());
    _phistory_man    = new history_man_impl(_phistory_desc.get());

    // XXX: !!! Warning !!!
    //
    // The two lists should have the description and the manager
    // of the same table in the same position
    //

    //// add the table managers to a list

    // (ip) Adding them in descending file order, so that the large
    //      files to be loaded at the begining. Expection is the
    //      WH and DISTR which are always the first two.
    _table_man_list.push_back(_pbranch_man);
    _table_man_list.push_back(_pteller_man);
    _table_man_list.push_back(_paccount_man);
    _table_man_list.push_back(_phistory_man);

    assert (_table_man_list.size() == 4);
        
    //// add the table descriptions to a list
    _table_desc_list.push_back(_pbranch_desc.get());
    _table_desc_list.push_back(_pteller_desc.get());
    _table_desc_list.push_back(_paccount_desc.get());
    _table_desc_list.push_back(_phistory_desc.get());

    assert (_table_desc_list.size() == 4);
        
    return (0);
}


/******************************************************************** 
 *
 *  @fn:    info()
 *
 *  @brief: Prints information about the current db instance status
 *
 ********************************************************************/

const int ShoreTPCBEnv::info()
{
    TRACE( TRACE_ALWAYS, "SF      = (%d)\n", _scaling_factor);
    TRACE( TRACE_ALWAYS, "Workers = (%d)\n", _worker_cnt);
    return (0);
}


/******************************************************************** 
 *
 *  @fn:    start()
 *
 *  @brief: Starts the tpcb env
 *
 ********************************************************************/

const int ShoreTPCBEnv::start()
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
 *  @brief: Stops the tpcb env
 *
 ********************************************************************/

const int ShoreTPCBEnv::stop()
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

void ShoreTPCBEnv::set_qf(const int aQF)
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


void ShoreTPCBEnv::set_sf(const int aSF)
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

const int ShoreTPCBEnv::upd_sf()
{
    // update whs
    int tmp_sf = envVar::instance()->getSysVarInt("sf");
    assert (tmp_sf);
    set_sf(tmp_sf);
    set_qf(tmp_sf);
    //print_sf();
    return (_scaling_factor);
}


void ShoreTPCBEnv::print_sf(void)
{
    TRACE( TRACE_ALWAYS, "*** ShoreTPCBEnv ***\n");
    TRACE( TRACE_ALWAYS, "Scaling Factor = (%d)\n", get_sf());
    TRACE( TRACE_ALWAYS, "Queried Factor = (%d)\n", get_qf());
}


const int ShoreTPCBEnv::upd_worker_cnt()
{
    // update worker thread cnt
    int workers = envVar::instance()->getVarInt("db-workers",0);
    assert (workers);
    _worker_cnt = workers;
    return (_worker_cnt);
}


class ShoreTPCBEnv::table_builder_t : public thread_t {
    ShoreTPCBEnv* _env;
    int _sf;
    long _start;
    long _count;
public:
    table_builder_t(ShoreTPCBEnv* env, int sf, long start, long count)
	: thread_t("TPC-B loader"), _env(env), _sf(sf), _start(start), _count(count) { }
    virtual void work();
};

struct ShoreTPCBEnv::table_creator_t : public thread_t {
    ShoreTPCBEnv* _env;
    int _sf;
    long _psize;
    long _pcount;
    table_creator_t(ShoreTPCBEnv* env, int sf, long psize, long pcount)
	: thread_t("TPC-B Table Creator"), _env(env), _sf(sf), _psize(psize), _pcount(pcount) { }
    virtual void work();
};

void  ShoreTPCBEnv::table_creator_t::work() {
    /* create the tables */
    W_COERCE(_env->db()->begin_xct());
    W_COERCE(_env->_pbranch_desc->create_table(_env->db()));
    W_COERCE(_env->_pteller_desc->create_table(_env->db()));
    W_COERCE(_env->_paccount_desc->create_table(_env->db()));
    W_COERCE(_env->_phistory_desc->create_table(_env->db()));
    W_COERCE(_env->db()->commit_xct());

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
}
void ShoreTPCBEnv::table_builder_t::work() {
    long total = _sf*ACCOUNTS_PER_BRANCH/ACCOUNTS_CREATED_PER_POP_XCT;
    long which;
    w_rc_t e;

    for(int i=0; i < _count; i += ACCOUNTS_CREATED_PER_POP_XCT) {
	long a_id = _start + i;
	populate_db_input_t in(_sf, a_id);
	trx_result_tuple_t out;
	fprintf(stderr, "Populating %d a_ids starting with %d\n", ACCOUNTS_CREATED_PER_POP_XCT, a_id);
	W_COERCE(_env->db()->begin_xct());
	e = _env->xct_populate_db(&in, a_id, out);
	if(e.is_error()) {
	    stringstream os;
	    os << e << ends;
	    string str = os.str();
	    fprintf(stderr, "Eek! Unable to populate db for index %d due to:\n%s\n",
		    i, str.c_str());
	    
	    w_rc_t e2 = _env->db()->abort_xct();
	    if(e2.is_error()) {
		TRACE( TRACE_ALWAYS, "Double-eek! Unable to abort trx for index %d due to [0x%x]\n", 
		       which, e2.err_num());
	    }
	}
    }
}


/********
 ******** Caution: The functions below should be invoked inside
 ********          the context of a smthread
 ********/


/****************************************************************** 
 *
 * @fn:    loaddata()
 *
 * @brief: Loads the data for all the TPCB tables, given the current
 *         scaling factor value. During the loading the SF cannot be
 *         changed.
 *
 ******************************************************************/

w_rc_t ShoreTPCBEnv::loaddata() 
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

    /* partly (no) thanks to Shore's next key index locking, and
       partly due to page latch and SMO issues, we have ridiculous
       deadlock rates if we try to throw lots of threads at a small
       btree. To work around this we'll partition the space of
       accounts into LOADERS_TO_USE segments and have a single thread
       load the first 10k accounts from each partition before firing
       up the real workers.
     */
    static int const LOADERS_TO_USE = 40;
    long total_accounts = _scaling_factor*ACCOUNTS_PER_BRANCH;
    w_assert1((total_accounts % LOADERS_TO_USE) == 0);
    long accts_per_worker = total_accounts/LOADERS_TO_USE;
    
    time_t tstart = time(NULL);
    
    TRACE( TRACE_DEBUG, "Loaddir (%s)\n", loaddatadir.c_str());

    {
	guard<table_creator_t> tc;
	tc = new table_creator_t(this, _scaling_factor, accts_per_worker, LOADERS_TO_USE);
	tc->fork();
	tc->join();
    }
    
    /* This number is really flexible. Basically, it just needs to be
       high enough to give good parallelism, while remaining low
       enough not to cause too much contention. I pulled '40' out of
       thin air.
     */
    guard<table_builder_t> loaders[LOADERS_TO_USE];
    for(long i=0; i < LOADERS_TO_USE; i++) {
	// the preloader thread picked up that first set of accounts...
	long start = accts_per_worker*i+ACCOUNTS_CREATED_PER_POP_XCT;
	long count = accts_per_worker-ACCOUNTS_CREATED_PER_POP_XCT;
	loaders[i] = new table_builder_t(this, _scaling_factor, start, count);
	loaders[i]->fork();
    }
    
    for(int i=0; i<LOADERS_TO_USE; i++) {
	loaders[i]->join();        
    }

    /* 4. join the loading threads */
    time_t tstop = time(NULL);

    /* 5. print stats */
    TRACE( TRACE_STATISTICS, "Loading finished. %d branches loaded in (%d) secs...\n",
           _scaling_factor, (tstop - tstart));

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

w_rc_t ShoreTPCBEnv::check_consistency()
{
    // not loaded from files, so no inconsistency possible
    return RCOK;
}


/****************************************************************** 
 *
 * @fn:    warmup()
 *
 * @brief: Touches the entire database - For memory-fitting databases
 *         this is enough to bring it to load it to memory
 *
 ******************************************************************/

w_rc_t ShoreTPCBEnv::warmup()
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

const int ShoreTPCBEnv::dump()
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


const int ShoreTPCBEnv::conf()
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

const int ShoreTPCBEnv::post_init() 
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

w_rc_t ShoreTPCBEnv::_post_init_impl() 
{
    ss_m* db = this->db();
    
    // lock the WH table
    typedef branch_t warehouse_t;
    typedef branch_man_impl warehouse_man_impl;
    
    warehouse_t* wh = branch();
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
	    W_DO(branch_man()->get_iter_for_file_scan(db, tmp));
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
		    int key_sz = branch_man()->format_key(idx+i, &row, arep);
		    vec_t kvec(arep._dest, key_sz);

		    /* destroy the old mapping and replace it with the new
		       one.  If it turns out this is super-slow, we can
		       look into probing the index with a cursor and
		       updating it directly.
		    */
		    int pnum = _pbranch_man->get_pnum(&idx[i], &row);
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




/********************************************************************* 
 *
 *  TLS row_impl objects
 *
 *********************************************************************/ 


// typedef row_impl<warehouse_t>  warehouse_tuple;
// typedef row_impl<district_t>   district_tuple;
// typedef row_impl<customer_t>   customer_tuple;
// typedef row_impl<history_t>    history_tuple;
// typedef row_impl<new_order_t>  new_order_tuple;
// typedef row_impl<order_t>      order_tuple;
// typedef row_impl<order_line_t> order_line_tuple;
// typedef row_impl<item_t>       item_tuple;
// typedef row_impl<stock_t>      stock_tuple;



// DECLARE_TUPLE_INTERFACE(warehouse_tuple,warehouse,warehouse_pool)
// DECLARE_TUPLE_INTERFACE(district_tuple,district,district_pool)
// DECLARE_TUPLE_INTERFACE(customer_tuple,customer,customer_pool)
// DECLARE_TUPLE_INTERFACE(history_tuple,history,history_pool)
// DECLARE_TUPLE_INTERFACE(new_order_tuple,new_order,new_order_pool)
// DECLARE_TUPLE_INTERFACE(order_tuple,order,order_pool)
// DECLARE_TUPLE_INTERFACE(order_line_tuple,order_line,order_line_pool)
// DECLARE_TUPLE_INTERFACE(item_tuple,item,item_pool)
// DECLARE_TUPLE_INTERFACE(stock_tuple,stock,stock_pool)
  


EXIT_NAMESPACE(tpcb);