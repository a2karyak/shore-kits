/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_env.cpp
 *
 *  @brief Declaration of the Shore TPC-C environment
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "workload/workload.h"
#include "workload/tpcc/shore_tpcc_env.h"


using namespace workload;
using namespace tpcc;


/** Exported functions */


/** @fn     init
 *
 *  @brief  Initializes the shore environment. Reads configuration
 *  opens database and device.
 *
 *  @return 0 on success, non-zero otherwise
 */

int ShoreTPCCEnv::init() 
{
    CRITICAL_SECTION(cs,_env_mutex);
    if (_initialized) {
        TRACE( TRACE_ALWAYS, "Already initialized\n");
        return (1);
    }

    // Read configuration options
    readconfig(_cname);
    printconfig();
    
    // Start the storage manager
    if (configure_sm()) {
        cerr << "Error configuring Shore\n";
        return (1);
    }

    if (start_sm()) {
        cerr << "Error starting Shore\n";
        return (2);
    }

    // if we reached this point the environment is properly initialized
    _initialized = true;
    TRACE( TRACE_ALWAYS, "ShoreEnv initialized\n");

    return (0);
}


/** @fn    close
 *
 *  @brief Closes the Shore environment
 */

int ShoreTPCCEnv::close() 
{
    CRITICAL_SECTION(cs,_env_mutex);
    if (!_initialized) {
        cerr << "Environment not initialized..." << endl;
        return (1);
    }

    close_sm();
    _initialized = false;

    // If reached this point the Shore environment is closed
    return (0);
}


/** @fn loaddata
 *
 *  @brief Loads the TPC-C data from a specified location
 */

int ShoreTPCCEnv::loaddata(c_str loadDir) 
{
    //    TRACE( TRACE_DEBUG, "Getting data from (%s)\n", loadDir.data());

    // Start measuring loading time
    time_t tstart = time(NULL);

    assert (false); // TO DO

    time_t tstop = time(NULL);

    //    TRACE( TRACE_ALWAYS, "Loading finished in (%d) secs\n", tstop - tstart);
    cout << "Loading finished in " << (tstop - tstart) << " secs." << endl;
    _initialized = true;
    return (0);
}



/** Storage manager functions */


/** @fn     close_sm
 *
 *  @brief  Closes the storage manager
 *
 *  @return 0 on sucess, non-zero otherwise
 */

int ShoreTPCCEnv::close_sm() 
{
    cout << "Closing Shore storage manager...\n";

    if (!_pssm) {
        cerr << "sm already closed..." << endl;
        return (1);
    }
    
    /** @note According to 
     *  http://www.cs.wisc.edu/shore/1.0/ssmapi/node3.html
     *  destroying the ss_m instance causes the SSM to shutdown
     */
    delete _pssm;

    // If we reached this point the sm is closed
    return (0);
}


/** @fn    gatherstats_sm
 *
 *  @brief Collects statistics from the sm
 */ 

void ShoreTPCCEnv::gatherstats_sm()
{
    sm_stats_info_t stats;
    memset(&stats, 0, sizeof(stats));
    ss_m::gather_stats(stats, false);
    cout << stats << endl;
}



/** @fn     configure_sm
 *
 *  @brief  Configure Shore environment
 *
 *  @return 0 on success, non-zero otherwise
 */ 

int ShoreTPCCEnv::configure_sm()
{
    cout << "Configuring Shore..." << endl;
    
    // have the SSM add its options to the group
    W_COERCE(ss_m::setup_options(_popts));

    w_ostrstream err;
    int opts_count = sizeof(storage_manager_opts)/sizeof(char const*);
    w_rc_t rc = _popts->parse_command_line(storage_manager_opts, 
                                           opts_count, 2, &err);
    err << ends;

    // configure
    if (rc) {
	cerr << "Error configuring Shore: " << endl;
	cerr << "\t" << w_error_t::error_string(rc.err_num()) << endl;
	cerr << "\t" << err.c_str() << endl;
	usage(*_popts);
        return (1);
    }

    // verify 
    w_reset_strstream(err);
    rc = _popts->check_required(&err);
    if (rc) {
	cerr << "These required options are not set:" << endl;
	cerr << err.c_str() << endl;
	usage(*_popts);
        return (2);
    }

    // If we reached this point the sm is configured correctly
    return (0);
}


