/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "sm_vas.h"
#include "util/progress.h"
#include "util/c_str.h"
#include "util/guard.h"
#include "util/stopwatch.h"
#include "workload/tpcc/tpcc_tbl_parsers.h"
#include <utility>
#include <sstream>

using namespace tpcc;

static int const RECORDS = 30000;

/* create an smthread based class for all sm-related work */
class smthread_user_t : public smthread_t {
    int	argc;
    char	**argv;
public:
    int	retval;
    
    smthread_user_t(int ac, char **av) 
	: smthread_t(t_regular, "smthread_user_t"),
	  argc(ac), argv(av), retval(0)
    {
    }
    void run();
};

#define MAX_FNAME_LEN = 31;
struct file_info_t {
    std::pair<int,int> _record_size;
    stid_t 	_table_id;
    stid_t	_index_id;
    rid_t 	_first_rid;
};

struct parse_thread : public smthread_t {
    c_str _fname; // file name
    ss_m* _ssm; // database handle
    vid_t _vid;
    file_info_t _info;
    sm_stats_info_t _stats;
    
public:
    parse_thread(c_str fname, ss_m* ssm, vid_t vid)
	: _fname(fname), _ssm(ssm), _vid(vid)
    {
	memset(&_stats, 0, sizeof(_stats));
    }
    virtual void run()=0;
    ~parse_thread() {}
};

/* Runs a transaction, checking for deadlock and retrying
   automatically as need be. 'Transaction' must be a type whose
   function call operator takes no arguments and returns w_rc_t.
 */
template<class Transaction>
w_rc_t run_xct(ss_m* ssm, Transaction &t) {
    w_rc_t e;
    do {
	e = ssm->begin_xct();
	if(e)
	    break;
	e = t(ssm);
	if(e)
	    e = ssm->abort_xct();
	else
	    e = ssm->commit_xct();
    } while(e && e.err_num() == smlevel_0::eDEADLOCK);
    return e;
}

template <class Parser>
struct parser_impl : public parse_thread {
    parser_impl(c_str fname, ss_m* ssm, vid_t vid)
	: parse_thread(fname, ssm, vid)
    {
	typedef typename Parser::record_t record_t;
	static size_t const ksize = sizeof(record_t::first_type);
	static size_t const bsize = Parser().has_body()? sizeof(record_t::second_type) : 0;
	
	// blow away the previous file, if any
	create_volume_xct<Parser> cvxct(_vid, _fname.data(), _info, ksize+bsize);
	W_COERCE(run_xct(_ssm, cvxct));
     
    }
    void run();
};

struct test_parser : public parser_impl<parse_tpcc_ORDER> {
    test_parser(int tid, ss_m* ssm, vid_t vid)
	: parser_impl(c_str("tbl_tpcc/test-%02d.dat", tid), ssm, vid)
    {
    }
};

pthread_mutex_t vol_mutex = PTHREAD_MUTEX_INITIALIZER;

template<class Parser>
struct create_volume_xct {
    vid_t &_vid;
    char const* _table_name;
    file_info_t &_info;
    size_t _bytes;
    create_volume_xct(vid_t &vid, char const* tname, file_info_t &info, size_t bytes)
	: _vid(vid), _table_name(tname), _info(info), _bytes(bytes)
    {
    }
    w_rc_t operator()(ss_m* ssm) {
	struct critical_section {
	    critical_section() { pthread_mutex_lock(&vol_mutex); }
	    ~critical_section() { pthread_mutex_unlock(&vol_mutex); }
	} cs;
	//	CRITICAL_SECTION(cs, vol_mutex);
	stid_t root_iid;
	vec_t table_name(_table_name, strlen(_table_name));
	unsigned size = sizeof(_info);
	vec_t table_info(&_info, size);
	bool found;
	W_DO(ss_m::vol_root_index(_vid, root_iid));
	W_DO(ss_m::find_assoc(root_iid, table_name, &_info, size, found));
	if(found) {
	    cout << "Removing previous instance of " << _table_name << endl;
	    W_DO(ss_m::destroy_file(_info._table_id));
	    W_DO(ss_m::destroy_assoc(root_iid, table_name, table_info));
	}
	// create the file and register it with the root index
	cout << "Creating table ``" << _table_name
	     << "'' with " << _bytes << " bytes per record" << endl;
	W_DO(ssm->create_file(_vid, _info._table_id, smlevel_3::t_regular));
	W_DO(ssm->create_index(_vid, smlevel_0::t_btree, smlevel_3::t_regular, Parser::describe_key(),
			       smlevel_0::t_cc_kvl, _info._index_id));
	W_DO(ss_m::vol_root_index(_vid, root_iid));
	W_DO(ss_m::create_assoc(root_iid, table_name, table_info));
	return RCOK;
    }
};

