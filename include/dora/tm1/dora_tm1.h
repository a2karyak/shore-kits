/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tm1.h
 *
 *  @brief:  The DORA TM1 class
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */


#ifndef __DORA_TM1_H
#define __DORA_TM1_H


#include <cstdio>

#include "tls.h"

#include "util.h"
#include "workload/tm1/shore_tm1_env.h"
#include "dora/dora_env.h"
#include "dora.h"

using namespace shore;
using namespace tm1;


ENTER_NAMESPACE(dora);



// Forward declarations

// TM1 GetSubData
class final_gsd_rvp;
class r_sub_gsd_action;

// TM1 GetNewDest
class final_gnd_rvp;
class r_sf_gnd_action;
class r_cf_gnd_action;

// TM1 GetAccData
class final_gad_rvp;
class r_ai_gad_action;

// TM1 UpdSubData
class final_usd_rvp;
class upd_sub_usd_action;
class upd_sf_usd_action;

// TM1 UpdLocation
class final_ul_rvp;
class upd_sub_ul_action;

// TM1 InsCallFwd
class final_icf_rvp;
class mid_icf_rvp;
class r_sub_icf_action;
class r_sf_icf_action;
class ins_cf_icf_action;

// TM1 DelCallFwd
class final_dcf_rvp;
class mid_dcf_rvp;
class r_sub_dcf_action;
class del_cf_dcf_action;


// Look also include/workload/tm1/tm1_const.h
const int TM1_SUBS_PER_DORA_PART = 10000;


/******************************************************************** 
 *
 * @class: dora_tm1
 *
 * @brief: Container class for all the data partitions for the TM1 database
 *
 ********************************************************************/

class DoraTM1Env : public ShoreTM1Env, public DoraEnv
{
public:
    
    DoraTM1Env(string confname)
        : ShoreTM1Env(confname), DoraEnv()
    { }

    virtual ~DoraTM1Env() 
    { 
        stop();
    }



    //// Control Database

    // {Start/Stop/Resume/Pause} the system 
    const int start();
    const int stop();
    const int resume();
    const int pause();
    const int newrun();
    const int set(envVarMap* vars);
    const int dump();
    const int info();    
    const int statistics();    
    const int conf();


    //// Partition-related

    inline irpImpl* decide_part(irpTableImpl* atable, const int aid) {
        // partitioning function
        return (atable->myPart(aid / TM1_SUBS_PER_DORA_PART));
    }      


    //// DORA TM1 - PARTITIONED TABLES

    DECLARE_DORA_PARTS(sub);
    DECLARE_DORA_PARTS(ai);
    DECLARE_DORA_PARTS(sf);
    DECLARE_DORA_PARTS(cf);



    //// DORA TM1 - TRXs   


    ////////////////
    // GetSubData //
    ////////////////

    DECLARE_DORA_TRX(get_sub_data);

    DECLARE_DORA_FINAL_RVP_GEN_FUNC(final_gsd_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(r_sub_gsd_action,rvp_t,get_sub_data_input_t);



    ////////////////
    // GetNewDest //
    ////////////////

    DECLARE_DORA_TRX(get_new_dest);

    DECLARE_DORA_FINAL_RVP_GEN_FUNC(final_gnd_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(r_sf_gnd_action,rvp_t,get_new_dest_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(r_cf_gnd_action,rvp_t,get_new_dest_input_t);



    ////////////////
    // GetAccData //
    ////////////////

    DECLARE_DORA_TRX(get_acc_data);

    DECLARE_DORA_FINAL_RVP_GEN_FUNC(final_gad_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(r_ai_gad_action,rvp_t,get_acc_data_input_t);



    ////////////////
    // UpdSubData //
    ////////////////

    DECLARE_DORA_TRX(upd_sub_data);

    DECLARE_DORA_FINAL_RVP_GEN_FUNC(final_usd_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(upd_sub_usd_action,rvp_t,upd_sub_data_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(upd_sf_usd_action,rvp_t,upd_sub_data_input_t);



    ////////////
    // UpdLoc //
    ////////////

    DECLARE_DORA_TRX(upd_loc);

    DECLARE_DORA_FINAL_RVP_GEN_FUNC(final_ul_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(upd_sub_ul_action,rvp_t,upd_loc_input_t);



    ////////////////
    // InsCallFwd //
    ////////////////

    DECLARE_DORA_TRX(ins_call_fwd);

    DECLARE_DORA_MIDWAY_RVP_GEN_FUNC(mid_icf_rvp,ins_call_fwd_input_t);
    DECLARE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_icf_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(r_sub_icf_action,mid_icf_rvp,ins_call_fwd_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(r_sf_icf_action,rvp_t,ins_call_fwd_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(ins_cf_icf_action,rvp_t,ins_call_fwd_input_t);



    ////////////////
    // DelCallFwd //
    ////////////////

    DECLARE_DORA_TRX(del_call_fwd);

    DECLARE_DORA_MIDWAY_RVP_GEN_FUNC(mid_dcf_rvp,del_call_fwd_input_t);
    DECLARE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_dcf_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(r_sub_dcf_action,mid_dcf_rvp,del_call_fwd_input_t);

    DECLARE_DORA_ACTION_GEN_FUNC(del_cf_dcf_action,rvp_t,del_call_fwd_input_t);
        
}; // EOF: DoraTM1Env


EXIT_NAMESPACE(dora);

#endif /** __DORA_TM1_H */