/** @fn     start_sm
 *
 *  @brief  Start Shore storage manager and Format & Mount the databae
 *
 *  @return 0 on success, non-zero otherwise
 */ 

int ShoreTPCCEnv::start_sm()
{
    cout << "Starting Shore..." << endl;
    _pssm = new ss_m();

    // format and mount the database...

    // TODO: make these command-line options!
    bool clobber = true;
    char const* device = "tbl_tpcc/shore";
    int quota = 100*1024; // 100 MB

    if(clobber) {
	cout << "Formatting a new device ``" << device
	     << "'' with a " << quota << "kB quota" << endl;

	// create and mount device
	// http://www.cs.wisc.edu/shore/1.0/man/device.ssm.html
	W_COERCE(_pssm->format_dev(device, quota, true));
        cout << "Formatting completed..." << endl;
    }

    // mount it...
    W_COERCE(_pssm->mount_dev(device, _vol_cnt, _devid));
    
    // Using the physical ID interface

    // create volume 
    // (only one per device supported, so this is kind of silly)
    // see http://www.cs.wisc.edu/shore/1.0/man/volume.ssm.html
    W_COERCE(_pssm->generate_new_lvid(_lvid));
    W_COERCE(_pssm->create_vol(device, _lvid, quota, false, *_pvid));

    // If we reached this point the sm has started correctly
    return (0);
}



/** Helper functions */


/** @fn    usage
 *
 *  @brief Prints shore options
 */ 

void ShoreTPCCEnv::usage(option_group_t& options)
{
    cerr << "Valid Shore options are: " << endl;
    options.print_usage(true, cerr);
}


/** @fn    readconfig
 *
 *  @brief Reads configuration file
 */ 

void ShoreTPCCEnv::readconfig(string conf_file)
{
    cout << "Reading configuration file " << conf_file << endl;
    
    ConfigFile shoreConfig(conf_file);

    string tmp;

    // Parse SM parameters
    for (int i = 0; i < _NUM_DEF_SM_OPTIONS; i++) {
        cout << _DEF_SM_OPTIONS[i][0] << " " << _DEF_SM_OPTIONS[i][1] << " " << _DEF_SM_OPTIONS[i][2] << endl;
        shoreConfig.readInto(tmp, _DEF_SM_OPTIONS[i][1], _DEF_SM_OPTIONS[i][2]);
        cout << tmp << endl;
        _sm_opts[_DEF_SM_OPTIONS[i][1]] = tmp;
    }    

    // Parse DEVICE parameters
    for (int i = 0; i < _NUM_DEF_DEV_OPTIONS; i++) {
        cout << _DEF_DEV_OPTIONS[i][0] << " " << _DEF_DEV_OPTIONS[i][1] << endl;
        shoreConfig.readInto(tmp, _DEF_DEV_OPTIONS[i][0], _DEF_DEV_OPTIONS[i][1]);
        cout << tmp << endl;
        _sm_opts[_DEF_DEV_OPTIONS[i][0]] = tmp;
    }
}


/** @fn    printconfig
 *
 *  @brief Prints configuration
 */ 

void ShoreTPCCEnv::printconfig() {
    cout << "Printing configuration " << endl;
    map<string,string>::iterator iter;
    
    // Print storage manager options
    cout << "** SM options" << endl;
    for ( iter = _sm_opts.begin(); iter != _sm_opts.end(); iter++)
            cout << "option: " << iter->first << " \tvalue: " << iter->second << endl;

    // Print device options
    cout << "** Device options" << endl;
    for ( iter = _dev_opts.begin(); iter != _dev_opts.end(); iter++)
            cout << "option: " << iter->first << " \tvalue: " << iter->second << endl;    

    cout << "End of configuration" <endl;
}


/** @fn dump
 *
 *  @brief Dumps the data
 */

void ShoreTPCCEnv::dump() {

    assert (false); // TO DO
    assert (false); // TO DO: Remove TRACE

    TRACE( TRACE_DEBUG, "~~~~~~~~~~~~~~~~~~~~~\n");    
    TRACE( TRACE_DEBUG, "Dumping Shore Data\n");

    TRACE( TRACE_DEBUG, "~~~~~~~~~~~~~~~~~~~~~\n");    
}
