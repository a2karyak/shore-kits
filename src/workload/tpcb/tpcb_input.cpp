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

#include "k_defines.h"

#include "workload/tpcb/tpcb_input.h"

ENTER_NAMESPACE(tpcb);


// Uncomment below to execute only local xcts
#undef  ONLY_LOCAL_TCPB
#define ONLY_LOCAL_TCPB

#ifdef  ONLY_LOCAL_TCPB
#warning TPCB - uses only local xcts
const int LOCAL_TPCB = 100;
#else
// Modify this value to control the percentage of remote xcts
// The default value is 85 (15% are remote xcts)
const int LOCAL_TPCB = 85;
#endif

#warning TPCB uses UZRand for generating skewed Zipfian inputs. Skewness can be controlled by the 'zipf' shell command.

// initial skew values for load imbalance
vector<int> b_imbalance_lower;
vector<int> b_imbalance_upper;
vector<int> t_imbalance_lower;
vector<int> t_imbalance_upper;
vector<int> a_imbalance_lower;
vector<int> a_imbalance_upper;
int _load_imbalance = 0;
bool _change_load = false;

/* ------------------- */
/* --- ACCT_UPDATE --- */
/* ------------------- */


acct_update_input_t create_acct_update_input(int sf, 
                                             int specificBr)
{
    assert (sf>0);

    acct_update_input_t auin;

    bool b_set = false;
    bool t_set = false;
    bool a_set = false;
    
    if(_change_load) {

	// branches
	int rand = URand(1,100);
	assert(rand >= 0);
     	assert(b_imbalance_upper.size() == b_imbalance_lower.size());
	int load = _load_imbalance / b_imbalance_lower.size();
	for(int i=0; !b_set && i<b_imbalance_upper.size(); i++) {
	    if(rand < load * (i+1)) {
		auin.b_id = URand(b_imbalance_lower[i],b_imbalance_upper[i]);
		b_set = true;
	    }
	}
	
        // tellers
	rand = URand(1,100);
	assert(rand >= 0);
     	assert(t_imbalance_upper.size() == t_imbalance_lower.size());
	load = _load_imbalance / t_imbalance_lower.size();
	for(int i=0; !t_set && i<t_imbalance_upper.size(); i++) {
	    if(rand < load * (i+1)) {
		auin.t_id = (auin.b_id * TPCB_TELLERS_PER_BRANCH) +
		    URand(t_imbalance_lower[i], t_imbalance_upper[i]);
		t_set = true;
	    }
	}

	// accounts
	rand = URand(1,100);
	assert(rand >= 0);
     	assert(a_imbalance_upper.size() == a_imbalance_lower.size());
	load = _load_imbalance / a_imbalance_lower.size();
	int account = 0;
	for(int i=0; a_set && i<a_imbalance_upper.size(); i++) {
	    if(rand < load * (i+1)) {
		account = URand(a_imbalance_lower[i], a_imbalance_upper[i]);
		a_set = true;
	    }
	}
	// 85 - 15 local Branch
	if (URand(0,100)>LOCAL_TPCB) {
	    // remote branch
	    auin.a_id = (URand(0,sf)*TPCB_ACCOUNTS_PER_BRANCH) + account;
	}
	else {
	    // local branch
	    auin.a_id = (auin.b_id*TPCB_ACCOUNTS_PER_BRANCH) + account;
	}
	
    }

    if(!b_set) {
	// The TPC-B branches start from 0 (0..SF-1)
	if (specificBr>0) {
	    auin.b_id = specificBr-1;
	}
	else {
	    auin.b_id = UZRand(0,sf-1);
	}
    }

    if(!t_set) {
	auin.t_id = (auin.b_id * TPCB_TELLERS_PER_BRANCH) + UZRand(0,TPCB_TELLERS_PER_BRANCH-1);
    }

    if(!a_set) {
	// 85 - 15 local Branch
	if (URand(0,100)>LOCAL_TPCB) {
	    // remote branch
	    auin.a_id = (URand(0,sf)*TPCB_ACCOUNTS_PER_BRANCH) + UZRand(0,TPCB_ACCOUNTS_PER_BRANCH-1);
	}
	else {
	    // local branch
	    auin.a_id = (auin.b_id*TPCB_ACCOUNTS_PER_BRANCH) + UZRand(0,TPCB_ACCOUNTS_PER_BRANCH-1);
	}
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




/* -------------------------- */
/* --- MBENCH_INSERT_ONLY --- */
/* -------------------------- */

volatile unsigned long balance = 0; // TODO: not used not, to be removed later

mbench_insert_only_input_t create_mbench_insert_only_input(int sf, 
							   int specificBr)
{
    assert (sf>0);
    
    mbench_insert_only_input_t mioin;

    //    atomic_inc_64_nv(&balance);

    // The TPC-B branches start from 0 (0..SF-1)
    if (specificBr>0) {
        mioin.b_id = specificBr-1;
    }
    else {
        mioin.b_id = UZRand(0,sf-1);
    }
        
    // local branch
    mioin.a_id = (mioin.b_id*TPCB_ACCOUNTS_PER_BRANCH) + UZRand(0,TPCB_ACCOUNTS_PER_BRANCH-1);
            
    mioin.balance = URand(0,200000000) - 100000000;
        
    return (mioin);
}

void mbench_insert_only_input_t::print()
{
    TRACE( TRACE_ALWAYS, "%d %d %.2f\n", b_id, a_id, balance);
}



/* -------------------------- */
/* --- MBENCH_DELETE_ONLY --- */
/* -------------------------- */


mbench_delete_only_input_t create_mbench_delete_only_input(int sf, 
							   int specificBr)
{
    assert (sf>0);
    
    mbench_delete_only_input_t mdoin;

    // The TPC-B branches start from 0 (0..SF-1)
    if (specificBr>0) {
        mdoin.b_id = specificBr-1;
    }
    else {
        mdoin.b_id = UZRand(0,sf-1);
    }
        
    // local branch
    mdoin.a_id = (mdoin.b_id*TPCB_ACCOUNTS_PER_BRANCH) + UZRand(0,TPCB_ACCOUNTS_PER_BRANCH-1);
            
    mdoin.balance = URand(0,2000000) - 1000000;

    return (mdoin);
}

void mbench_delete_only_input_t::print()
{
    TRACE( TRACE_ALWAYS, "%d %d %.2f\n", b_id, a_id, balance);
}




/* ------------------------- */
/* --- MBENCH_PROBE_ONLY --- */
/* ------------------------- */


mbench_probe_only_input_t create_mbench_probe_only_input(int sf, 
							 int specificBr)
{
    assert (sf>0);
    
    mbench_probe_only_input_t mpoin;

    // The TPC-B branches start from 0 (0..SF-1)
    if (specificBr>0) {
        mpoin.b_id = specificBr-1;
    }
    else {
        mpoin.b_id = UZRand(0,sf-1);
    }
        
    // local branch
    mpoin.a_id = (mpoin.b_id*TPCB_ACCOUNTS_PER_BRANCH) + UZRand(0,TPCB_ACCOUNTS_PER_BRANCH-1);
            
    mpoin.balance = URand(0,200000000) - 100000000;

    return (mpoin);
}

void mbench_probe_only_input_t::print()
{
    TRACE( TRACE_ALWAYS, "%d %d %.2f\n", b_id, a_id, balance);
}




/* ---------------------------- */
/* --- MBENCH_INSERT_DELETE --- */
/* ---------------------------- */


mbench_insert_delete_input_t create_mbench_insert_delete_input(int sf, 
							       int specificBr)
{
    assert (sf>0);
    
    mbench_insert_delete_input_t midin;
    mbench_insert_only_input_t mioin;
    mioin = create_mbench_insert_only_input(sf, specificBr);
    midin.b_id = mioin.b_id;
    midin.a_id = mioin.a_id;
    midin.balance = mioin.balance;

    return (midin);
}




/* --------------------------- */
/* --- MBENCH_INSERT_PROBE --- */
/* --------------------------- */


mbench_insert_probe_input_t create_mbench_insert_probe_input(int sf, 
							     int specificBr)
{
    assert (sf>0);
    
    mbench_insert_probe_input_t mipin;
    mbench_insert_only_input_t mioin;
    mioin = create_mbench_insert_only_input(sf, specificBr);
    mipin.b_id = mioin.b_id;
    mipin.a_id = mioin.a_id;
    mipin.balance = mioin.balance;

    return (mipin);
}




/* --------------------------- */
/* --- MBENCH_DELETE_PROBE --- */
/* --------------------------- */


mbench_delete_probe_input_t create_mbench_delete_probe_input(int sf, 
							     int specificBr)
{
    assert (sf>0);
    
    mbench_delete_probe_input_t mdpin;
    mbench_delete_only_input_t mdoin;
    mdoin = create_mbench_delete_only_input(sf, specificBr);
    mdpin.b_id = mdoin.b_id;
    mdpin.a_id = mdoin.a_id;
    mdpin.balance = mdoin.balance;


    return (mdpin);
}




/* ------------------ */
/* --- MBENCH_MIX --- */
/* ------------------ */


mbench_mix_input_t create_mbench_mix_input(int sf, 
					   int specificBr)
{
    assert (sf>0);
    
    mbench_mix_input_t mmin;
    mbench_insert_only_input_t mioin;
    mioin = create_mbench_insert_only_input(sf, specificBr);
    mmin.b_id = mioin.b_id;
    mmin.a_id = mioin.a_id;
    mmin.balance = mioin.balance;

    return (mmin);
}




EXIT_NAMESPACE(tpcb);