template <class Parser>
void parser_impl<Parser>::run() {
    FILE* fd = fopen(_fname.data(), "r");
    if(fd == NULL) {
        TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", _fname.data());
	// TOOD: return an error code or something
	return;
    }

    unsigned long progress = 0;
    char linebuffer[MAX_LINE_LENGTH];
    Parser parser;
    typedef typename Parser::record_t record_t;
    static size_t const ksize = sizeof(record_t::first_type);
    static size_t const bsize = parser.has_body()? sizeof(record_t::second_type) : 0;

    sm_stats_info_t stats;
    memset(&stats, 0, sizeof(stats));
    sm_stats_info_t* dummy; // needed to keep xct commit from deleting our stats...

    int i=0;
    for(int loop=0; loop < 4; loop++) {
	fseek(fd, 0, SEEK_SET);
    W_COERCE(_ssm->begin_xct());
    
    // insert records one by one...
    record_t record;
    vec_t head(&record.first, ksize);
    vec_t body(&record.second, bsize);
    rid_t rid;
    vec_t idx(&rid, sizeof(rid_t));
    bool first = true;
    static int const INTERVAL = 1000;
    int mark = INTERVAL;
    
    for(; fgets(linebuffer, MAX_LINE_LENGTH, fd); i++) {
	record = parser.parse_row(linebuffer);
	W_COERCE(_ssm->create_rec(_info._table_id, head, bsize, body, rid));
	W_COERCE(_ssm->create_assoc(_info._index_id, head, idx));
	if(first)
	    _info._first_rid = rid;
	if(i >= RECORDS) break;
	first = false;
	//	progress_update(&progress);
	if(i >= mark) {
	    if((i % 10000) == 0)
		printf(" [%d]", pthread_self());
	    fflush(stdout);
	    W_COERCE(_ssm->commit_xct(dummy));
	    W_COERCE(_ssm->begin_xct());
	    mark += INTERVAL;
	}
	    
    }
    
    // done!
    W_COERCE(_ssm->commit_xct(dummy));
    }
    _info._record_size = std::make_pair(ksize, bsize);
    //    progress_done(parser.table_name());
    //stringstream ss;
    //ss << me()->thread_stats();
    //ss << ends;
    //    printf("Successfully loaded %d records\n%s", i, ss.str().c_str());
    if ( fclose(fd) ) {
	TRACE(TRACE_ALWAYS, "fclose() failed on %s\n", _fname.data());
	// TOOD: return an error code or something
	return;
    }

}


void
usage(option_group_t& options)
{
    cerr << "Valid options are: " << endl;
    options.print_usage(true, cerr);
}

#define DEST_DIR "log"

static int const MAX_THREADS = 32;
int active_threads = 8;

