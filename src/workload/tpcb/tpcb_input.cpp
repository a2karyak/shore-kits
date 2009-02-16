/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:  tpcb_input.cpp
 *
 *  @brief: Implementation of the (common) inputs for the TPCB trxs
 */


#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif


#include "workload/tpcb/tpcb_input.h"



ENTER_NAMESPACE(tpcb);


/* ------------------- */
/* --- ACCT_UPDATE --- */
/* ------------------- */


acct_update_input_t create_acct_update_input(int sf, 
                                             int specificBr)
{
    w_assert3 (sf>0);

    acct_update_input_t auin;

    if (specificBr>=0)
        auin.b_id = specificBr;
    else
        auin.b_id = URand(0,sf);
        
    auin.t_id = auin.b_id * TPCB_TELLERS_PER_BRANCH + URand(0,TPCB_TELLERS_PER_BRANCH);

    // 85 - 15 local Branch
    if (URand(0,100)>85) {
        // remote branch
        auin.a_id = URand(0,sf) + URand(0,TPCB_ACCOUNTS_PER_BRANCH);
    }
    else {
        // local branch
        auin.a_id = auin.b_id + URand(0,TPCB_ACCOUNTS_PER_BRANCH);
    }
        
    auin.delta = URand(0,2000000) - 1000000;
    return (auin);
}




/* -------------------- */
/* ---  POPULATE_DB --- */
/* -------------------- */


populate_db_input_t create_populate_db_input(int sf, 
                                             int specificSub)
{
    w_assert3 (sf>0);
    populate_db_input_t pdbin(sf,specificSub);
    return (pdbin);
}



EXIT_NAMESPACE(tpcb);
