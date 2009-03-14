/* -*- mode:C++; c-basic-offset:4 -*- */

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
    { "shore-sli_enable", "0" },
};

const int    SHORE_NUM_SYS_OPTIONS  = 6;


// SHORE_SYS_SM_OPTIONS: 
// Those options are appended as parameters when starting Shore
// Those are the database-independent
const string SHORE_SYS_SM_OPTIONS[][3]  = {
    { "-sm_logging", "shore-logging", "yes" },
    { "-sm_diskrw", "shore-diskrw", "diskrw" },
    { "-sm_errlog", "shore-errlog", "info" },
    { "-sm_num_page_writers", "shore-pagecleaners", "16" }
};

const int    SHORE_NUM_SYS_SM_OPTIONS   = 4;


// SHORE_DB_SM_OPTIONS: 
// Those options are appended as parameters when starting Shore
// Thore are the database-instance-specific
const string SHORE_DB_SM_OPTIONS[][3]  = {
    { "-sm_bufpoolsize", "bufpoolsize", "0" },
    { "-sm_logdir", "logdir", "log" },
    { "-sm_logsize", "logsize", "0" },
    { "-sm_logbufsize", "logbufsize", "0" },
};

const int    SHORE_NUM_DB_SM_OPTIONS   = 4;


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





#define DECLARE_TRX(trx) \
    w_rc_t run_##trx(const int xct_id, trx_result_tuple_t& atrt, trx##_input_t& in);       \
    w_rc_t run_##trx(const int xct_id, trx_result_tuple_t& atrt, const int specificID);    \
    w_rc_t xct_##trx(const int xct_id, trx_result_tuple_t& atrt, trx##_input_t& in);       \
    void   _inc_##trx##_att();  \
    void   _inc_##trx##_failed(); \
    void   _inc_##trx##_dld()


#define DECLARE_TABLE(table,manimpl,abbrv)                              \
    guard<manimpl>   _p##abbrv##_man;                                   \
    inline manimpl*  abbrv##_man() { return (_p##abbrv##_man); }        \
    guard<table>     _p##abbrv##_desc;                                  \
    inline table*    abbrv##_desc() { return (_p##abbrv##_desc.get()); }


#define DEFINE_RUN_WITH_INPUT_TRX_WRAPPER(cname,trx)                   \
    w_rc_t cname::run_##trx(const int xct_id, trx_result_tuple_t& atrt, trx##_input_t& in) { \
        TRACE( TRACE_TRX_FLOW, "%d. %s ...\n", xct_id, #trx);           \
        ++my_stats.attempted.##trx;                                     \
        w_rc_t e = xct_##trx(xct_id, atrt, in);                         \
        if (e.is_error()) {                                             \
            if (e.err_num() != smlevel_0::eDEADLOCK)                    \
                ++my_stats.failed.##trx;                                \
            else ++my_stats.deadlocked.##trx##;                         \
            TRACE( TRACE_TRX_FLOW, "Xct (%d) aborted [0x%x]\n", xct_id, e.err_num()); \
            w_rc_t e2 = _pssm->abort_xct();                           \
            if(e2.is_error()) TRACE( TRACE_ALWAYS, "Xct (%d) abort failed [0x%x]\n", xct_id, e2.err_num()); \
            if (atrt.get_notify()) atrt.get_notify()->signal();       \
            if (_measure!=MST_MEASURE) return (e);                    \
            _env_stats.inc_trx_att();                                 \
            return (e); }                                             \
        TRACE( TRACE_TRX_FLOW, "Xct (%d) completed\n", xct_id);       \
        if (atrt.get_notify()) atrt.get_notify()->signal();           \
        if (_measure!=MST_MEASURE) return (RCOK);                     \
        _env_stats.inc_trx_com();                                     \
        return (RCOK); }

#define DEFINE_RUN_WITHOUT_INPUT_TRX_WRAPPER(cname,trx)               \
    w_rc_t cname::run_##trx(const int xct_id, trx_result_tuple_t& atrt, const int specificID) { \
        trx##_input_t in = create_##trx##_input(_scaling_factor, specificID);                   \
        return (run_##trx(xct_id, atrt, in)); }


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
    int        _ntrx_att;
    int        _ntrx_com;
    tatas_lock _ntrx_lock;

    env_stats_t() 
        : _ntrx_att(0), _ntrx_com(0)
    { }

    ~env_stats_t() { }

    void print_env_stats() const;

    int inc_trx_att() {
        CRITICAL_SECTION(g_att_cs, _ntrx_lock);
        return (++_ntrx_att);
    }

    int inc_trx_com() {
        CRITICAL_SECTION(g_com_cs, _ntrx_lock);
        ++_ntrx_att;
        return (++_ntrx_com);
    }

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

    eDBControl _dbc;
    tatas_lock _dbc_lock;

public:

    db_iface() 
        : _dbc(DBC_UNDEF)
    { }
    virtual ~db_iface() { }

    // Access methods

    const eDBControl dbc() { 
        CRITICAL_SECTION(dbc_cs, _dbc_lock);
        return(_dbc); 
    }
    
    void set_dbc(const eDBControl adbc) {
        assert (adbc!=DBC_UNDEF);
        CRITICAL_SECTION(dbc_cs, _dbc_lock);
        _dbc = adbc;
    }
    

    // DB INTERFACE     

    virtual const int conf()=0;    
    virtual const int set(envVarMap* vars)=0;    
    virtual const int load_schema()=0;
    virtual const int init()=0;
    virtual const int open()=0;
    virtual const int close()=0;
    virtual const int start()=0;
    virtual const int stop()=0;
    virtual const int restart()=0;
    virtual const int pause()=0;
    virtual const int resume()=0;    
    virtual const int newrun()=0;    
    virtual const int statistics()=0;    
    virtual const int dump()=0;    
    virtual const int info()=0;    
    
}; // EOF: db_iface






