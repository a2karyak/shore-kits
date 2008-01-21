/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_load.h
 *
 *  @brief Definition of the Shore TPC-C loaders
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "workload/tpcc/shore_tpcc_env.h"
#include "workload/tpcc/shore_tpcc_load.h"


using namespace tpcc;


/** sl_thread_t methods */


/** @fn    usage
 *
 *  @brief Prints shore options
 */ 

void sl_thread_t::usage(option_group_t& options)
{
    cerr << "Valid options are: " << endl;
    options.print_usage(true, cerr);
}


/** @fn run
 * 
 *  @brief Runs a shore thread
 */

void sl_thread_t::run() 
{
    option_group_t options(1);

    cout << "Configuring Shore..." << endl;
    
    // have the SSM add its options to the group
    W_COERCE(ss_m::setup_options(&options));

    // initialize the options we care about..
    static char const* opts[] = {
	"fake-command-line",
	"-sm_bufpoolsize", "102400", // in kB
        "-sm_logging", "no", // can be removed temporary
	"-sm_logdir", "log",
	"-sm_logsize", "102400", // in kB
	"-sm_logbufsize", "10240", // in kB
	"-sm_diskrw", "/export/home/ipandis/DEV/shore-lomond/installed/bin/diskrw",
	"-sm_errlog", "info", // one of {none emerg fatal alert internal error warning info debug}
    };

    w_ostrstream err;
    int opts_count = sizeof(opts)/sizeof(char const*); // clobbered by the parser
    w_rc_t rc = options.parse_command_line(opts, opts_count, 2, &err);
    err << ends;

    if (rc) {
	cerr << "Error configuring Shore: " << endl;
	cerr << "\t" << w_error_t::error_string(rc.err_num()) << endl;
	cerr << "\t" << err.c_str() << endl;
	usage(options);
    }

    // verify 
    w_reset_strstream(err);
    rc = options.check_required(&err);
    if (rc) {
	cerr << "These required options are not set:" << endl;
	cerr << err.c_str() << endl;
	usage(options);
    }

    cout << "Starting Shore..." << endl;
    guard<ss_m> ssm = new ss_m();

    // format? and mount the database...
    // TODO: make these command-line options!
    bool clobber = true;
    char const* device = "tbl_tpcc/shore";
    int quota = 100*1024; // 100 MB

    if(clobber) {
	cout << "Formatting a new device ``" << device
	     << "'' with a " << quota << "kB quota" << endl;

	// create and mount device
	// http://www.cs.wisc.edu/shore/1.0/man/device.ssm.html
	W_COERCE(ssm->format_dev(device, quota, true));
        cout << "Formatting completed..." << endl;
    }

    // mount it...
    unsigned vol_cnt;
    devid_t devid;
    W_COERCE(ssm->mount_dev(device, vol_cnt, devid));
    
    lvid_t lvid; // this is a "crutch for those... using the physical ID... interface"
    vid_t vid(1);
    if(clobber) {
	// create volume (only one per device supported, so this is kind of silly)
	// see http://www.cs.wisc.edu/shore/1.0/man/volume.ssm.html
	W_COERCE(ssm->generate_new_lvid(lvid));
	W_COERCE(ssm->create_vol(device, lvid, quota, false, vid));
    }
    else {
	lvid_t* lvid_list;
	unsigned lvid_cnt;
	W_COERCE(ssm->list_volumes(device, lvid_list, lvid_cnt));
	if (lvid_cnt == 0) {
	    cerr << "Error, device has no volumes" << endl;
	    exit();
	}	
	lvid = lvid_list[0];
	delete [] lvid_list;
    }

    static int const THREADS = 1;// ShoreTPCCEnv::SHORE_PAYMENT_TABLES;
    guard<shore_parse_thread> threads[THREADS];

    // initialize all table loaders
    threads[ShoreTPCCEnv::WAREHOUSE] = 
        new shore_parser_impl_WAREHOUSE(ShoreTPCCEnv::WAREHOUSE, 
                                        ssm.get(), vid, _env);
    /*
    threads[ShoreTPCCEnv::DISTRICT] = 
        new shore_parser_impl_DISTRICT(ShoreTPCCEnv::DISTRICT, 
                                       ssm.get(), vid, _env);
    threads[ShoreTPCCEnv::CUSTOMER] = 
        new shore_parser_impl_CUSTOMER(ShoreTPCCEnv::CUSTOMER, 
                                       ssm.get(), vid, _env);

    threads[ShoreTPCCEnv::HISTORY] = 
        new shore_parser_impl_HISTORY(ShoreTPCCEnv::HISTORY, 
                                      ssm.get(), vid, _env);
    */

    // start each table loader
    for(int i=0; i < THREADS; i++) {
        cout << "Starting loader..." << endl;
	threads[i]->fork();
    }

    sm_stats_info_t stats;
    memset(&stats, 0, sizeof(stats));
    for(int i=0; i < THREADS; i++) {
	threads[i]->join();
	//stats += threads[i]->_stats;
    }
    ss_m::gather_stats(stats, false);
    //    cout << stats << endl;
	
    // done!
    cout << "Shutting down Shore..." << endl;
}
