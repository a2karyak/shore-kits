/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
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

/** @file:   shore_tm1_env.cpp
 *
 *  @brief:  Declaration of the Shore TM1 environment (database)
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */

#include "workload/tm1/shore_tm1_env.h"
#include "sm/shore/shore_helper_loader.h"

using namespace shore;


ENTER_NAMESPACE(tm1);


/** Exported functions */

/******************************************************************** 
 *
 * ShoreTM1Env functions
 *
 ********************************************************************/ 

const int ShoreTM1Env::load_schema()
{
    // get the sysname type from the configuration
    _sysname = envVar::instance()->getSysname();
    TRACE( TRACE_ALWAYS, "Sysname (%s)\n", _sysname.c_str());

    // create the schema
    _psub_desc  = new subscriber_t(_sysname);
    _pai_desc   = new access_info_t(_sysname);
    _psf_desc   = new special_facility_t(_sysname);
    _pcf_desc   = new call_forwarding_t(_sysname);

    // initiate the table managers
    _psub_man = new sub_man_impl(_psub_desc.get());
    _pai_man  = new ai_man_impl(_pai_desc.get());
    _psf_man  = new sf_man_impl(_psf_desc.get());
    _pcf_man  = new cf_man_impl(_pcf_desc.get());
        
    return (0);
}



/******************************************************************** 
 *
 *  @fn:    info()
 *
 *  @brief: Prints information about the current db instance status
 *
 ********************************************************************/

const int ShoreTM1Env::info()
{
    TRACE( TRACE_ALWAYS, "SF      = (%d)\n", _scaling_factor);
    TRACE( TRACE_ALWAYS, "Workers = (%d)\n", _worker_cnt);
    return (0);
}



/******************************************************************** 
 *
 *  @fn:    statistics
 *
 *  @brief: Prints statistics for TM1 
 *
 ********************************************************************/

const int ShoreTM1Env::statistics() 
{
    // read the current trx statistics
    CRITICAL_SECTION(cs, _statmap_mutex);
    ShoreTM1TrxStats rval;
    rval -= rval; // dirty hack to set all zeros
    for (statmap_t::iterator it=_statmap.begin(); it != _statmap.end(); ++it) 
	rval += *it->second;

    TRACE( TRACE_STATISTICS, "GebSubData. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.get_sub_data,
           rval.failed.get_sub_data,
           rval.deadlocked.get_sub_data);

    TRACE( TRACE_STATISTICS, "GebNewDest. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.get_new_dest,
           rval.failed.get_new_dest,
           rval.deadlocked.get_new_dest);

    TRACE( TRACE_STATISTICS, "GebAccData. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.get_acc_data,
           rval.failed.get_acc_data,
           rval.deadlocked.get_acc_data);

    TRACE( TRACE_STATISTICS, "UpdSubData. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.upd_sub_data,
           rval.failed.upd_sub_data,
           rval.deadlocked.upd_sub_data);

    TRACE( TRACE_STATISTICS, "UpdLocation. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.upd_loc,
           rval.failed.upd_loc,
           rval.deadlocked.upd_loc);

    TRACE( TRACE_STATISTICS, "InsCallFwd. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.ins_call_fwd,
           rval.failed.ins_call_fwd,
           rval.deadlocked.ins_call_fwd);

    TRACE( TRACE_STATISTICS, "DelCallFwd. Att (%d). Abt (%d). Dld (%d)\n",
           rval.attempted.del_call_fwd,
           rval.failed.del_call_fwd,
           rval.deadlocked.del_call_fwd);

    return (0);
}



/******************************************************************** 
 *
 *  @fn:    start()
 *
 *  @brief: Starts the tm1 env
 *
 ********************************************************************/

const int ShoreTM1Env::start()
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
 *  @brief: Stops the tm1 env
 *
 ********************************************************************/

const int ShoreTM1Env::stop()
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
 *  @fn:    set_sf
 *
 *  @brief: Set the scaling factor
 *
 ********************************************************************/

void ShoreTM1Env::set_qf(const int aQF)
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

void ShoreTM1Env::set_sf(const int aSF)
{

    if (aSF > 0) {
        TRACE( TRACE_ALWAYS, "New Scaling factor: %d\n", aSF);
        _scaling_factor = aSF;
    }
    else {
        TRACE( TRACE_ALWAYS, "Invalid scaling factor input: %d\n", aSF);
    }
}

const int ShoreTM1Env::upd_sf()
{
    int tmp_sf = envVar::instance()->getSysVarInt("sf");
    assert (tmp_sf);
    set_sf(tmp_sf);
    return (_scaling_factor);
}


