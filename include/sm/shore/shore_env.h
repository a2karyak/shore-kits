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

/** @file shore_env.h
 *
 *  @brief Definition of a Shore environment (database)
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_ENV_H
#define __SHORE_ENV_H

#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif

#include <map>

// for binding LWP to cores
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>

#include "sm_vas.h"
#include "util.h"

#include "shore_reqs.h"


ENTER_NAMESPACE(shore);


using std::map;



/******** Constants ********/


const int SHORE_DEF_NUM_OF_CORES     = 32; // default number of cores

const int SHORE_NUM_OF_RETRIES       = 3;

#define SHORE_TABLE_DATA_DIR  "databases"
#define SHORE_CONF_FILE       "shore.conf"

const string CONFIG_PARAM       = "db-config";
const string CONFIG_PARAM_VALUE = "invalid";


// !!!
// shore.conf should have values with the corresponding configuration suffix
// for all the database-instance-dependent parameters
// !!!


// SHORE_SYS_OPTIONS:
// Database-independent options 
const string SHORE_SYS_OPTIONS[][2] = {
    { "db-clobberdev", "0" },
    { "sys-maxcpucount", "0" },
    { "sys-activecpucount", "0" },
    { "shore-fakeiodelay", "0" },
    { "shore-fakeiodelay-enable", "0" },
};

const int    SHORE_NUM_SYS_OPTIONS  = 5;


// SHORE_SYS_SM_OPTIONS: 
// Those options are appended as parameters when starting Shore
// Those are the database-independent
const string SHORE_SYS_SM_OPTIONS[][3]  = {
    { "-sm_logging", "shore-logging", "yes" },
    { "-sm_logisraw", "shore-logisraw", "no" },
    { "-sm_diskrw", "shore-diskrw", "diskrw" },
    { "-sm_errlog", "shore-errlog", "info" },
    { "-sm_num_page_writers", "shore-pagecleaners", "16" },
};

const int    SHORE_NUM_SYS_SM_OPTIONS   = 5;


// SHORE_DB_SM_OPTIONS: 
// Those options are appended as parameters when starting Shore
// Thore are the database-instance-specific
const string SHORE_DB_SM_OPTIONS[][3]  = {
    { "-sm_bufpoolsize", "bufpoolsize", "0" },
    { "-sm_logdir", "logdir", "log" },
    { "-sm_logsize", "logsize", "0" },
    { "-sm_logbufsize", "logbufsize", "0" },
    { "-sm_logcount", "logcount", "10" },
};

const int    SHORE_NUM_DB_SM_OPTIONS   = 5;


// SHORE_DB_OPTIONS
// Database-instance-specific options
const string SHORE_DB_OPTIONS[][2] = {
    { "device", "databases/shore" },
    { "devicequota", "0" },
    { "loadatadir", SHORE_TABLE_DATA_DIR },
    { "sf", "0" },
    { "system", "invalid" },
};

const int    SHORE_NUM_DB_OPTIONS  = 5;




/****************************************************************** 
 *
 * MACROS used in _env, _schema, _xct files
 *
 ******************************************************************/

#define DECLARE_TRX(trx) \
    w_rc_t run_##trx(Request* prequest, trx##_input_t& in);             \
    w_rc_t run_##trx(Request* prequest);                                \
    w_rc_t xct_##trx(const int xct_id, trx##_input_t& in);              \
    void   _inc_##trx##_att();                                          \
    void   _inc_##trx##_failed();                                       \
    void   _inc_##trx##_dld()


#define DECLARE_TABLE(table,manimpl,abbrv)                              \
    guard<manimpl>   _p##abbrv##_man;                                   \
    inline manimpl*  abbrv##_man() { return (_p##abbrv##_man); }        \
    guard<table>     _p##abbrv##_desc;                                  \
    inline table*    abbrv##_desc() { return (_p##abbrv##_desc.get()); }


#ifdef CFG_FLUSHER // ***** Mainstream FLUSHER ***** //
// Commits lazily


