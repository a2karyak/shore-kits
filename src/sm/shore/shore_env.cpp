/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_env.cpp
 *
 *  @brief Implementation of a Shore environment (database)
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "sm/shore/shore_conf.h"
#include "sm/shore/shore_env.h"


using namespace shore;


/** Exported functions */



/******************************************************************** 
 *
 *  @fn:    print_env_stats
 *
 *  @brief: Prints trx statistics
 *
 ********************************************************************/

void env_stats_t::print_env_stats() 
{
    CRITICAL_SECTION(ntrx_cs, _ntrx_lock);
    TRACE( TRACE_STATISTICS, "===============================\n");
    TRACE( TRACE_STATISTICS, "Database transaction statistics\n");
    TRACE( TRACE_STATISTICS, "Attempted: %d\n", _ntrx_att);
    TRACE( TRACE_STATISTICS, "Committed: %d\n", _ntrx_com);
    TRACE( TRACE_STATISTICS, "Aborted  : %d\n", (_ntrx_att-_ntrx_com));
    TRACE( TRACE_STATISTICS, "===============================\n");
}



/********
 ******** Caution: The functions below should be invoked inside
 ********          the context of a smthread
 ********/


/*********************************************************************
 *
 *  @fn     init
 *
 *  @brief  Initialize the shore environment. Reads configuration
 *          opens database and device.
 *
 *  @return 0 on success, non-zero otherwise
 *
 *********************************************************************/

int ShoreEnv::init() 
{
    CRITICAL_SECTION(cs,_init_mutex);
    if (_initialized) {
        TRACE( TRACE_ALWAYS, "Already initialized\n");
        return (0);
    }

    // Read configuration options
    readconfig(_cname);
    //    printconfig();
    
    // Start the storage manager
    if (configure_sm()) {
        TRACE( TRACE_ALWAYS, "Error configuring Shore\n");
        return (1);
    }

    if (start_sm()) {
        TRACE( TRACE_ALWAYS, "Error starting Shore\n");
        return (2);
    }

    // if we reached this point the environment is properly initialized
    _initialized = true;
    TRACE( TRACE_DEBUG, "ShoreEnv initialized\n");

    return (0);
}


/*********************************************************************
 *
 *  @fn:     close
 *
 *  @brief:  Closes the Shore environment
 *
 *  @return: 0 on success, non-zero otherwise
 *
 *********************************************************************/

int ShoreEnv::close() 
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


/******************************************************************** 
 *
 *  @fn:    statistics
 *
 *  @brief: Prints "du"-like statistics for the SM
 *
 ********************************************************************/

int ShoreEnv::statistics() 
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


/******************************************************************** 
 *
 *  @fn:     close_sm
 *
 *  @brief:  Closes the storage manager
 *
 *  @return: 0 on sucess, non-zero otherwise
 *
 ********************************************************************/

