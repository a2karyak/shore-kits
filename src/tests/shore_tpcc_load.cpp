/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "sm_vas.h"
#include "util/progress.h"
#include "util/c_str.h"
#include "workload/tpcc/tpcc_tbl_parsers.h"
#include <utility>

using namespace tpcc;

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
    serial_t 	_table_id;
    serial_t 	_first_rid;
};

struct parse_thread : public smthread_t {
    c_str _fname; // file name
    ss_m* _ssm; // database handle
    lvid_t _lvid;
    file_info_t _info;
    
public:
    parse_thread(c_str fname, ss_m* ssm, lvid_t lvid)
	: _fname(fname), _ssm(ssm), _lvid(lvid)
    {
    }
    virtual void run()=0;
    ~parse_thread() {}
};

template <class Parser>
struct parser_impl : public parse_thread {
    parser_impl(c_str fname, ss_m* ssm, lvid_t lvid) : parse_thread(fname, ssm, lvid) { }
    void run();
};



template <class Parser>
void parser_impl<Parser>::run() {
    FILE* fd = fopen(_fname.data(), "r");
    if(fd == NULL) {
        TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", _fname.data());
	// TOOD: return an error code or something
	return;
    }

    serial_t root_iid;
    unsigned long progress = 0;
    char linebuffer[MAX_LINE_LENGTH];
    Parser parser;
    typedef typename Parser::record_t record_t;
    static size_t const ksize = sizeof(record_t::first_type);
    static size_t const bsize = parser.has_body()? sizeof(record_t::second_type) : 0;
    vec_t table_name(parser.table_name(), strlen(parser.table_name()));

    // blow away the previous file, if any
    file_info_t info;
    unsigned size = sizeof(info);
    vec_t table_info(&info, size);
    bool found;
    W_COERCE(_ssm->begin_xct());
    W_COERCE(ss_m::vol_root_index(_lvid, root_iid));
    W_COERCE(ss_m::find_assoc(_lvid, root_iid, table_name, &info, size, found));
    if(found) {
	cout << "Removing previous instance of " << parser.table_name() << endl;
	W_COERCE(ss_m::destroy_file(_lvid, info._table_id));
	W_COERCE(ss_m::destroy_assoc(_lvid, root_iid, table_name, table_info));
    }
    
    W_COERCE(_ssm->commit_xct());
    
    W_COERCE(_ssm->begin_xct());
    // create the file and register it with the root index
    cout << "Creating table ``" << parser.table_name()
	 << "'' with " << (ksize+bsize) << " bytes per record" << endl;
    W_COERCE(_ssm->create_file(_lvid, info._table_id, smlevel_3::t_regular));
    W_COERCE(ss_m::vol_root_index(_lvid, root_iid));
    W_COERCE(ss_m::create_assoc(_lvid, root_iid, table_name, table_info));
    
    // insert records one by one...
    record_t record;
    vec_t head(&record.first, ksize);
    vec_t body(&record.second, bsize);
    serial_t rid;
    bool first = true;
    for(int i=0; fgets(linebuffer, MAX_LINE_LENGTH, fd); i++) {
	record = parser.parse_row(linebuffer);
	W_COERCE(_ssm->create_rec(_lvid, info._table_id, head, bsize, body, rid));
	if(first)
	    info._first_rid = rid;
	
	first = false;
	progress_update(&progress);
	if(i >= 1000) {
	    W_COERCE(_ssm->commit_xct());
	    W_COERCE(_ssm->begin_xct());
	    i=0;
	}
	    
    }
    
    // done!
    W_COERCE(_ssm->commit_xct());
    
    info._record_size = std::make_pair(ksize, bsize);
    progress_done(parser.table_name());
    _info = info;
    cout << "Successfully loaded " << progress << " records" << endl;
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

void smthread_user_t::run() {
    option_group_t options(1);

    cout << "Configuring Shore..." << endl;
    
    // have the SSM add its options to the group
    W_COERCE(ss_m::setup_options(&options));

    // initialize the options we care about..
    static char const* opts[] = {
	"fake-command-line",
	"-sm_bufpoolsize", "102400", // in kB
	"-sm_logdir", "log",
	"-sm_logsize", "102400", // in kB
	"-sm_diskrw", "/export/home/ryanjohn/projects/shore-lomond/installed/bin/diskrw",
	"-sm_errlog", "info", // one of {none emerg fatal alert internal error warning info debug}
    };
    w_ostrstream err;
    int opts_count = 11; // clobbered by the parser
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
    char const* device = "tbl_tpcc/shore";
    int quota = 100*1024; // 100 MB
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
    
    lvid_t lvid;
    if(clobber) {
	// create volume (only one per device supported, so this is kind of silly)
	// see http://www.cs.wisc.edu/shore/1.0/man/volume.ssm.html
	W_COERCE(ssm->generate_new_lvid(lvid));
	W_COERCE(ssm->create_vol(device, lvid, quota));
	W_COERCE(ssm->add_logical_id_index(lvid, 0, 0));
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

    //create and spawn threads...
    guard<parse_thread> item_thread = new parser_impl<parse_tpcc_ITEM>("tbl_tpcc/item.dat", ssm.get(), lvid);
    guard<parse_thread> new_order_thread = new parser_impl<parse_tpcc_NEW_ORDER>("tbl_tpcc/new_order.dat", ssm.get(), lvid);
    guard<parse_thread> history_thread = new parser_impl<parse_tpcc_HISTORY>("tbl_tpcc/history.dat", ssm.get(), lvid);
    guard<parse_thread> district_thread = new parser_impl<parse_tpcc_DISTRICT>("tbl_tpcc/district.dat", ssm.get(), lvid);
    guard<parse_thread> order_thread = new parser_impl<parse_tpcc_ORDER>("tbl_tpcc/order.dat", ssm.get(), lvid);
    guard<parse_thread> warehouse_thread = new parser_impl<parse_tpcc_WAREHOUSE>("tbl_tpcc/warehouse.dat", ssm.get(), lvid);

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
