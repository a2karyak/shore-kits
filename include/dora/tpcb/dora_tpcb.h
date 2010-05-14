/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tpcb.h
 *
 *  @brief:  The DORA TPCB class
 *
 *  @author: Ippokratis Pandis (ipandis)
 *  @date:   July 2009
 */


#ifndef __DORA_TPCB_H
#define __DORA_TPCB_H


#include <cstdio>

#include "tls.h"

#include "util.h"
#include "workload/tpcb/shore_tpcb_env.h"
#include "dora/dora_env.h"
#include "dora.h"

using namespace shore;
using namespace tpcb;


ENTER_NAMESPACE(dora);



// Forward declarations

// TPCB AcctUpod
class final_au_rvp;
class upd_br_action;
class upd_ac_action;
class upd_te_action;
class ins_hi_action;


// @note: The BranchID is the one that determines the DoraSF


// Look also include/workload/tpcb/tpcb_const.h
const int TPCB_BRANCH_PER_DORA_PART = 10000;


/******************************************************************** 
 *
 * @class: dora_tpcb
 *
 * @brief: Container class for all the data partitions for the TPCB database
 *
 ********************************************************************/

class DoraTPCBEnv : public ShoreTPCBEnv, public DoraEnv
{
public:
    
    DoraTPCBEnv(string confname);
    virtual ~DoraTPCBEnv();

    //// Control Database

    // {Start/Stop/Resume/Pause} the system 
    int start();
    int stop();
    int resume();
    int pause();
    int newrun();
    int set(envVarMap* /* vars */) { return(0); /* do nothing */ };
    int dump();
    int info() const;    
    int statistics();    
    int conf();


    //// Partition-related

    inline irpImpl* decide_part(irpTableImpl* atable, const int aid) {
        //TRACE( TRACE_STATISTICS, "X=%d AID=%d\n", atable, aid);
        // partitioning function
        return (atable->myPart(aid));
    }      


    //// DORA TPCB - PARTITIONED TABLES

    DECLARE_DORA_PARTS(br);  // Branch
    DECLARE_DORA_PARTS(te);  // Account
    DECLARE_DORA_PARTS(ac);  // Teller
    DECLARE_DORA_PARTS(hi);  // History


    //// DORA TPCB - TRXs   


    //////////////
    // AccntUpd //
    //////////////

    DECLARE_DORA_TRX(acct_update);

    DECLARE_DORA_FINAL_RVP_GEN_FUNC(final_au_rvp);

    DECLARE_DORA_ACTION_GEN_FUNC(upd_br_action,rvp_t,acct_update_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(upd_te_action,rvp_t,acct_update_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(upd_ac_action,rvp_t,acct_update_input_t);
    DECLARE_DORA_ACTION_GEN_FUNC(ins_hi_action,rvp_t,acct_update_input_t);
        
}; // EOF: DoraTPCBEnv


EXIT_NAMESPACE(dora);

#endif // __DORA_TPCB_H

