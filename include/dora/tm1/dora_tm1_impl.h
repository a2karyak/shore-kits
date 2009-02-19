/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tm1_impl.h
 *
 *  @brief:  DORA TM1 TRXs
 *
 *  @note:   Definition of RVPs and Actions that synthesize (according to DORA)
 *           the TM1 trx
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */


#ifndef __DORA_TM1_IMPL_H
#define __DORA_TM1_IMPL_H


#include "dora.h"
#include "workload/tm1/shore_tm1_env.h"
#include "dora/tm1/dora_tm1.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tm1;


/******************************************************************** 
 *
 * DORA TM1 GET_SUB_DATA
 *
 ********************************************************************/

DECLARE_DORA_FINAL_RVP_CLASS(final_gsd_rvp,DoraTM1Env,1,1);

DECLARE_DORA_ACTION_NO_RVP_CLASS(r_sub_gsd_action,int,DoraTM1Env,get_sub_data_input_t,1);



/******************************************************************** 
 *
 * DORA TM1 GET_NEW_DEST
 *
 ********************************************************************/

DECLARE_DORA_FINAL_RVP_CLASS(final_gnd_rvp,DoraTM1Env,2,2);

DECLARE_DORA_ACTION_NO_RVP_CLASS(r_sf_gnd_action,int,DoraTM1Env,get_new_dest_input_t,2);
DECLARE_DORA_ACTION_NO_RVP_CLASS(r_cf_gnd_action,int,DoraTM1Env,get_new_dest_input_t,3);



/******************************************************************** 
 *
 * DORA TM1 GET_ACC_DATA
 *
 ********************************************************************/

DECLARE_DORA_FINAL_RVP_CLASS(final_gad_rvp,DoraTM1Env,1,1);

DECLARE_DORA_ACTION_NO_RVP_CLASS(r_ai_gad_action,int,DoraTM1Env,get_acc_data_input_t,2);



/******************************************************************** 
 *
 * DORA TM1 UPD_SUB_DATA
 *
 ********************************************************************/

DECLARE_DORA_FINAL_RVP_CLASS(final_usd_rvp,DoraTM1Env,2,2);

DECLARE_DORA_ACTION_NO_RVP_CLASS(upd_sub_usd_action,int,DoraTM1Env,upd_sub_data_input_t,1);
DECLARE_DORA_ACTION_NO_RVP_CLASS(upd_sf_usd_action,int,DoraTM1Env,upd_sub_data_input_t,2);



/******************************************************************** 
 *
 * DORA TM1 UPD_LOC
 *
 ********************************************************************/

DECLARE_DORA_FINAL_RVP_CLASS(final_ul_rvp,DoraTM1Env,1,1);

DECLARE_DORA_ACTION_NO_RVP_CLASS(upd_sub_ul_action,int,DoraTM1Env,upd_loc_input_t,1);



/******************************************************************** 
 *
 * DORA TM1 INS_CALL_FWD
 *
 ********************************************************************/

DECLARE_DORA_FINAL_RVP_CLASS(final_icf_rvp,DoraTM1Env,2,2);

DECLARE_DORA_ACTION_NO_RVP_CLASS(r_sf_icf_action,int,DoraTM1Env,ins_call_fwd_input_t,2);
DECLARE_DORA_ACTION_NO_RVP_CLASS(ins_cf_icf_action,int,DoraTM1Env,ins_call_fwd_input_t,3);



/******************************************************************** 
 *
 * DORA TM1 DEL_CALL_FWD
 *
 ********************************************************************/

DECLARE_DORA_FINAL_RVP_CLASS(final_dcf_rvp,DoraTM1Env,1,1);

DECLARE_DORA_ACTION_NO_RVP_CLASS(del_cf_dcf_action,int,DoraTM1Env,del_call_fwd_input_t,3);




EXIT_NAMESPACE(dora);

#endif /** __DORA_TM1_IMPL_H */