int ShoreEnv::close_sm() 
{
    TRACE( TRACE_ALWAYS, "Closing Shore storage manager...\n");
    
    if (!_pssm) {
        TRACE( TRACE_ALWAYS, "sm already closed...\n");
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

void ShoreEnv::gatherstats_sm()
{
    sm_du_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    
    //    ss_m::gather_stats(stats, false);
    cout << stats << endl;
}



/******************************************************************** 
 *
 * @fn     configure_sm
 *
 *  @brief  Configure Shore environment
 *
 *  @return: 0 on sucess, non-zero otherwise
 *
 ********************************************************************/

int ShoreEnv::configure_sm()
{
    TRACE( TRACE_DEBUG, "Configuring Shore...\n");
    
    // have the SSM add its options to the group
    W_COERCE(ss_m::setup_options(_popts));

    int szOpt = _sm_opts.size();
    assert (szOpt > 1);
    const char* myOpts[(2*SHORE_NUM_DEF_SM_OPTIONS) + 1];
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



/****************************************************************** 
 *
 * @fn     start_sm()
 *
 * @brief  Start Shore storage manager. 
 *         - Format and mount the device
 *         - Create the volume in the device 
 *         (Shore limitation: only 1 volume per device)
 *
 * @return 0 on success, non-zero otherwise
 *
 ******************************************************************/

int ShoreEnv::start_sm()
{
    TRACE( TRACE_DEBUG, "Starting Shore...\n");

    if (_initialized == false) {
        _pssm = new ss_m();
    }
    else {
        TRACE( TRACE_DEBUG, "Shore already started...\n");
        return (1);        
    }

    // format and mount the database...

    // Get the configuration from the config file
    char const* device =  _dev_opts[SHORE_DEF_DEV_OPTIONS[0][0]].c_str();
    int quota = atoi(_dev_opts[SHORE_DEF_DEV_OPTIONS[1][0]].c_str());
    int clobber = atoi(_dev_opts[SHORE_DEF_DEV_OPTIONS[2][0]].c_str());

    assert (_pssm);
    assert (strlen(device)>0);
    assert (quota>0);

    if (clobber) {
        TRACE( TRACE_DEBUG, "Formatting a new device (%s) with a (%d)kB quota\n",
               device, quota);

	// create and mount device
	// http://www.cs.wisc.edu/shore/1.0/man/device.ssm.html
	W_COERCE(_pssm->format_dev(device, quota, true));
        TRACE( TRACE_DEBUG, "Formatting completed...\n");
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

void ShoreEnv::usage(option_group_t& options)
{
    cerr << "Valid Shore options are: " << endl;
    options.print_usage(true, cerr);
}


/** @fn    readconfig
 *
 *  @brief Reads configuration file
 */ 

void ShoreEnv::readconfig(string conf_file)
{
    TRACE( TRACE_DEBUG, "Reading configuration file (%s)\n", conf_file);
    
    ConfigFile  sh_config(conf_file);   // config file reader
    string tmp;

    // Parse SM parameters
    for (int i = 0; i < SHORE_NUM_DEF_SM_OPTIONS; i++) {
        sh_config.readInto(tmp, SHORE_DEF_SM_OPTIONS[i][1], SHORE_DEF_SM_OPTIONS[i][2]);
        _sm_opts[SHORE_DEF_SM_OPTIONS[i][0]] = tmp;
    }    

    // Parse DEVICE parameters
    for (int i = 0; i < SHORE_NUM_DEF_DEV_OPTIONS; i++) {
        sh_config.readInto(tmp, SHORE_DEF_DEV_OPTIONS[i][0], SHORE_DEF_DEV_OPTIONS[i][1]);
        _dev_opts[SHORE_DEF_DEV_OPTIONS[i][0]] = tmp;
    }
}


/** @fn    printconfig
 *
 *  @brief Prints configuration
 */ 

void ShoreEnv::printconfig() {
    TRACE( TRACE_DEBUG, "Printing configuration\n");
    
    // Print storage manager options
    map<string,string>::iterator sm_iter;
    TRACE( TRACE_DEBUG, "** SM options\n");
    for ( sm_iter = _sm_opts.begin(); sm_iter != _sm_opts.end(); sm_iter++)
        cout << sm_iter->first << " \t| " << sm_iter->second << endl;

    // Print device options
    map<string,string>::iterator dev_iter;
    TRACE( TRACE_DEBUG, "** Device options\n");
    for ( dev_iter = _dev_opts.begin(); dev_iter != _dev_opts.end(); dev_iter++)
        cout << dev_iter->first << " \t| " << dev_iter->second << endl;    
}


/** @fn dump
 *
 *  @brief Dumps the data
 */

void ShoreEnv::dump() {

    assert (false); // TO DO
    assert (false); // TO DO: Remove TRACE

    TRACE( TRACE_DEBUG, "~~~~~~~~~~~~~~~~~~~~~\n");    
    TRACE( TRACE_DEBUG, "Dumping Shore Data\n");

    TRACE( TRACE_DEBUG, "~~~~~~~~~~~~~~~~~~~~~\n");    
}