const int ShoreTM1Env::upd_worker_cnt()
{
    // update worker thread cnt
    int workers = envVar::instance()->getVarInt("db-workers",0);
    assert (workers);
    _worker_cnt = workers;
    return (_worker_cnt);
}



/****************************************************************** 
 *
 * @struct: table_creator_t
 *
 * @brief:  Helper class for creating the environment tables and
 *          loading a number of records in a single-threaded fashion
 *
 ******************************************************************/

struct ShoreTM1Env::table_creator_t : public thread_t 
{
    ShoreTM1Env* _env;
    int _loaders;
    int _subs_per_worker;
    int _preloads_per_worker;

    table_creator_t(ShoreTM1Env* env, 
                    const int loaders, const int subs_per_worker, const int preloads_per_worker)
	: thread_t("TM1-Table-Creator"), _env(env),
          _loaders(loaders),_subs_per_worker(subs_per_worker),_preloads_per_worker(preloads_per_worker)
    { 
        assert(loaders);
        assert(subs_per_worker);
        assert(preloads_per_worker>=0);
    }
    virtual void work();

}; // EOF: ShoreTM1Env::table_creator_t


void  ShoreTM1Env::table_creator_t::work() 
{
    // 1. Create the tables
    W_COERCE(_env->db()->begin_xct());
    W_COERCE(_env->_psub_desc->create_table(_env->db()));
    W_COERCE(_env->_pai_desc->create_table(_env->db()));
    W_COERCE(_env->_psf_desc->create_table(_env->db()));
    W_COERCE(_env->_pcf_desc->create_table(_env->db()));
    W_COERCE(_env->db()->commit_xct());

    // 2. Preload (preloads_per_worker) records for each of the loaders
    int sub_id = 0;
    for (int i=0; i<_loaders; i++) {
        W_COERCE(_env->db()->begin_xct());        
        TRACE( TRACE_ALWAYS, "Preloading (%d). Start (%d). Todo (%d)\n", 
               i, (i*_subs_per_worker), _preloads_per_worker);
            
        for (int j=0; j<_preloads_per_worker; j++) {
            sub_id = i*_subs_per_worker + j;
            W_COERCE(_env->xct_populate_one(sub_id));
        }
        W_COERCE(_env->db()->commit_xct());
    }
}


/****************************************************************** 
 *
 * @class: table_builder_t
 *
 * @brief:  Helper class for loading the environment tables
 *
 ******************************************************************/

class ShoreTM1Env::table_builder_t : public thread_t 
{
    ShoreTM1Env* _env;
    int _loader_id;
    int _start;
    int _count;
public:
    table_builder_t(ShoreTM1Env* env, int loaderid, int sf, int start, int count)
	: thread_t(c_str("TM1-Loader-%d",loaderid)), 
          _env(env), _loader_id(loaderid), _start(start), _count(count) 
    { }
    virtual void work();

}; // EOF: ShoreTM1Env::table_builder_t