/******** Exported variables ********/

class base_worker_t;

class ShoreEnv;
extern ShoreEnv* _g_shore_env;




/******************************************************************** 
 *
 * @enum  MeasurementState
 *
 * @brief Possible states of a measurement in the Shore Enviroment
 *
 ********************************************************************/

enum MeasurementState { MST_UNDEF, MST_WARMUP, MST_MEASURE, MST_DONE };


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
    int                _max_cpu_count;    // hard limit
    int                _active_cpu_count; // soft limit
    tatas_lock         _cpu_count_lock;

    // Stats
    env_stats_t        _env_stats; 

    // Measurement state
    MeasurementState volatile _measure;
    tatas_lock                _measure_lock;

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

    // Construction  //
    ShoreEnv(string confname) 
        : db_iface(),
          _pssm(NULL), 
          _initialized(false), _init_mutex(thread_mutex_create()),
          _loaded(false), _load_mutex(thread_mutex_create()),
          _statmap_mutex(thread_mutex_create()),
          _last_stats_mutex(thread_mutex_create()),
          _vol_mutex(thread_mutex_create()), _cname(confname),
          _measure(MST_UNDEF),
          _max_cpu_count(SHORE_DEF_NUM_OF_CORES), 
          _active_cpu_count(SHORE_DEF_NUM_OF_CORES)
    {
        _popts = new option_group_t(1);
        _pvid = new vid_t(1);
    }

    virtual ~ShoreEnv() 
    {         
        if (dbc()!=DBC_STOPPED) stop();
        pthread_mutex_destroy(&_init_mutex);
        pthread_mutex_destroy(&_statmap_mutex);
        pthread_mutex_destroy(&_last_stats_mutex);
        pthread_mutex_destroy(&_load_mutex);
        pthread_mutex_destroy(&_vol_mutex);
    }


    // DB INTERFACE

    virtual const int conf();
    virtual const int set(envVarMap* vars) { return(0); /* do nothing */ };
    virtual const int init();
    virtual const int post_init() { return (0); /* do nothing */ }; // Should return >0 on error
    virtual const int open() { return(0); /* do nothing */ };
    virtual const int close();
    virtual const int start() { return(0); /* do nothing */ };
    virtual const int stop() { return(0); /* do nothing */ };
    virtual const int restart();
    virtual const int pause() { return(0); /* do nothing */ };
    virtual const int resume() { return(0); /* do nothing */ };    
    virtual const int newrun() { return(0); /* do nothing */ };
    virtual const int statistics();
    virtual const int dump();
    virtual const int info()=0;    

    // Loads the database schema after the config file is read, and before the storage
    // manager is started.
    // Should return >0 on error
    virtual const int load_schema()=0; 

    virtual w_rc_t warmup()=0;
    virtual w_rc_t loaddata()=0;
    virtual w_rc_t check_consistency()=0;
       
    // inline access methods
    inline ss_m* db() { return(_pssm); }
    inline vid_t* vid() { return(_pvid); }


    inline bool is_initialized() { 
        CRITICAL_SECTION(cs, _init_mutex); 
        return (_initialized); 
    }

    inline bool is_loaded() { 
        CRITICAL_SECTION(cs, _load_mutex);
        return (_loaded); 
    }

    
    void set_measure(const MeasurementState aMeasurementState) {
        assert (aMeasurementState != MST_UNDEF);
        CRITICAL_SECTION(measure_cs, _measure_lock);
        _measure = aMeasurementState;
    }
    inline const MeasurementState get_measure() { return (_measure); }


    inline pthread_mutex_t* get_init_mutex() { return (&_init_mutex); }
    inline pthread_mutex_t* get_vol_mutex() { return (&_vol_mutex); }
    inline pthread_mutex_t* get_load_mutex() { return (&_load_mutex); }
    inline bool get_init_no_cs() { return (_initialized); }
    inline bool get_loaded_no_cs() { return (_loaded); }
    inline void set_init_no_cs(const bool b_is_init) { _initialized = b_is_init; }
    inline void set_loaded_no_cs(const bool b_is_loaded) { _loaded = b_is_loaded; }

    // cpu count functions
    void print_cpus() const;
    inline const int get_max_cpu_count() const { return (_max_cpu_count); }
    inline const int get_active_cpu_count() const { return (_active_cpu_count); }
    void set_active_cpu_count(const int actcpucnt);
    // disabled - max_count can be set only on conf
    //    void set_max_cpu_count(const int maxcpucnt); 


    // fake io delay interface
    const int disable_fake_disk_latency();
    const int enable_fake_disk_latency(const int adelay);
    int set_sli_enabled(bool enable);


    // does a log flush
    const int checkpoint();


    inline string sysname() { return (_sysname); }
    env_stats_t* get_env_stats() { return (&_env_stats); }

    // for thread-local stats
    virtual void env_thread_init(base_worker_t* aworker)=0;
    virtual void env_thread_fini(base_worker_t* aworker)=0;   

    // throughput printing
    virtual void print_throughput(const int iQueriedSF, 
                                  const int iSpread, 
                                  const int iNumOfThreads,
                                  const double delay)=0;

    virtual void reset_stats()=0;
    

protected:
   
    // returns 0 on success
    const int _set_sys_params();


}; // EOF ShoreEnv


EXIT_NAMESPACE(shore);


#endif /* __SHORE_ENV_H */

