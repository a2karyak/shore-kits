/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University
   
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


/* ------------------- */
/* --- ACCT_UPDATE --- */
/* ------------------- */


acct_update_input_t create_acct_update_input(int sf, 
                                             int specificBr)
{
    assert (sf>0);

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
    assert (sf>0);
    populate_db_input_t pdbin(sf,specificSub);
    return (pdbin);
}



EXIT_NAMESPACE(tpcb);