void ShoreTM1Env::table_builder_t::work() 
{
    assert (_count>0);
    
    w_rc_t e = RCOK;
    int commitmark = 0;
    int tracemark = 0;
    int subsadded = 0;

    W_COERCE(_env->db()->begin_xct());

    // add _count number of Subscribers (with the corresponding AI, SF, and CF entries)
    int last_commit = 0;
    for (subsadded=0; subsadded<_count; ++subsadded) {
    again:
	int sub_id = _start + subsadded;

        // insert row
	e = _env->xct_populate_one(sub_id);

	if(e.is_error()) {
	    W_COERCE(_env->db()->abort_xct());
	    if(e.err_num() == smlevel_0::eDEADLOCK) {
		W_COERCE(_env->db()->begin_xct());
		subsadded = last_commit + 1;
		goto again;
	    }
	    stringstream os;
	    os << e << ends;
	    string str = os.str();
	    TRACE( TRACE_ALWAYS, "Unable to Insert Subscriber (%d) due to:\n%s\n",
                   sub_id, str.c_str());
	}

        // Output some information
        if (subsadded>=tracemark) {
            TRACE( TRACE_ALWAYS, "Start (%d). Todo (%d). Added (%d)\n", 
                   _start, _count, subsadded);
            tracemark += TM1_LOADING_TRACE_INTERVAL;
        }

        // Commit every now and then
        if (subsadded>=commitmark) {
            e = _env->db()->commit_xct();

            if (e.is_error()) {
                stringstream os;
                os << e << ends;
                string str = os.str();
                TRACE( TRACE_ALWAYS, "Unable to Commit (%d) due to:\n%s\n",
                       sub_id, str.c_str());
	    
                w_rc_t e2 = _env->db()->abort_xct();
                if(e2.is_error()) {
                    TRACE( TRACE_ALWAYS, "Unable to abort trx for Subscriber (%d) due to [0x%x]\n", 
                           sub_id, e2.err_num());
                }
            }

	    last_commit = subsadded;
            commitmark += TM1_LOADING_COMMIT_INTERVAL;

            W_COERCE(_env->db()->begin_xct());
        }
    }

    // final commit
    e = _env->db()->commit_xct();

    if (e.is_error()) {
        stringstream os;
        os << e << ends;
        string str = os.str();
        TRACE( TRACE_ALWAYS, "Unable to final Commit due to:\n%s\n",
               str.c_str());
	    
        w_rc_t e2 = _env->db()->abort_xct();
        if(e2.is_error()) {
            TRACE( TRACE_ALWAYS, "Unable to abort trx due to [0x%x]\n", 
                   e2.err_num());
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
 * @brief: Loads the data for all the TM1 tables, given the current
 *         scaling factor value. During the loading the SF cannot be
 *         changed.
 *
 ******************************************************************/

w_rc_t ShoreTM1Env::loaddata() 
{
    // 0. lock the loading status
    CRITICAL_SECTION(load_cs, _load_mutex);
    if (_loaded) {
        TRACE( TRACE_TRX_FLOW, 
               "Env already loaded. Doing nothing...\n");
        return (RCOK);
    }        

    // 1. create the loader threads

    /* partly (no) thanks to Shore's next key index locking, and
       partly due to page latch and SMO issues, we have ridiculous
       deadlock rates if we try to throw lots of threads at a small
       btree. To work around this we'll partition the space of
       subscribers into TM1_LOADERS_TO_USE segments and have a single 
       thread load the first TM1_SUBS_TO_PRELOAD (2k) subscribers 
       from each partition before firing up the real workers.
     */
    
    int loaders_to_use = envVar::instance()->getVarInt("db-loaders",TM1_LOADERS_TO_USE);
    w_assert1( loaders_to_use <= TM1_MAX_NUM_OF_LOADERS);
    int preloads_per_worker = envVar::instance()->getVarInt("db-record-preloads",TM1_SUBS_TO_PRELOAD);

    int total_subs = _scaling_factor*TM1_SUBS_PER_SF;

    w_assert1((total_subs % loaders_to_use) == 0);

    int subs_per_worker = total_subs/loaders_to_use;    
    time_t tstart = time(NULL);

    {
	guard<table_creator_t> tc;
	tc = new table_creator_t(this, loaders_to_use, subs_per_worker, preloads_per_worker);
	tc->fork();
	tc->join();
    }
    
    /* This number is really flexible. Basically, it just needs to be
       high enough to give good parallelism, while remaining low
       enough not to cause too much contention. Ryan pulled '40' out of
       thin air.
     */
    table_builder_t* loaders[TM1_MAX_NUM_OF_LOADERS];
    for (int i=0; i<loaders_to_use; i++) {
	// the preloader thread picked up a first set of subscribers...
	int start = i*subs_per_worker + preloads_per_worker;
	int count = subs_per_worker - preloads_per_worker;
	loaders[i] = new table_builder_t(this, i, _scaling_factor, start, count);
	loaders[i]->fork();
    }
    
    for(int i=0; i<loaders_to_use; i++) {
	loaders[i]->join();        
    }


    /* 4. join the loading threads */
    time_t tstop = time(NULL);

    /* 5. print stats */
    TRACE( TRACE_STATISTICS, "Loading finished. %d subscribers loaded in (%d) secs...\n",
           total_subs, (tstop - tstart));

    /* 6. notify that the env is loaded */
    _loaded = true;

    return (RCOK);
}


/****************************************************************** 
 *
 * @fn:    conf()
 *
 ******************************************************************/

const int ShoreTM1Env::conf()
{
    // reread the params
    ShoreEnv::conf();
    upd_sf();
    upd_worker_cnt();
    return (0);
}



/********************************************************************* 
 *
 *  @fn:    _post_init_impl
 *
 *********************************************************************/ 

const int ShoreTM1Env::post_init() 
{
    return (0);
}

w_rc_t ShoreTM1Env::_post_init_impl() 
{
    TRACE(TRACE_DEBUG, "Doing nothing\n");
    return (RCOK);
}

 
EXIT_NAMESPACE(tm1);