#define DEFINE_RUN_WITH_INPUT_TRX_WRAPPER(cname,trx)                    \
    w_rc_t cname::run_##trx(Request* prequest, trx##_input_t& in) {     \
        int xct_id = prequest->xct_id();                                \
        TRACE( TRACE_TRX_FLOW, "%d. %s ...\n", xct_id, #trx);           \
        ++my_stats.attempted.##trx;                                     \
        w_rc_t e = xct_##trx(xct_id, in);                               \
        if (!e.is_error()) {                                            \
            lsn_t xctLastLsn;                                           \
            e = _pssm->commit_xct(true,&xctLastLsn);                    \
            prequest->set_last_lsn(xctLastLsn); }                       \
        if (e.is_error()) {                                             \
            if (e.err_num() != smlevel_0::eDEADLOCK)                    \
                ++my_stats.failed.##trx;                                \
            else ++my_stats.deadlocked.##trx##;                         \
            TRACE( TRACE_TRX_FLOW, "Xct (%d) aborted [0x%x]\n", xct_id, e.err_num()); \
            w_rc_t e2 = _pssm->abort_xct();                             \
            if(e2.is_error()) TRACE( TRACE_ALWAYS, "Xct (%d) abort failed [0x%x]\n", xct_id, e2.err_num()); \
            prequest->notify_client();                                  \
            if ((*&_measure)!=MST_MEASURE) return (e);                  \
            _env_stats.inc_trx_att();                                   \
            return (e); }                                               \
        TRACE( TRACE_TRX_FLOW, "Xct (%d) (%d) to flush\n", xct_id, prequest->tid()); \
        to_base_flusher(prequest);                                      \
        return (RCOK); }

#else // ***** NO FLUSHER ***** //

#define DEFINE_RUN_WITH_INPUT_TRX_WRAPPER(cname,trx)                    \
    w_rc_t cname::run_##trx(Request* prequest, trx##_input_t& in) {     \
        int xct_id = prequest->xct_id();                                \
        TRACE( TRACE_TRX_FLOW, "%d. %s ...\n", xct_id, #trx);           \
        ++my_stats.attempted.##trx;                                     \
        w_rc_t e = xct_##trx(xct_id, in);                               \
        if (!e.is_error()) {                                            \
            if (isAsynchCommit()) e = _pssm->commit_xct(true);          \
            else e = _pssm->commit_xct(); }                             \
        if (e.is_error()) {                                             \
            if (e.err_num() != smlevel_0::eDEADLOCK)                    \
                ++my_stats.failed.##trx;                                \
            else ++my_stats.deadlocked.##trx##;                         \
            TRACE( TRACE_TRX_FLOW, "Xct (%d) aborted [0x%x]\n", xct_id, e.err_num()); \
            w_rc_t e2 = _pssm->abort_xct();                             \
            if(e2.is_error()) TRACE( TRACE_ALWAYS, "Xct (%d) abort failed [0x%x]\n", xct_id, e2.err_num()); \
            prequest->notify_client();                                  \
            if ((*&_measure)!=MST_MEASURE) return (e);                  \
            _env_stats.inc_trx_att();                                   \
            return (e); }                                               \
        TRACE( TRACE_TRX_FLOW, "Xct (%d) completed\n", xct_id);         \
        prequest->notify_client();                                      \
        if ((*&_measure)!=MST_MEASURE) return (RCOK);                   \
        _env_stats.inc_trx_com();                                       \
        return (RCOK); }

#endif // ***** EOF: CFG_FLUSHER ***** //


