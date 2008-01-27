/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_env.cpp
 *
 *  @brief Declaration of the Shore TPC-C environment
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/shore_tpcc_env.h"


using namespace tpcc;


/** Exported variables */

ShoreTPCCEnv* shore_env;


/** Exported functions */


/********
 ******** Caution: The functions below should be invoked inside
 ********          the context of a smthread
 ********/


/** @fn     init
 *
 *  @brief  Initializes the shore environment. Reads configuration
 *  opens database and device.
 *
 *  @return 0 on success, non-zero otherwise
 */

int ShoreTPCCEnv::init() 
{
    CRITICAL_SECTION(cs,_init_mutex);
    if (_initialized) {
        TRACE( TRACE_ALWAYS, "Already initialized\n");
        return (1);
    }

    // Read configuration options
    readconfig(_cname);
    //    printconfig();
    
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
    CRITICAL_SECTION(cs, _init_mutex);
    if (!_initialized) {
        cerr << "Environment not initialized..." << endl;
        return (1);
    }

    close_sm();
    _initialized = false;

    // If reached this point the Shore environment is closed
    return (0);
}



/** @fn    statistics
 *
 *  @brief Prints "du"-like statistics for the SM
 */

int ShoreTPCCEnv::statistics() 
{
    CRITICAL_SECTION(cs, _init_mutex);
    if (!_initialized) {
        cerr << "Environment not initialized..." << endl;
        return (1);
    }

    gatherstats_sm();

    // If reached this point the Shore environment is closed
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
    cout << "Closing Shore storage manager... " << endl;

    if (!_pssm) {
        cerr << "sm already closed..." << endl;
        return (1);
    }

//     /** Begin trx **/ 
//     W_COERCE(_pssm->begin_xct());
//     cout << "Destroying volume..." << endl;
//     W_COERCE(_pssm->destroy_vol(_lvid));
    
//     cout << "Dismounting all devices... " << endl;
//     W_COERCE(_pssm->dismount_all());

//     cout << "Committing... " << endl ;
//     W_COERCE(_pssm->commit_xct());
//     /** Commit trx **/    

    
    /** @note According to 
     *  http://www.cs.wisc.edu/shore/1.0/ssmapi/node3.html
     *  destroying the ss_m instance causes the SSM to shutdown
     */
    delete (_pssm);

    // If we reached this point the sm is closed
    return (0);
}


/** @fn    gatherstats_sm
 *
 *  @brief Collects statistics from the sm
 */ 

void ShoreTPCCEnv::gatherstats_sm()
{
    sm_du_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    
    //    ss_m::gather_stats(stats, false);
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

    int szOpt = _sm_opts.size();
    assert (szOpt > 1);
    const char* myOpts[(2*_NUM_DEF_SM_OPTIONS) + 1];
    int i=0;
    // iterate over all options
    myOpts[i++] = (char*)"fake";
    for (map<string,string>::iterator sm_iter = _sm_opts.begin();
         sm_iter != _sm_opts.end(); sm_iter++)
        {            
            myOpts[i++] = sm_iter->first.c_str();
            myOpts[i++] = sm_iter->second.c_str();
        }

    w_ostrstream err;
    int numOpts = (2*szOpt)+1;
    w_rc_t rc = _popts->parse_command_line(myOpts, numOpts, 2, &err);

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
 *  @brief  Start Shore storage manager. 
 *  - Formats and mounts the device
 *  - Creates the volume in the device 
 *  (Shore limitation: only 1 volume per device)
 *
 *  @return 0 on success, non-zero otherwise
 */ 

int ShoreTPCCEnv::start_sm()
{
    cout << "Starting Shore..." << endl;
    if (_pssm) {
        cerr << "Shore already started...\n";
        return (1);
    }

    _pssm = new ss_m();

    // format and mount the database...

    // Get the configuration from the config file
    char const* device =  _dev_opts[_DEF_DEV_OPTIONS[0][0]].c_str();
    int quota = atoi(_dev_opts[_DEF_DEV_OPTIONS[1][0]].c_str());
    bool clobber = atoi(_dev_opts[_DEF_DEV_OPTIONS[2][0]].c_str());

    assert(strlen(device)>0);
    assert(quota>0);

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
        shoreConfig.readInto(tmp, _DEF_SM_OPTIONS[i][1], _DEF_SM_OPTIONS[i][2]);
        _sm_opts[_DEF_SM_OPTIONS[i][0]] = tmp;

        //        cout << _DEF_SM_OPTIONS[i][0] << " " << _DEF_SM_OPTIONS[i][1] << " " << _DEF_SM_OPTIONS[i][2];
        //        cout << ": (" << tmp << ")" << endl;
    }    

    // Parse DEVICE parameters
    for (int i = 0; i < _NUM_DEF_DEV_OPTIONS; i++) {
        shoreConfig.readInto(tmp, _DEF_DEV_OPTIONS[i][0], _DEF_DEV_OPTIONS[i][1]);
        _dev_opts[_DEF_DEV_OPTIONS[i][0]] = tmp;

        //        cout << _DEF_DEV_OPTIONS[i][0] << " " << _DEF_DEV_OPTIONS[i][1];
        //        cout << ": (" << tmp << ")" << endl;
    }
}


/** @fn    printconfig
 *
 *  @brief Prints configuration
 */ 

void ShoreTPCCEnv::printconfig() {
    cout << "Printing configuration " << endl;
    
    // Print storage manager options
    map<string,string>::iterator sm_iter;
    cout << "** SM options" << endl;
    for ( sm_iter = _sm_opts.begin(); sm_iter != _sm_opts.end(); sm_iter++)
        cout << sm_iter->first << " \t| " << sm_iter->second << endl;

    // Print device options
    map<string,string>::iterator dev_iter;
    cout << "** Device options" << endl;
    for ( dev_iter = _dev_opts.begin(); dev_iter != _dev_opts.end(); dev_iter++)
        cout << dev_iter->first << " \t| " << dev_iter->second << endl;    
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
