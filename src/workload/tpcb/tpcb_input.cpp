/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

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


// Uncomment below to execute only local xcts
#undef  ONLY_LOCAL_TCPB
#define ONLY_LOCAL_TCPB

#ifdef  ONLY_LOCAL_TCPB
#warning TPCB = Uses only local xcts
const int LOCAL_TPCB = 100;
#else
// Modify this value to control the percentage of remote xcts
// The default value is 85 (15% are remote xcts)
const int LOCAL_TPCB = 85;
#endif


/* ------------------- */
/* --- ACCT_UPDATE --- */
/* ------------------- */


acct_update_input_t create_acct_update_input(int sf, 
                                             int specificBr)
{
    assert (sf>0);

    acct_update_input_t auin;

    // The TPC-B branches start from 0 (0..SF-1)
    if (specificBr>0) {
        auin.b_id = specificBr-1;
    }
    else {
        auin.b_id = URand(0,sf-1);
    }
        
    auin.t_id = (auin.b_id * TPCB_TELLERS_PER_BRANCH) + URand(0,TPCB_TELLERS_PER_BRANCH-1);

    // 85 - 15 local Branch
    if (URand(0,100)>LOCAL_TPCB) {
        // remote branch
        auin.a_id = (URand(0,sf)*TPCB_ACCOUNTS_PER_BRANCH) + URand(0,TPCB_ACCOUNTS_PER_BRANCH-1);
    }
    else {
        // local branch
        auin.a_id = (auin.b_id*TPCB_ACCOUNTS_PER_BRANCH) + URand(0,TPCB_ACCOUNTS_PER_BRANCH-1);
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
    assert (sf>0);
    populate_db_input_t pdbin(sf,specificSub);
    return (pdbin);
}



EXIT_NAMESPACE(tpcb);