#define DEFINE_RUN_WITHOUT_INPUT_TRX_WRAPPER(cname,trx)                 \
    w_rc_t cname::run_##trx(Request* prequest) {                        \
        trx##_input_t in = create_##trx##_input(_queried_factor, prequest->selectedID()); \
        return (run_##trx(prequest, in)); }


#define DEFINE_TRX_STATS(cname,trx)                                   \
    void cname::_inc_##trx##_att()    { ++my_stats.attempted.##trx; } \
    void cname::_inc_##trx##_failed() { ++my_stats.failed.##trx; }    \
    void cname::_inc_##trx##_dld() { ++my_stats.deadlocked.##trx; }


#define DEFINE_TRX(cname,trx)                        \
    DEFINE_RUN_WITHOUT_INPUT_TRX_WRAPPER(cname,trx); \
    DEFINE_RUN_WITH_INPUT_TRX_WRAPPER(cname,trx);    \
    DEFINE_TRX_STATS(cname,trx)





/****************************************************************** 
 *
 *  @struct: env_stats_t
 *
 *  @brief:  Environment statistics - total trxs attempted/committed
 *
 ******************************************************************/

struct env_stats_t 
{
    volatile uint_t _ntrx_att;
    volatile uint_t _ntrx_com;

    env_stats_t() 
        : _ntrx_att(0), _ntrx_com(0)
    { }

    ~env_stats_t() { }

    void print_env_stats() const;

    inline uint_t inc_trx_att() { return (atomic_inc_uint_nv(&_ntrx_att)); }
    inline uint_t inc_trx_com() {
        atomic_inc_uint(&_ntrx_att);
        return (atomic_inc_uint_nv(&_ntrx_com)); }

}; // EOF env_stats_t



/******************************************************************** 
 *
 * @enum:  eDBControl
 *
 * @brief: States of a controlled DB
 *
 ********************************************************************/

enum eDBControl { DBC_UNDEF, DBC_PAUSED, DBC_ACTIVE, DBC_STOPPED };


/*********************************************************************
 *
 *  @abstract class: db_iface
 *
 *  @brief:          Interface of basic shell commands for dbs
 *
 *  @usage:          - Inherit from this class
 *                   - Implement the process_{START/STOP/PAUSE/RESUME} fuctions
 *
 *********************************************************************/

class db_iface
{
public:
    typedef map<string,string>        envVarMap;
    typedef envVarMap::iterator       envVarIt;
    typedef envVarMap::const_iterator envVarConstIt;

private:
    volatile eDBControl _dbc;

public:

    db_iface() 
        : _dbc(DBC_UNDEF)
    { }
    virtual ~db_iface() { }

    // Access methods

    const eDBControl dbc() { return(*&_dbc); }
    
    void set_dbc(const eDBControl adbc) {
        assert (adbc!=DBC_UNDEF);
        atomic_swap(&_dbc, adbc);
    }
    

    // DB INTERFACE     

    virtual int conf()=0;    
    virtual int set(envVarMap* vars)=0;    
    virtual int load_schema()=0;
    virtual int init()=0;
    virtual int open()=0;
    virtual int close()=0;
    virtual int start()=0;
    virtual int stop()=0;
    virtual int restart()=0;
    virtual int pause()=0;
    virtual int resume()=0;    
    virtual int newrun()=0;    
    virtual int statistics()=0;    
    virtual int dump()=0;    
    virtual int info()=0;    
    
}; // EOF: db_iface



// Forward decl
class base_worker_t;
class trx_worker_t;
class flusher_t;
class ShoreEnv;


/******** Exported variables ********/

extern ShoreEnv* _g_shore_env;




/******************************************************************** 
 *
 * @enum  MeasurementState
 *
 * @brief Possible states of a measurement in the Shore Enviroment
 *
 ********************************************************************/

enum MeasurementState { MST_UNDEF, MST_WARMUP, MST_MEASURE, MST_DONE, MST_PAUSE };


/******************************************************************** 
 * 
 *  ShoreEnv
 *  
 *  Shore database abstraction. Among others it configures, starts 
 *  and closes the Shore database 
 *
 ********************************************************************/

class ShoreEnv : public db_iface
{
public:
    typedef map<string,string> ParamMap;

    typedef trx_request_t Request;
    typedef atomic_class_stack<Request> RequestStack;

    typedef trx_worker_t                Worker;
    typedef trx_worker_t*               WorkerPtr;
    typedef vector<WorkerPtr>           WorkerPool;
    typedef WorkerPool::iterator        WorkerIt;

protected:       

    ss_m*           _pssm;               // database handle

    // Status variables
    bool            _initialized; 
    pthread_mutex_t _init_mutex;
    bool            _loaded;
    pthread_mutex_t _load_mutex;

    pthread_mutex_t _statmap_mutex;
    pthread_mutex_t _last_stats_mutex;

    // Device and volume. There is a single volume per device. 
    // The whole environment resides in a single volume.
    devid_t            _devid;     // device id
    guard<vid_t>       _pvid;      // volume id
    stid_t             _root_iid;  // root id of the volume
    pthread_mutex_t    _vol_mutex; // volume mutex
    lvid_t             _lvid;      // logical volume id (unnecessary, using physical ids)
    unsigned int       _vol_cnt;   // volume count (unnecessary, always 1)

    // Configuration variables
    guard<option_group_t>   _popts;     // config options
    string                  _cname;     // config filename

    ParamMap      _sys_opts;  // db-instance-independent options  
    ParamMap      _sm_opts;   // db-instance-specific options that are passed to Shore
    ParamMap      _dev_opts;  // db-instance-specific options    

    // Processor info
    uint_t _max_cpu_count;    // hard limit
    uint_t _active_cpu_count; // soft limit


    // List of worker threads
    WorkerPool      _workers;    
    uint            _worker_cnt;         

    // Scaling factors
    //
    // @note: The scaling factors of any environment is an integer value 
    //        So we are putting them on shore_env
    // 
    // In various environments: 
    //
    // TM1:  SF=1 --> 15B   (10K Subscribers)
    // TPCB: SF=1 --> 20MB  (1 Branch)
    // TPCC: SF=1 --> 130MB (1 Warehouse)
    //       
    int             _scaling_factor; 
    pthread_mutex_t _scaling_mutex;
    int             _queried_factor;
    pthread_mutex_t _queried_mutex;


    // Stats
    env_stats_t        _env_stats; 

    // Measurement state
    MeasurementState volatile _measure;

    // system name
    string          _sysname;

    // Helper functions
    void usage(option_group_t& options);
    void readconfig(const string conf_file);


    // Storage manager access functions
    int  configure_sm();
    int  start_sm();
    int  close_sm();
    void gatherstats_sm();
    
public:

    ShoreEnv(string confname);
    virtual ~ShoreEnv();


    // DB INTERFACE

    virtual int conf();
    virtual int set(envVarMap* vars) { return(0); /* do nothing */ };
    virtual int init();
    virtual int post_init() { return (0); /* do nothing */ }; // Should return >0 on error
    virtual int open() { return(0); /* do nothing */ };
    virtual int close();
    virtual int start();
    virtual int stop();
    virtual int restart();
    virtual int pause() { return(0); /* do nothing */ };
    virtual int resume() { return(0); /* do nothing */ };    
    virtual int newrun()=0;
    virtual int statistics();
    virtual int dump();
    virtual int info()=0;    


    // Loads the database schema after the config file is read, and before the storage
    // manager is started.
    // Should return >0 on error
    virtual int load_schema()=0; 

    virtual w_rc_t warmup()=0;
    virtual w_rc_t loaddata()=0;
    virtual w_rc_t check_consistency()=0;
       
    // inline access methods
    inline ss_m* db() { return(_pssm); }
    inline vid_t* vid() { return(_pvid); }

    bool is_initialized();
    bool is_loaded();
    
    void set_measure(const MeasurementState aMeasurementState) {
        //assert (aMeasurementState != MST_UNDEF);
        atomic_swap(&_measure, aMeasurementState);
    }
    inline MeasurementState get_measure() { return (*&_measure); }


    pthread_mutex_t* get_init_mutex() { return (&_init_mutex); }
    pthread_mutex_t* get_vol_mutex() { return (&_vol_mutex); }
    pthread_mutex_t* get_load_mutex() { return (&_load_mutex); }
    bool get_init_no_cs() { return (_initialized); }
    bool get_loaded_no_cs() { return (_loaded); }
    void set_init_no_cs(const bool b_is_init) { _initialized = b_is_init; }
    void set_loaded_no_cs(const bool b_is_loaded) { _loaded = b_is_loaded; }

    // CPU count functions
    void print_cpus() const;
    int get_max_cpu_count() const;
    int get_active_cpu_count() const;
    void set_active_cpu_count(const uint_t actcpucnt);
    // disabled - max_count can be set only on conf
    //    void set_max_cpu_count(const int maxcpucnt); 

    // --- scaling and querying factor --- //
    void set_qf(const uint aQF);
    int get_qf() const;
    void set_sf(const uint aSF);
    int get_sf() const;
    int upd_sf();
    void print_sf() const;

    // Environment workers
    uint upd_worker_cnt();
    trx_worker_t* worker(const uint idx);        

    // Request atomic trash stack
    RequestStack _request_pool;

    // For thread-local stats
    virtual void env_thread_init()=0;
    virtual void env_thread_fini()=0;   

    // Fake io delay interface
    int disable_fake_disk_latency();
    int enable_fake_disk_latency(const int adelay);

    // Takes a checkpoint (forces dirty pages)
    int checkpoint();

    string sysname() { return (_sysname); }
    env_stats_t* get_env_stats() { return (&_env_stats); }

    // Throughput printing
    virtual void print_throughput(const int iQueriedSF, 
                                  const int iSpread, 
                                  const int iNumOfThreads,
                                  const double delay,
                                  const ulong_t mioch,
                                  const double avgcpuusage)=0;

    virtual void reset_stats()=0;

    // Run one transaction
    virtual w_rc_t run_one_xct(Request* prequest)=0;



    // Control whether asynchronous commit will be used
    inline bool isAsynchCommit() const { return (_asynch_commit); }
    void setAsynchCommit(const bool bAsynch);

    
#ifdef CFG_SLI
public:
    bool isSLIEnabled() const { return (_bUseSLI); }
    void setSLIEnabled(const bool bUseSLI) { _bUseSLI = bUseSLI; }
protected:
    bool _bUseSLI;    
#endif    

#ifdef CFG_ELR
public:
    bool isELREnabled() const { return (_bUseELR); }
    void setELREnabled(const bool bUseELR) { _bUseELR = bUseELR; }
protected:
    bool _bUseELR;
#endif


#ifdef CFG_FLUSHER
    guard<flusher_t>   _base_flusher;
    virtual int        _start_flusher();
    virtual int        _stop_flusher();
    void               to_base_flusher(Request* ar);
#endif

protected:
   
    // returns 0 on success
    int _set_sys_params();

    bool _asynch_commit;    

}; // EOF ShoreEnv



EXIT_NAMESPACE(shore);

#endif /* __SHORE_ENV_H */

