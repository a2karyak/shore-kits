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
};

const int    SHORE_NUM_SYS_OPTIONS  = 3;


// SHORE_SYS_SM_OPTIONS: 
// Those options are appended as parameters when starting Shore
// Those are the database-independent
const string SHORE_SYS_SM_OPTIONS[][3]  = {
    { "-sm_logging", "shore-logging", "yes" },
    { "-sm_diskrw", "shore-diskrw", "diskrw" },
    { "-sm_errlog", "shore-errlog", "info" },
    { "-sm_fake_io_delay", "shore-fakeiodelay", "0" },
    { "-sm_page_writers", "shore-pagecleaners", "1" }
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
};

const int    SHORE_NUM_DB_SM_OPTIONS   = 4;


// SHORE_DB_OPTIONS
// Database-instance-specific options
const string SHORE_DB_OPTIONS[][2] = {
    { "device", "databases/shore" },
    { "devicequota", "0" },
    { "loadatadir", SHORE_TABLE_DATA_DIR },
    { "sf", "0" },
    { "system", "invalid" }
};

const int    SHORE_NUM_DB_OPTIONS  = 5;


/****************************************************************** 
 *
 *  @struct: tpcc_stats_t
 *
 *  @brief:  TPCC Environment statistics
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
        pthread_mutex_destroy(&_vol_mutex);
        pthread_mutex_destroy(&_load_mutex);
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

    inline string sysname() { return (_sysname); }
    env_stats_t* get_env_stats() { return (&_env_stats); }

    
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

protected:
   
    // returns 0 on success
    const int _set_sys_params();


}; // EOF ShoreEnv


EXIT_NAMESPACE(shore);


#endif /* __SHORE_ENV_H */

