/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_env.cpp
 *
 *  @brief Implementation of a Shore environment (database)
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "util/confparser.h"
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

void env_stats_t::print_env_stats() const
{
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

const int ShoreEnv::init() 
{
    CRITICAL_SECTION(cs,_init_mutex);
    if (_initialized) {
        TRACE( TRACE_ALWAYS, "Already initialized\n");
        return (0);
    }

    // Read configuration options
    readconfig(_cname);

    // Set sys params
    if (_set_sys_params()) {
        TRACE( TRACE_ALWAYS, "Problem in setting system parameters\n");
        return (1);
    }

    
    // Apply configuration to the storage manager
    if (configure_sm()) {
        TRACE( TRACE_ALWAYS, "Error configuring Shore\n");
        return (2);
    }

    // Load the database schema
    if (load_schema()) {
        TRACE( TRACE_ALWAYS, "Error loading the database schema\n");
        return (3);
    }

    // Start the storage manager
    if (start_sm()) {
        TRACE( TRACE_ALWAYS, "Error starting Shore database\n");
        return (4);
    }

    // Call the (virtual) post-initialization function
    int clobber = atoi(_sys_opts[SHORE_SYS_OPTIONS[0][0]].c_str());
    if (!clobber) {
        if (int rval = post_init()) {
            TRACE( TRACE_ALWAYS, "Error in Shore post-init\n");
            return (rval);
        }
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

const int ShoreEnv::close() 
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

const int ShoreEnv::statistics() 
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

    TRACE( TRACE_ALWAYS, "Dismounting all devices...\n");
    w_rc_t e = _pssm->dismount_all();
    if (e.is_error()) {
        TRACE( TRACE_ALWAYS, "Problem in dismounting [0x%x]\n",
               e.err_num());
    }

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
    const char* myOpts[(2*(SHORE_NUM_SYS_SM_OPTIONS+SHORE_NUM_DB_SM_OPTIONS)) + 1];
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
    if (rc.is_error()) {
	cerr << "Error configuring Shore: " << endl;
	cerr << "\t" << w_error_t::error_string(rc.err_num()) << endl;
	cerr << "\t" << err.c_str() << endl;
	usage(*_popts);
        return (1);
    }

    // verify 
    w_reset_strstream(err);
    rc = _popts->check_required(&err);
    if (rc.is_error()) {
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
 *  @fn:     start_sm()
 *
 *  @brief:  Start Shore storage manager. 
 *           - Format and mount the device
 *           - Create the volume in the device 
 *           (Shore limitation: only 1 volume per device)
 *
 *  @return: 0 on success, non-zero otherwise
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
    char const* device =  _dev_opts[SHORE_DB_OPTIONS[0][0]].c_str();
    int quota = atoi(_dev_opts[SHORE_DB_OPTIONS[1][0]].c_str());
    int clobber = atoi(_sys_opts[SHORE_SYS_OPTIONS[0][0]].c_str());

    assert (_pssm);
    assert (strlen(device)>0);
    assert (quota>0);

    if (clobber) {
        // if didn't clobber then the db is already loaded
        CRITICAL_SECTION(cs, _load_mutex);

        TRACE( TRACE_DEBUG, "Formatting a new device (%s) with a (%d) kB quota\n",
               device, quota);

	// create and mount device
	// http://www.cs.wisc.edu/shore/1.0/man/device.ssm.html
	W_COERCE(_pssm->format_dev(device, quota, true));
        TRACE( TRACE_DEBUG, "Formatting device completed...\n");

        // mount it...
        W_COERCE(_pssm->mount_dev(device, _vol_cnt, _devid));
        TRACE( TRACE_DEBUG, "Mounting (new) device completed...\n");

        // create volume 
        // (only one per device supported, so this is kind of silly)
        // see http://www.cs.wisc.edu/shore/1.0/man/volume.ssm.html
        W_COERCE(_pssm->generate_new_lvid(_lvid));
        W_COERCE(_pssm->create_vol(device, _lvid, quota, false, *_pvid));

        // set that the database is not loaded
        _loaded = false;
    }
    else {
        // if didn't clobber then the db is already loaded
        CRITICAL_SECTION(cs, _load_mutex);

        TRACE( TRACE_DEBUG, "Using device (%s)\n", device);

        // mount it...
        W_COERCE(_pssm->mount_dev(device, _vol_cnt, _devid));
        TRACE( TRACE_DEBUG, 
               "Mounting (old) device completed. Volumes found: (%d)...\n", 
               _vol_cnt);
        
        // get the list of volumes in order to set (_lvid)
        lvid_t* volume_list;
        unsigned int volume_cnt;
        W_COERCE(_pssm->list_volumes(device, volume_list, volume_cnt));
        
        assert (volume_cnt); // there should be at least one volume

        _lvid = volume_list[0];
        delete [] volume_list;                 

        // "speculate" that the database is loaded
        _loaded = true;
    }
    
    // Using the physical ID interface

    // If we reached this point the sm has started correctly
    return (0);
}


/****************************************************************** 
 *
 *  @fn:    set_{max/active}_cpu_count()
 *
 *  @brief: Setting new cpu counts
 *
 *  @note:  Setting max cpu count is disabled. This value can be set
 *          only at the config file.
 *
 ******************************************************************/

void ShoreEnv::set_active_cpu_count(const int actcpucnt) 
{
    assert (actcpucnt);
    CRITICAL_SECTION(cpu_cnt_cs, _cpu_count_lock);
    _active_cpu_count = actcpucnt;
}



/** Helper functions */


/****************************************************************** 
 *
 *  @fn:     _set_sys_params()
 *
 *  @brief:  Sets system params set in the configuration file
 *
 *  @return: Returns 0 on success
 *
 ******************************************************************/

const int ShoreEnv::_set_sys_params()
{
    int problem = 0;

    // cpu info - first checks if valid input    
    int tmp_max_cpu_count = atoi(_sys_opts[SHORE_SYS_OPTIONS[1][0]].c_str());
    int tmp_active_cpu_count = atoi(_sys_opts[SHORE_SYS_OPTIONS[2][0]].c_str());    

    if ((tmp_active_cpu_count>0) && (tmp_active_cpu_count<=tmp_max_cpu_count)) {
        CRITICAL_SECTION(cpu_cnt_cs, _cpu_count_lock);
        _max_cpu_count = tmp_max_cpu_count;
        _active_cpu_count = tmp_active_cpu_count;
    }
    else {
        TRACE( TRACE_ALWAYS, 
               "Incorrect CPU count input: Max (%d) - Active (%d)\n",
               tmp_max_cpu_count, tmp_active_cpu_count);
        problem=1;
    }        
    assert(_max_cpu_count);
    assert(_active_cpu_count);
    assert(_active_cpu_count<=_max_cpu_count);
    print_cpus();
    return (problem);
}


void ShoreEnv::print_cpus() const { 
    TRACE( TRACE_ALWAYS, "MaxCPU=(%d) - ActiveCPU=(%d)\n", 
           _max_cpu_count, _active_cpu_count);
}


/****************************************************************** 
 *
 *  @fn    usage
 *
 *  @brief Prints shore options
 *
 ******************************************************************/ 

void ShoreEnv::usage(option_group_t& options)
{
    cerr << "Valid Shore options are: " << endl;
    options.print_usage(true, cerr);
}



/****************************************************************** 
 *
 *  @fn:    readconfig
 *
 *  @brief: Reads configuration file
 *
 ******************************************************************/

void ShoreEnv::readconfig(string conf_file)
{
    TRACE( TRACE_DEBUG, "Reading config file (%s)\n", conf_file.c_str());
    
    string tmp;
    int i=0;
    envVar* ev = envVar::instance();
    ev->setConfFile(conf_file);

    // Parse the configuration which will use (suffix)
    string configsuf = ev->getVar(CONFIG_PARAM,CONFIG_PARAM_VALUE);

    // configsuf should have taken a valid value
    assert (configsuf.compare(CONFIG_PARAM_VALUE)!=0); 
    TRACE( TRACE_ALWAYS, "Reading configuration (%s)\n", configsuf.c_str());

    // Parse SYSTEM parameters
    TRACE( TRACE_DEBUG, "Reading SYS options\n");
    for (i; i<SHORE_NUM_SYS_OPTIONS; i++) {
        tmp = ev->getVar(SHORE_SYS_OPTIONS[i][0],SHORE_SYS_OPTIONS[i][1]);
        _sys_opts[SHORE_SYS_OPTIONS[i][0]] = tmp;
    }

    // Parse SYS-SM (database-independent) parameters
    TRACE( TRACE_DEBUG, "Reading SYS-SM options\n");
    for (i=0; i<SHORE_NUM_SYS_SM_OPTIONS; i++) {
        tmp = ev->getVar(SHORE_SYS_SM_OPTIONS[i][1],SHORE_SYS_SM_OPTIONS[i][2]);
        _sm_opts[SHORE_SYS_SM_OPTIONS[i][0]] = tmp;
    }    

    // Parse DB-SM (database-specific) parameters
    TRACE( TRACE_DEBUG, "Reading DB-SM options\n");
    for (i=0; i<SHORE_NUM_DB_SM_OPTIONS; i++) {
        tmp = ev->getVar(configsuf + "-" + SHORE_DB_SM_OPTIONS[i][1],SHORE_DB_SM_OPTIONS[i][2]);
        _sm_opts[SHORE_DB_SM_OPTIONS[i][0]] = tmp;
    }    

    // Parse DB-specific parameters
    TRACE( TRACE_DEBUG, "Reading DB options\n");
    for (i=0; i<SHORE_NUM_DB_OPTIONS; i++) {
        tmp = ev->getVar(configsuf + "-" + SHORE_DB_OPTIONS[i][0],SHORE_DB_OPTIONS[i][1]);
        _dev_opts[SHORE_DB_OPTIONS[i][0]] = tmp;
    }

    //ev->printVars();
}



/****************************************************************** 
 *
 * @fn:    restart()
 *
 * @brief: Re-starts the environment
 *
 * @note:  - Stops everything
 *         - Rereads configuration
 *         - Starts everything
 *
 ******************************************************************/

const int ShoreEnv::restart()
{
    TRACE( TRACE_DEBUG, "Restarting (%s)...\n", _sysname);
    stop();
    conf();
    _set_sys_params();
    start();
    return(0);
}


/****************************************************************** 
 *
 *  @fn:    conf
 *
 *  @brief: Prints configuration
 *
 ******************************************************************/

const int ShoreEnv::conf() {
    TRACE( TRACE_DEBUG, "ShoreEnv configuration\n");
    
    // Print storage manager options
    map<string,string>::iterator iter;
    TRACE( TRACE_DEBUG, "** SYS options\n");
    for ( iter = _sys_opts.begin(); iter != _sys_opts.end(); iter++)
        cout << "(" << iter->first << ") (" << iter->second << ")" << endl;

    TRACE( TRACE_DEBUG, "** SM options\n");
    for ( iter = _sm_opts.begin(); iter != _sm_opts.end(); iter++)
        cout << "(" << iter->first << ") (" << iter->second << ")" << endl;

    TRACE( TRACE_DEBUG, "** DB options\n");
    for ( iter = _dev_opts.begin(); iter != _dev_opts.end(); iter++)
        cout << "(" << iter->first << ") (" << iter->second << ")" << endl;    

    return (0);
}





/** @fn dump
 *
 *  @brief Dumps the data
 */

const int ShoreEnv::dump() {

    TRACE( TRACE_DEBUG, "~~~~~~~~~~~~~~~~~~~~~\n");    
    TRACE( TRACE_DEBUG, "Dumping Shore Data\n");

    TRACE( TRACE_ALWAYS, "Not implemented...\n");

    TRACE( TRACE_DEBUG, "~~~~~~~~~~~~~~~~~~~~~\n");    
    return (0);
}
