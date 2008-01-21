/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_load.h
 *
 *  @brief Definition of the Shore TPC-C loaders
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_LOAD_H
#define __SHORE_TPCC_LOAD_H


#include "workload/loader.h"
#include "workload/tpcc/tpcc_tbl_parsers.h"
#include "stages/tpcc/shore/shore_tools.h"
#include "workload/tpcc/shore_tpcc_env.h"
#include <utility>


using namespace qpipe;


ENTER_NAMESPACE(tpcc);


///////////////////////////////////////////////////////////
// @class sl_thread_t
//
// @brief An smthread base class for all sm loading related work

class sl_thread_t : public smthread_t {
private:
    ShoreTPCCEnv* _env;

public:
    int	retval;
    
    sl_thread_t(ShoreTPCCEnv* env) 
	: smthread_t(t_regular, "sl_thread_t"),
          retval(0)
    {
    }

    // thread entrance
    void run();
    
    // helper methods
    void usage(option_group_t& options);
};


///////////////////////////////////////////////////////////
// @struct file_info_t
//
// @brief Structure that represents a table

#define MAX_FNAME_LEN = 31;
struct file_info_t {
    std::pair<int,int> _record_size;
    stid_t 	_table_id;
    rid_t 	_first_rid;
};


///////////////////////////////////////////////////////////
// @class Abstract shore_parse_thread
//
// @brief Parser thread for Shore
//
// @note run() function is pure virtual

struct shore_parse_thread : public smthread_t {
    c_str _fname; // file name
    ss_m* _ssm; // database handle
    vid_t _vid;
    file_info_t _info;
    sm_stats_info_t _stats;
    ShoreTPCCEnv* _env;
    
public:

    shore_parse_thread(c_str fname, ss_m* ssm, vid_t vid, ShoreTPCCEnv* env)
	: _fname(fname), _ssm(ssm), _vid(vid), _env(env)
    {
	memset(&_stats, 0, sizeof(_stats));
    }

    virtual void run()=0;
    ~shore_parse_thread() {}

    inline ShoreTPCCEnv* getEnv() { return (_env); }

}; // EOF: shore_parse_thread



///////////////////////////////////////////////////////////
// @struct shore_parser_impl
//
// @brief Implementation of a shore parser thread

template <class Parser>
struct shore_parser_impl : public shore_parse_thread {
    shore_parser_impl(c_str fname, ss_m* ssm, vid_t vid, ShoreTPCCEnv* env)
	: shore_parse_thread(fname, ssm, vid, env)
    {
    }
    void run();

}; // EOF: shore_parse_thread


template <class Parser>
void shore_parser_impl<Parser>::run() {

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

    // blow away the previous file, if any
    file_info_t info;
    create_volume_xct<Parser> cvxct(_vid, _fname.data(), info, ksize+bsize, 
                                    getEnv()->get_vol_mutex());
    W_COERCE(run_xct(_ssm, cvxct));
     
    sm_stats_info_t stats;
    memset(&stats, 0, sizeof(stats));
    sm_stats_info_t* dummy; // needed to keep xct commit from deleting our stats...
    
    W_COERCE(_ssm->begin_xct());
    
    // insert records one by one...
    record_t record;
    vec_t head(&record.first, ksize);
    vec_t body(&record.second, bsize);
    rid_t rid;
    bool first = true;
    static int const INTERVAL = 100000;
    int mark = INTERVAL;
    int i;
    
    for(i=0; fgets(linebuffer, MAX_LINE_LENGTH, fd); i++) {
	record = parser.parse_row(linebuffer);
	W_COERCE(_ssm->create_rec(info._table_id, head, bsize, body, rid));

	if(first)
	    info._first_rid = rid;
	
	first = false;
	progress_update(&progress);
	if(i >= mark) {
	    W_COERCE(_ssm->commit_xct(dummy));
	    W_COERCE(_ssm->begin_xct());
	    mark += INTERVAL;
	}
	    
    }
    
    // done!
    W_COERCE(_ssm->commit_xct(dummy));
    info._record_size = std::make_pair(ksize, bsize);
    progress_done(parser.table_name());
    _info = info;
    cout << "Successfully loaded " << i << " records" << endl;
    if ( fclose(fd) ) {
	TRACE(TRACE_ALWAYS, "fclose() failed on %s\n", _fname.data());
	// TOOD: return an error code or something
	return;
    }
}


//// Defines all Shore parsers

#define DEFINE_SHORE_TPCC_PARSER_IMPL(tname) \
    struct shore_parser_impl_##tname : public shore_parser_impl<parse_tpcc_##tname> { \
            shore_parser_impl_##tname(int tid, ss_m* ssm, vid_t vid, ShoreTPCCEnv* env) \
            : shore_parser_impl(c_str("%s/%s", SHORE_TPCC_DATA_DIR, SHORE_TPCC_DATA_##tname), ssm, vid, env) {}}

DEFINE_SHORE_TPCC_PARSER_IMPL(WAREHOUSE);
DEFINE_SHORE_TPCC_PARSER_IMPL(DISTRICT);
DEFINE_SHORE_TPCC_PARSER_IMPL(CUSTOMER);
DEFINE_SHORE_TPCC_PARSER_IMPL(HISTORY);
DEFINE_SHORE_TPCC_PARSER_IMPL(ITEM);
DEFINE_SHORE_TPCC_PARSER_IMPL(NEW_ORDER);
DEFINE_SHORE_TPCC_PARSER_IMPL(ORDER);
DEFINE_SHORE_TPCC_PARSER_IMPL(ORDERLINE);
DEFINE_SHORE_TPCC_PARSER_IMPL(STOCK);       

#undef DEFINE_SHORE_TPCC_PARSER_IMPL

/*
struct test_parser : public shore_parser_impl<parse_tpcc_ORDER> {
    test_parser(int tid, ss_m* ssm, vid_t vid)
	: shore_parser_impl(c_str("tbl_tpcc/test-%02d.dat", tid), ssm, vid)
    {
    }

}; // EOF: test_parser
*/


EXIT_NAMESPACE(tpcc);

#endif
