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
#include "stages/tpcc/common/tpcc_tbl_parsers.h"

#include "sm/shore/shore_file_desc.h"
#include "sm/shore/shore_tools.h"

#include "workload/tpcc/shore_tpcc_env.h"

#include <utility>


using namespace qpipe;
using namespace shore;


ENTER_NAMESPACE(tpcc);



///////////////////////////////////////////////////////////
// @struct file_info_t
//
// @brief Structure that represents a table

// #define MAX_FNAME_LEN = 31;
// struct file_info_t {
//     std::pair<int,int> _record_size;
//     stid_t 	       _table_id;
//     rid_t 	       _first_rid;
// };


///////////////////////////////////////////////////////////
// @class loading_smt_t
//
// @brief An smthread-based class for all sm loading related work

class loading_smt_t : public smthread_t {
private:
    ShoreTPCCEnv* _env;    
    c_str _lname;

public:
    int	_rv;
    
    loading_smt_t(ShoreTPCCEnv* env, c_str lname) 
	: smthread_t(t_regular), 
          _env(env), _lname(lname), _rv(0)
    {
    }

    ~loading_smt_t() { }

    // thread entrance
    void run();

    // methods
    int loaddata();

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }
    inline c_str tname() { return (_lname); }

}; // EOF: loading_smt_t



///////////////////////////////////////////////////////////
// @class closing_smt_t
//
// @brief An smthread-based class for closing the Shore environment

class closing_smt_t : public smthread_t {
private:
    ShoreTPCCEnv* _env;    
    c_str _tname;

public:
    int	_rv;
    
    closing_smt_t(ShoreTPCCEnv* env, c_str cname) 
	: smthread_t(t_regular), 
          _env(env), _tname(cname),_rv(0)
    {
    }

    ~closing_smt_t() { }

    // thread entrance
    void run();

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }
    inline c_str tname() { return (_tname); }

}; // EOF: closing_smt_t



///////////////////////////////////////////////////////////
// @class du_smthread_t
//
// @brief An smthread-based class for all querying stats of the sm

class du_smt_t : public smthread_t {
private:
    ShoreTPCCEnv* _env;    
    c_str _dname;

public:
    int	_rv;
    
    du_smt_t(ShoreTPCCEnv* env, c_str dname) 
	: smthread_t(t_regular), 
          _env(env), _dname(dname), _rv(0)
    {
    }

    ~du_smt_t() { }

    // thread entrance
    void run();

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }
    inline c_str tname() { return (_dname); }

}; // EOF: du_smt_t



///////////////////////////////////////////////////////////
// @class Abstract shore_parse_thread
//
// @brief Parser thread for Shore
//
// @note run() function is pure virtual

class shore_parse_thread : public smthread_t {
protected:
    file_info_t _info;
    c_str _load_fname;
    sm_stats_info_t _stats;
    ShoreTPCCEnv* _env;
    
public:

    shore_parse_thread(c_str fname, ShoreTPCCEnv* env)
	: _load_fname(fname), _env(env)
    {
	memset(&_stats, 0, sizeof(_stats));
    }

    virtual ~shore_parse_thread() {}
    virtual void run()=0;

}; // EOF: shore_parse_thread



///////////////////////////////////////////////////////////
// @struct shore_parser_impl
//
// @brief Implementation of a shore parser thread

template <class Parser>
struct shore_parser_impl : public shore_parse_thread {
    shore_parser_impl(c_str fname, ShoreTPCCEnv* env)
	: shore_parse_thread(fname, env)
    {
    }

    void run();

}; // EOF: shore_parse_thread


template <class Parser>
void shore_parser_impl<Parser>::run() {

    FILE* fd = fopen(_load_fname.data(), "r");
    if(fd == NULL) {
        TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", _load_fname.data());
	// TODO: return an error code or something
	return;
    }

    unsigned long progress = 0;
    char linebuffer[MAX_LINE_LENGTH];
    Parser parser;
    typedef typename Parser::record_t record_t;
    static size_t const ksize = sizeof(record_t::first_type);
    static size_t const bsize = parser.has_body() ? sizeof(record_t::second_type) : 0;

    // blow away the previous file, if any
    create_volume_xct<Parser> cvxct(_load_fname.data(), _info, 
                                    ksize+bsize, _env);
    W_COERCE(run_xct(_env->db(), cvxct));
     
    sm_stats_info_t stats;
    memset(&stats, 0, sizeof(stats));
    sm_stats_info_t* dummy; // needed to keep xct commit from deleting our stats...
    
    // Start inserting records
    W_COERCE(_env->db()->begin_xct());
    
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
	W_COERCE(_env->db()->create_rec(_info.fid(), head, bsize, body, rid));

        // Remember the first row inserted
	if(first) {
	    _info.set_first_rid(rid);
            first = false;
        }

	progress_update(&progress);
	if(i >= mark) {
            // commit every INTERVAL records
	    W_COERCE(_env->db()->commit_xct(dummy));
	    W_COERCE(_env->db()->begin_xct());
	    mark += INTERVAL;
	}	    
    }
    
    // done!
    W_COERCE(_env->db()->commit_xct(dummy));
    _info.set_record_size(std::make_pair(ksize, bsize));
    progress_done(parser.table_name());

    cout << "Successfully loaded " << i << " records" << endl;
    if ( fclose(fd) ) {
	TRACE(TRACE_ALWAYS, "fclose() failed on %s\n", _load_fname.data());
	// TODO return an error code or something
	return;
    }
}


//// Defines all Shore parsers

#define DEFINE_SHORE_TPCC_PARSER_IMPL(tname) \
    struct shore_parser_impl_##tname : public shore_parser_impl<parse_tpcc_##tname> { \
            shore_parser_impl_##tname(int tid, ShoreTPCCEnv* env) \
            : shore_parser_impl(c_str("%s/%s", SHORE_TPCC_DATA_DIR, SHORE_TPCC_DATA_##tname), env) {}}

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


EXIT_NAMESPACE(tpcc);

#endif
