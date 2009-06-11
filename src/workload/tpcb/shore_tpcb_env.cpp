/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University

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
/** @file:   shore_tpcb_env.cpp
 *
 *  @brief:  Declaration of the Shore TPC-C environment (database)
 *
 *  @author: Ryan Johnson, Feb 2009
 *  @author: Ippokratis Pandis, Feb 2009
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
    _sysname = "baseline";
    TRACE( TRACE_ALWAYS, "Sysname (%s)\n", _sysname.c_str());

    // create the schema
    _pbranch_desc   = new branch_t(_sysname);
    _pteller_desc   = new teller_t(_sysname);
    _paccount_desc  = new account_t(_sysname);
    _phistory_desc  = new history_t(_sysname);


    // initiate the table managers
    _pbranch_man   = new branch_man_impl(_pbranch_desc.get());
    _pteller_man   = new teller_man_impl(_pteller_desc.get());
    _paccount_man  = new account_man_impl(_paccount_desc.get());
    _phistory_man  = new history_man_impl(_phistory_desc.get());
        
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

    TRACE( TRACE_ALWAYS, "Starting (%s)\n", _sysname.c_str());      
    info();
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



struct ShoreTPCBEnv::checkpointer_t : public thread_t {
    ShoreTPCBEnv* _env;
    checkpointer_t(ShoreTPCBEnv* env) : thread_t("TPC-C Load Checkpointer"), _env(env) { }
    virtual void work();
};




/****************************************************************** 
 *
 * @struct: table_creator_t
 *
 * @brief:  Helper class for creating the environment tables and
 *          loading a number of records in a single-threaded fashion
 *
 ******************************************************************/

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

void ShoreTPCBEnv::table_builder_t::work() {
    w_rc_t e;

    for(int i=0; i < _count; i += TPCB_ACCOUNTS_CREATED_PER_POP_XCT) {
	long a_id = _start + i;
	populate_db_input_t in(_sf, a_id);
	trx_result_tuple_t out;
	fprintf(stderr, ".");
	//fprintf(stderr, "Populating %d a_ids starting with %d\n", ACCOUNTS_CREATED_PER_POP_XCT, a_id);
    retry:
	W_COERCE(_env->db()->begin_xct());
	e = _env->xct_populate_db(a_id, out, in);
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
    }
    fprintf(stderr, "Finished loading account groups %d .. %d \n", _start, _start+_count);
}




/****************************************************************** 
 *
 * @class: table_builder_t
 *
 * @brief:  Helper class for loading the environment tables
 *
 ******************************************************************/


struct ShoreTPCBEnv::table_creator_t : public thread_t {
    ShoreTPCBEnv* _env;
    int _sf;
    long _psize;
    long _pcount;
    table_creator_t(ShoreTPCBEnv* env, int sf, long psize, long pcount)
	: thread_t("TPC-B Table Creator"), _env(env), _sf(sf), _psize(psize), _pcount(pcount) { }
    virtual void work();
};

void ShoreTPCBEnv::checkpointer_t::work() {
    bool volatile* loaded = &_env->_loaded;
    while(!*loaded) {
	_env->set_measure(MST_MEASURE);
	for(int i=0; i < 60 && !*loaded; i++) 
	    ::sleep(1);
	
	//	_env->set_measure(MST_PAUSE);
	
        TRACE( TRACE_ALWAYS, "db checkpoint - start\n");
        _env->checkpoint();
        TRACE( TRACE_ALWAYS, "db checkpoint - end\n");
    }
}



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
	fprintf(stderr, "Populating %d a_ids starting with %d\n", TPCB_ACCOUNTS_CREATED_PER_POP_XCT, a_id);
	W_COERCE(_env->db()->begin_xct());
	W_COERCE(_env->xct_populate_db(a_id, out, in));
    }

    W_COERCE(_env->db()->begin_xct());
    W_COERCE(_env->_post_init_impl());
    W_COERCE(_env->db()->commit_xct());
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

    int num_tbl = 4;
    int cnt = 0;

    guard<checkpointer_t> chk(new checkpointer_t(this));
    chk->fork();
    
    /* partly (no) thanks to Shore's next key index locking, and
       partly due to page latch and SMO issues, we have ridiculous
       deadlock rates if we try to throw lots of threads at a small
       btree. To work around this we'll partition the space of
       accounts into LOADERS_TO_USE segments and have a single thread
       load the first 10k accounts from each partition before firing
       up the real workers.
     */
    int loaders_to_use = envVar::instance()->getVarInt("db-loaders",10);
    long total_accounts = _scaling_factor*TPCB_ACCOUNTS_PER_BRANCH;
    w_assert1((total_accounts % loaders_to_use) == 0);
    long accts_per_worker = total_accounts/loaders_to_use;
    
    time_t tstart = time(NULL);
    
    {
	guard<table_creator_t> tc;
	tc = new table_creator_t(this, _scaling_factor, accts_per_worker, loaders_to_use);
	tc->fork();
	tc->join();
    }
    
    /* This number is really flexible. Basically, it just needs to be
       high enough to give good parallelism, while remaining low
       enough not to cause too much contention. I pulled '40' out of
       thin air.
     */
    array_guard_t< guard<table_builder_t> > loaders(new guard<table_builder_t>[loaders_to_use]);
    for(long i=0; i < loaders_to_use; i++) {
	// the preloader thread picked up that first set of accounts...
	long start = accts_per_worker*i+TPCB_ACCOUNTS_CREATED_PER_POP_XCT;
	long count = accts_per_worker-TPCB_ACCOUNTS_CREATED_PER_POP_XCT;
	loaders[i] = new table_builder_t(this, _scaling_factor, start, count);
	loaders[i]->fork();
    }
    
    for(int i=0; i<loaders_to_use; i++) {
	loaders[i]->join();        
    }

    /* 4. join the loading threads */
    time_t tstop = time(NULL);

    /* 5. print stats */
    TRACE( TRACE_STATISTICS, "Loading finished. %d branches loaded in (%d) secs...\n",
           _scaling_factor, (tstop - tstart));

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
//     table_man_t* ptable_man = NULL;
//     for(table_man_list_iter table_man_iter = _table_man_list.begin(); 
//         table_man_iter != _table_man_list.end(); table_man_iter++)
//         {
//             ptable_man = *table_man_iter;
//             ptable_man->print_table(this->_pssm);
//         }
    return (0);
}


const int ShoreTPCBEnv::conf()
{
    // reread the params
    ShoreEnv::conf();
    upd_sf();
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
    
    warehouse_t* wh = branch_desc();
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
  


EXIT_NAMESPACE(tpcb);
