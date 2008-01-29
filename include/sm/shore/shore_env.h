/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_env.h
 *
 *  @brief Definition of a Shore environment (database)
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_ENV_H
#define __SHORE_ENV_H

#include "sm_vas.h"
#include "util.h"

#include <map>


ENTER_NAMESPACE(shore);

using std::map;



/******** Constants ********/


#define SHORE_DEFAULT_CONF_FILE "shore.conf"

static const string SHORE_DEF_SM_OPTIONS[][3]  = {
    { "-sm_bufpoolsize", "bufpoolsize", "102400" },
    { "-sm_logging", "logging", "yes" },
    { "-sm_logdir", "logdir", "log" },
    { "-sm_logsize", "logsize", "102400" },
    { "-sm_logbufsize", "logbufsize", "10240" },
    { "-sm_diskrw", "diskrw", "/export/home/ipandis/DEV/shore-lomond/installed/bin/diskrw" },
    { "-sm_errlog", "errlog", "info" }
};

static const int    SHORE_NUM_DEF_SM_OPTIONS   = 7;

static const string SHORE_DEF_DEV_OPTIONS[][2] = {
    { "device", "tbl_tpcc/shore" },
    { "devicequota", "102400" },
    { "clobberdev", "1" },
};

static const int    SHORE_NUM_DEF_DEV_OPTIONS  = 3;



/******************************************************************** 
 * 
 *  ShoreEnv
 *  
 *  Shore database abstraction. Among others it configures, starts 
 *  and closes the Shore database 
 *
 ********************************************************************/

class ShoreEnv 
{
private:       

    ss_m* _pssm;                    // database handle

    // Status variables
    bool            _initialized; 
    pthread_mutex_t _init_mutex;
    bool            _loaded;
    pthread_mutex_t _load_mutex;

    // Device and volume. There is a single volume per device. 
    // The whole environment resides in a single volume.
    devid_t             _devid;       // device id
    vid_t*              _pvid;        // volume id
    stid_t              _root_iid;    // root id of the volume
    pthread_mutex_t     _vol_mutex;   // volume mutex
    lvid_t              _lvid;        // logical volume id (unnecessary, using physical ids)
    unsigned int        _vol_cnt;     // volume count (unnecessary, always 1)

    // Configuration variables
    option_group_t*     _popts;       // config options
    string              _cname;       // config filename
    map<string,string>  _sm_opts;     // map of options for the sm
    map<string,string>  _dev_opts;    // map of options for the device    

    // Helper functions
    void usage(option_group_t& options);
    void readconfig(const string conf_file);
    void printconfig();
    void dump();

    // Storage manager access functions
    int configure_sm();
    int start_sm();
    int close_sm();
    void gatherstats_sm();
    
public:

    /** Construction  */
    ShoreEnv(string confname) :
        _cname(confname), _init_mutex(thread_mutex_create()),
        _vol_mutex(thread_mutex_create()), _load_mutex(thread_mutex_create()),
        _initialized(false), _loaded(false)                   
    {
        _popts = new option_group_t(1);
        _pvid = new vid_t(1);
    }

    virtual ~ShoreEnv() {         
        pthread_mutex_destroy(&_init_mutex);
        pthread_mutex_destroy(&_vol_mutex);
        pthread_mutex_destroy(&_load_mutex);

        if (_popts) {
            delete (_popts);
            _popts = NULL;
        }

        if (_pvid) {
            delete (_pvid);
            _pvid = NULL;
        }
        
        if (_pssm) {
            delete (_pssm);
            _pssm = NULL;
        }
    }

    /** Public methods */    
    virtual int init();
    virtual int close();
    virtual int loaddata()=0;
    virtual int statistics();


    // inline access methods
    inline ss_m* db() { return(_pssm); }
    inline vid_t* vid() { return(_pvid); }

    inline pthread_mutex_t* get_vol_mutex() { return(&_vol_mutex); }
    inline pthread_mutex_t* get_load_mutex() { return(&_load_mutex); }
    inline bool is_loaded_no_cs() { return (_loaded); }
    inline void set_loaded_no_cs(bool b) { _loaded = b; } 

    inline bool is_initialized() { return (_initialized); }
    inline bool is_loaded() { return (_loaded); }

}; // EOF ShoreEnv


EXIT_NAMESPACE(shore);


#endif /* __SHORE_ENV_H */