void smthread_user_t::run() {
    if(argc == 2) {
	int n = atoi(argv[1]);
	if(n > 0 && n <= MAX_THREADS)
	    active_threads = n;
    }
    cout << "Running with " << active_threads << " threads" << endl;
    option_group_t options(1);

    cout << "Configuring Shore..." << endl;
    
    // have the SSM add its options to the group
    W_COERCE(ss_m::setup_options(&options));

    // initialize the options we care about..
    static char const* opts[] = {
	"fake-command-line",
	"-sm_bufpoolsize", "409600", // in kB
	//	"-sm_logging", "no", // temporary
	"-sm_logdir", DEST_DIR,
	"-sm_logsize", "409600", // in kB
	"-sm_logbufsize", "102400", // in kB
	"-sm_diskrw", "/export/home/ryanjohn/projects/shore-lomond/installed/bin/diskrw",
	"-sm_errlog", "info", // one of {none emerg fatal alert internal error warning info debug}
    };
    w_ostrstream err;
    int opts_count = sizeof(opts)/sizeof(char const*); // clobbered by the parser
    w_rc_t rc = options.parse_command_line(opts, opts_count, 2, &err);
    err << ends;
    if(rc) {
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
    char const* device = DEST_DIR "/shoredb";
    int quota = 1000*1024; // in kB
    if(clobber) {
	cout << "Formatting a new device ``" << device
	     << "'' with a " << quota << "kB quota" << endl;

	// create and mount device
	// http://www.cs.wisc.edu/shore/1.0/man/device.ssm.html
	W_COERCE(ssm->format_dev(device, quota, true));
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

#if 1
    guard<parse_thread> threads[MAX_THREADS];
    for(int i=0; i < active_threads; i++)
	threads[i] = new test_parser(i, ssm.get(), vid);
    stopwatch_t timer;
    for(int i=0; i < active_threads; i++)
	threads[i]->fork();

    sm_stats_info_t stats;
    memset(&stats, 0, sizeof(stats));
    for(int i=0; i < active_threads; i++) {
	threads[i]->join();
	//stats += threads[i]->_stats;
    }
    fprintf(stderr, "\nCompleted in %.2lf seconds\n", timer.time());
    ss_m::gather_stats(stats, false);
    //cout << stats << endl;
	
#else
    //create and spawn threads...
    guard<parse_thread> item_thread = new parser_impl<parse_tpcc_ITEM>("tbl_tpcc/item.dat", ssm.get(), vid);
    guard<parse_thread> new_order_thread = new parser_impl<parse_tpcc_NEW_ORDER>("tbl_tpcc/new_order.dat", ssm.get(), vid);
    guard<parse_thread> history_thread = new parser_impl<parse_tpcc_HISTORY>("tbl_tpcc/history.dat", ssm.get(), vid);
    guard<parse_thread> district_thread = new parser_impl<parse_tpcc_DISTRICT>("tbl_tpcc/district.dat", ssm.get(), vid);
    guard<parse_thread> order_thread = new parser_impl<parse_tpcc_ORDER>("tbl_tpcc/order.dat", ssm.get(), vid);
    guard<parse_thread> warehouse_thread = new parser_impl<parse_tpcc_WAREHOUSE>("tbl_tpcc/warehouse.dat", ssm.get(), vid);

#if 0 // serial
    W_COERCE(order_thread->fork());
    W_IGNORE(order_thread->join());
    W_COERCE(district_thread->fork());
    W_IGNORE(district_thread->join());
    W_COERCE(warehouse_thread->fork());
    W_IGNORE(warehouse_thread->join());
    W_COERCE(history_thread->fork());
    W_IGNORE(history_thread->join());
    W_COERCE(new_order_thread->fork());
    W_IGNORE(new_order_thread->join());
    W_COERCE(item_thread->fork());
    W_IGNORE(item_thread->join());
#else
    W_COERCE(warehouse_thread->fork());
    W_COERCE(district_thread->fork());
    W_COERCE(order_thread->fork());
    W_COERCE(history_thread->fork());
    W_COERCE(new_order_thread->fork());
    W_COERCE(item_thread->fork());

    W_IGNORE(order_thread->join());
    W_IGNORE(district_thread->join());
    W_IGNORE(warehouse_thread->join());
    W_IGNORE(history_thread->join());
    W_IGNORE(new_order_thread->join());
   W_IGNORE(item_thread->join());
#endif
#endif
   

    // done!
    cout << "Shutting down Shore..." << endl;
}

int main(int argc, char* argv[]) {
    smthread_user_t *smtu = new smthread_user_t(argc, argv);
    if (!smtu)
	W_FATAL(fcOUTOFMEMORY);

    w_rc_t e = smtu->fork();
    if(e) {
	cerr << "error forking main thread: " << e <<endl;
	return 1;
    }
    e = smtu->join();
    if(e) {
	cerr << "error joining main thread: " << e <<endl;
	return 1;
    }

    int	rv = smtu->retval;
    delete smtu;

    return rv;
}
