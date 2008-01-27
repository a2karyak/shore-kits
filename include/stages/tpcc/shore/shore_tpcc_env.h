/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_env.h
 *
 *  @brief Definition of the Shore TPC-C environment
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_ENV_H
#define __SHORE_TPCC_ENV_H

#include "sm_vas.h"
#include "util.h"
#include "stages/tpcc/common/tpcc_scaling_factor.h"
#include "stages/tpcc/common/tpcc_struct.h"
#include <map>


ENTER_NAMESPACE(tpcc);

using std::map;



/******** Exported variables ********/

class ShoreTPCCEnv;
extern ShoreTPCCEnv* shore_env;



////////////////////////////////////////////////////////////////////////

/* constants */

#define SHORE_DEFAULT_CONF_FILE    "shore.conf"

#define SHORE_TPCC_DATA_DIR        "tpcc_sf"

#define SHORE_TPCC_DATA_WAREHOUSE  "WAREHOUSE.dat"
#define SHORE_TPCC_DATA_DISTRICT   "DISTRICT.dat"
#define SHORE_TPCC_DATA_CUSTOMER   "CUSTOMER.dat"
#define SHORE_TPCC_DATA_HISTORY    "HISTORY.dat"

#define SHORE_TPCC_DATA_ITEM       "ITEM.dat"
#define SHORE_TPCC_DATA_NEW_ORDER  "NEW_ORDER.dat"
#define SHORE_TPCC_DATA_ORDER      "ORDER.dat"
#define SHORE_TPCC_DATA_ORDERLINE  "ORDERLINE.dat"
#define SHORE_TPCC_DATA_STOCK      "STOCK.dat"

static const string _DEF_SM_OPTIONS[][3] = {
    { "-sm_bufpoolsize", "bufpoolsize", "102400" },
    { "-sm_logging", "logging", "yes" },
    { "-sm_logdir", "logdir", "log" },
    { "-sm_logsize", "logsize", "102400" },
    { "-sm_logbufsize", "logbufsize", "10240" },
    { "-sm_diskrw", "diskrw", "/export/home/ipandis/DEV/shore-lomond/installed/bin/diskrw" },
    { "-sm_errlog", "errlog", "info" }
};

static const int _NUM_DEF_SM_OPTIONS = 7;

static const string _DEF_DEV_OPTIONS[][2] = {
    { "device", "tbl_tpcc/shore" },
    { "devicequota", "102400" },
    { "clobberdev", "1" },
};

static const int _NUM_DEF_DEV_OPTIONS = 3;



////////////////////////////////////////////////////////////////////////

/** @class ShoreTPCCEnv
 *  
 *  @brief Class that contains the various data structures used in the
 *  Shore TPC-C environment.
 */

class ShoreTPCCEnv 
{
public:
    /** Constants */
    static const int WAREHOUSE = 0;
    static const int DISTRICT = 1;
    static const int CUSTOMER = 2;
    static const int HISTORY = 3;

    static const int ITEM = 4;
    static const int NEW_ORDER = 5;
    static const int ORDER = 6;
    static const int ORDERLINE = 7;
    static const int STOCK = 8;

    static const int SHORE_PAYMENT_TABLES = 4;
    static const int SHORE_TPCC_TABLES = 9;

private:       

    ss_m* _pssm;                  // database handle

    // Status variables
    bool _initialized; 
    pthread_mutex_t _init_mutex;
    bool _loaded;
    pthread_mutex_t _load_mutex;

    // Device and volume. There is a single volume per device. 
    // The whole environment resides in a single volume.
    devid_t _devid;               // device id
    vid_t* _pvid;                 // volume id
    stid_t _root_iid;             // root id of the volume
    pthread_mutex_t _vol_mutex;   // volume mutex
    lvid_t _lvid;                 // logical volume id (unnecessary, using physical ids)
    unsigned _vol_cnt;            // volume count (unnecessary, always 1)

    // Configuration variables
    option_group_t* _popts;       // config options
    string _cname;                // config filename
    map<string,string> _sm_opts;  // map of options for the sm
    map<string,string> _dev_opts; // map of options for the device
        

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
    ShoreTPCCEnv(string confname) :
        _cname(confname), _init_mutex(thread_mutex_create()),
        _vol_mutex(thread_mutex_create()), _load_mutex(thread_mutex_create()),
        _initialized(false), _loaded(false)                   
    {
        _popts = new option_group_t(1);
        _pvid = new vid_t(1);
    }

    ~ShoreTPCCEnv() {         
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
    int init();
    int close();
    int loaddata();  
    int statistics();


    // inline access methods
    inline ss_m* db() { return(_pssm); }
    inline vid_t* vid() { return(_pvid); }
    inline pthread_mutex_t* get_vol_mutex() { return(&_vol_mutex); }
    inline pthread_mutex_t* get_load_mutex() { return(&_load_mutex); }
    inline bool is_loaded_no_cs() { return (_loaded); }
    inline void set_loaded_no_cs(bool b) { _loaded = b; } 

    // TODO (should guard those with a mutex?)
    inline bool is_initialized() { return (_initialized); }
    inline bool is_loaded() { return (_loaded); }

}; // EOF ShoreTPCCEnv


EXIT_NAMESPACE(tpcc);


#endif
