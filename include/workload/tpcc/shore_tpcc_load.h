/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_load.h
 *
 *  @brief Definition of the Shore TPC-C loaders
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_LOAD_H
#define __SHORE_TPCC_LOAD_H


#include "workload/tpcc/tpcc_loader.h"
#include "sm_vas.h"
#include <utility>


ENTER_NAMESPACE(tpcc);


///////////////////////////////////////////////////////////
// @class smthread_user_t
//
// @brief An smthread base class for all sm-related work

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
    
public:

    shore_parse_thread(c_str fname, ss_m* ssm, vid_t vid)
	: _fname(fname), _ssm(ssm), _vid(vid)
    {
	memset(&_stats, 0, sizeof(_stats));
    }

    virtual void run()=0;
    ~shore_parse_thread() {}

}; // EOF: shore_parse_thread



EXIT_NAMESPACE(tpcc);

#endif
