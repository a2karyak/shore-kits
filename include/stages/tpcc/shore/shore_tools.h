/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tools.h
 *
 *  @brief Shore common tools
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TOOLS_H
#define __SHORE_TOOLS_H


#include "sm_vas.h"
#include "util/namespace.h"
#include "workload/tpcc/shore_tpcc_env.h"

ENTER_NAMESPACE(tpcc);


/** Exported variable */

extern ShoreTPCCEnv* shore_env;


/** @fn run_xct
 *  
 *  @brief  Runs a transaction, checking for deadlock and retrying
 *  automatically as need be. 
 *
 *  @note Transaction must be a type whose function call operator takes 
 *  no arguments and returns w_rc_t.
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



/** @struct create_volume_xct
 *  
 *  @brief  Creates a volume in the context of a transaction.
 */

template<class Parser>
struct create_volume_xct 
{
    //    vid_t &_vid;
    //    pthread_mutex_t* _vol_mutex;
    ShoreTPCCEnv* _penv;

    char const* _table_name;
    file_info_t &_info;
    size_t _bytes;

    create_volume_xct(char const* tname, file_info_t &info, 
                      size_t bytes, ShoreTPCCEnv* env
                      )
	: _table_name(tname), _info(info), _bytes(bytes), _penv(env)
    {
    }

    w_rc_t operator()(ss_m* ssm) {

	CRITICAL_SECTION(cs, *(_penv->get_vol_mutex()));

	stid_t root_iid;
	vec_t table_name(_table_name, strlen(_table_name));
	unsigned size = sizeof(_info);
	vec_t table_info(&_info, size);
	bool found;

	W_DO(ss_m::vol_root_index(*(_penv->get_db_vid()), root_iid));
	W_DO(ss_m::find_assoc(root_iid, table_name, &_info, size, found));

	if(found) {
	    cout << "Removing previous instance of " << _table_name << endl;
	    W_DO(ss_m::destroy_file(_info._table_id));
	    W_DO(ss_m::destroy_assoc(root_iid, table_name, table_info));
	}

	// create the file and register it with the root index
	cout << "Creating table ``" << _table_name
	     << "'' with " << _bytes << " bytes per record" << endl;
	W_DO(ssm->create_file(*(_penv->get_db_vid()), _info._table_id, smlevel_3::t_regular));
	W_DO(ss_m::vol_root_index(*(_penv->get_db_vid()), root_iid));
	W_DO(ss_m::create_assoc(root_iid, table_name, table_info));
	return RCOK;
    }

}; // EOF: create_volume_xct 



EXIT_NAMESPACE(tpcc);

#endif
