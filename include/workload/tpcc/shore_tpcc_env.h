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

#include "util/guard.h"
#include "util/c_str.h"
#include "util/namespace.h"

#include "stages/tpcc/common/tpcc_scaling_factor.h"
#include "stages/tpcc/common/tpcc_struct.h"

#include <map>


ENTER_NAMESPACE(tpcc);

using std::map;


/* constants */

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

static char const* storage_manager_opts[] = {
    "fake-command-line",
    "-sm_bufpoolsize", "102400", // in kB
    //	"-sm_logging", "no", // temporary
    "-sm_logdir", "log",
    "-sm_logsize", "102400", // in kB
    "-sm_logbufsize", "10240", // in kB
    "-sm_diskrw", "/export/home/ipandis/DEV/shore-lomond/installed/bin/diskrw",
    "-sm_errlog", "info", // one of {none emerg fatal alert internal error warning info debug}
};



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

    // TODO: (ip) ADD ALL TABLES!!

    static const int SHORE_PAYMENT_TABLES = 4;

private:       
    /** Private variables */
    bool _initialized; 
    pthread_mutex_t _env_mutex;

    ss_m* _pssm;                  // database handle
    pthread_mutex_t _vol_mutex;   // volume mutex

    unsigned _vol_cnt;
    devid_t _devid;
    lvid_t _lvid;
    vid_t* _pvid;

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
    ShoreTPCCEnv() {        
        pthread_mutex_init(&_env_mutex, NULL);
        pthread_mutex_init(&_vol_mutex, NULL);

	_initialized = false;
        _popts = new option_group_t(1);
        _pvid = new vid_t(1);
    }

    ~ShoreTPCCEnv() {         
        pthread_mutex_destroy(&_vol_mutex);
        pthread_mutex_destroy(&_env_mutex);
    }

    /** Public methods */    
    int init();
    int close();
    int loaddata(c_str loadDir);  

    inline ss_m* get_db_hd() { return(_pssm); }
    inline vid_t* get_db_vid() { return(_pvid); }
    inline pthread_mutex_t* get_vol_mutex() { return(&_vol_mutex); }
    inline bool is_initialized() const { return _initialized; }

}; // EOF ShoreTPCCEnv


EXIT_NAMESPACE(tpcc);


#endif
