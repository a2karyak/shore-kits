/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_mbench.h
 *
 *  @brief:  DORA MBENCHES
 *
 *  @note:   Definition of RVPs and Actions that synthesize 
 *           the mbenches according to DORA
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_MBENCHES_H
#define __DORA_MBENCHES_H


#include "dora.h"
#include "workload/tpcc/shore_tpcc_env.h"
#include "dora/tpcc/dora_tpcc.h"

using namespace shore;
using namespace tpcc;


ENTER_NAMESPACE(dora);




/******************************************************************** 
 *
 * DORA TPCC MBENCHES
 *
 ********************************************************************/

DECLARE_DORA_FINAL_RVP_CLASS(final_mb_rvp,DoraTPCCEnv,1,1);

DECLARE_DORA_ACTION_NO_RVP_CLASS(upd_wh_mb_action,int,DoraTPCCEnv,mbench_wh_input_t,1);

DECLARE_DORA_ACTION_NO_RVP_CLASS(upd_cust_mb_action,int,DoraTPCCEnv,mbench_cust_input_t,1);


EXIT_NAMESPACE(dora);

#endif /** __DORA_MBENCHES_H */

