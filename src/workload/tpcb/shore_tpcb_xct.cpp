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

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcb_env.cpp
 *
 *  @brief:  Implementation of the Baseline Shore TPC-B transactions
 *
 *  @author: Ryan Johnson, Feb 2009
 *  @author: Ippokratis Pandis, Feb 2009
 */

#include "workload/tpcb/shore_tpcb_env.h"

#include <vector>
#include <numeric>
#include <algorithm>

using namespace shore;


ENTER_NAMESPACE(tpcb);



/******************************************************************** 
 *
 * Thread-local TPC-B TRXS Stats
 *
 ********************************************************************/

static __thread ShoreTPCBTrxStats my_stats;

void ShoreTPCBEnv::env_thread_init()
{
    CRITICAL_SECTION(stat_mutex_cs, _statmap_mutex);
    _statmap[pthread_self()] = &my_stats;
}

void ShoreTPCBEnv::env_thread_fini()
{
    CRITICAL_SECTION(stat_mutex_cs, _statmap_mutex);
    _statmap.erase(pthread_self());
}


/******************************************************************** 
 *
 *  @fn:    _get_stats
 *
 *  @brief: Returns a structure with the currently stats
 *
 ********************************************************************/

const ShoreTPCBTrxStats ShoreTPCBEnv::_get_stats()
{
    CRITICAL_SECTION(cs, _statmap_mutex);
    ShoreTPCBTrxStats rval;
    rval -= rval; // dirty hack to set all zeros
    for (statmap_t::iterator it=_statmap.begin(); it != _statmap.end(); ++it) 
	rval += *it->second;
    return (rval);
}


/******************************************************************** 
 *
 *  @fn:    reset_stats
 *
 *  @brief: Updates the last gathered statistics
 *
 ********************************************************************/

void ShoreTPCBEnv::reset_stats()
{
    CRITICAL_SECTION(last_stats_cs, _last_stats_mutex);
    _last_stats = _get_stats();
}


/******************************************************************** 
 *
 *  @fn:    print_throughput
 *
 *  @brief: Prints the throughput given a measurement delay
 *
 ********************************************************************/

void ShoreTPCBEnv::print_throughput(const int iQueriedSF, 
                                    const int iSpread, 
                                    const int iNumOfThreads, 
                                    const double delay,
                                    const double avgcpuusage)
{
    CRITICAL_SECTION(last_stats_cs, _last_stats_mutex);

    // get the current statistics
    ShoreTPCBTrxStats current_stats = _get_stats();
    
    // now calculate the diff
    current_stats -= _last_stats;
        
    int trxs_att  = current_stats.attempted.total();
    int trxs_abt  = current_stats.failed.total();
    int trxs_dld  = current_stats.deadlocked.total();

    TRACE( TRACE_ALWAYS, "*******\n"             \
           "QueriedSF: (%d)\n"                   \
           "Spread:    (%s)\n"                   \
           "Threads:   (%d)\n"                   \
           "Trxs Att:  (%d)\n"                   \
           "Trxs Abt:  (%d)\n"                   \
           "Trxs Dld:  (%d)\n"                   \
           "Secs:      (%.2f)\n"                 \
           "AvgCPUs:   (%.1f)\n"                 \
           "TPS:       (%.2f)\n",
           iQueriedSF, 
           (iSpread ? "Yes" : "No"),
           iNumOfThreads, trxs_att, trxs_abt, trxs_dld,
           delay, avgcpuusage,
           (trxs_att-trxs_abt-trxs_dld)/delay);
}






/******************************************************************** 
 *
 * TPC-C TRXS
 *
 * (1) The run_XXX functions are wrappers to the real transactions
 * (2) The xct_XXX functions are the implementation of the transactions
 *
 ********************************************************************/


/********************************************************************* 
 *
 *  @fn:    run_one_xct
 *
 *  @brief: Baseline client - Entry point for running one trx 
 *
 *  @note:  The execution of this trx will not be stopped even if the
 *          measure internal has expired.
 *
 *********************************************************************/

w_rc_t ShoreTPCBEnv::run_one_xct(const int xctid,
                                 int xct_type,
                                 const int whid, 
                                 trx_result_tuple_t& trt)
{
    switch (xct_type) {
    case XCT_TPCB_ACCT_UPDATE:
	return run_acct_update(xctid, trt, whid);
    default:
        assert (0); // UNKNOWN TRX-ID
    }
    return (RCOK);
}



/******************************************************************** 
 *
 * TPC-C TRXs Wrappers
 *
 * @brief: They are wrappers to the functions that execute the transaction
 *         body. Their responsibility is to:
 *
 *         1. Prepare the corresponding input
 *         2. Check the return of the trx function and abort the trx,
 *            if something went wrong
 *         3. Update the tpcb db environment statistics
 *
 ********************************************************************/


DEFINE_TRX(ShoreTPCBEnv,acct_update);
DEFINE_TRX(ShoreTPCBEnv,populate_db);



// uncomment the line below if want to dump (part of) the trx results
//#define PRINT_TRX_RESULTS


/******************************************************************** 
 *
 * TPC-B Acct Update
 *
 ********************************************************************/
w_rc_t ShoreTPCBEnv::xct_acct_update(const int xct_id, 
                                     trx_result_tuple_t& trt,
                                     acct_update_input_t& ppin)
{
    w_rc_t e = RCOK;

    // ensure a valid environment    
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // account update trx touches 4 tables:
    // branch, teller, account, and history -- just like TPC-C payment:
    // branch, teller, account, and history

    // get table tuples from the caches
    row_impl<branch_t>* prb = _pbranch_man->get_tuple();
    assert (prb);

    row_impl<teller_t>* prt = _pteller_man->get_tuple();
    assert (prt);

    row_impl<account_t>* pracct = _paccount_man->get_tuple();
    assert (pracct);

    row_impl<history_t>* prhist = _phistory_man->get_tuple();
    assert (prhist);


    trt.set_id(xct_id);
    trt.set_state(UNSUBMITTED);
    rep_row_t areprow(_paccount_man->ts());

    // allocate space for the biggest of the 4 table representations
    areprow.set(_paccount_desc->maxsize()); 

    prb->_rep = &areprow;
    prt->_rep = &areprow;
    pracct->_rep = &areprow;
    prhist->_rep = &areprow;

//     /* 0. initiate transaction */
//     W_DO(_pssm->begin_xct());

    { // make gotos safe

	double total;
	
	/*
	  1. Update account
	*/
	e = _paccount_man->a_index_probe_forupdate(_pssm, pracct, ppin.a_id);
        if (e.is_error()) { goto done; }

	pracct->get_value(2, total);
	pracct->set_value(2, total + ppin.delta);
	e = _paccount_man->update_tuple(_pssm, pracct);
	if (e.is_error()) { goto done; }

	
	/*
	  2. Write to History
	*/
	prhist->set_value(0, ppin.b_id);
	prhist->set_value(1, ppin.t_id);
	prhist->set_value(2, ppin.a_id);
	prhist->set_value(3, ppin.delta);
	prhist->set_value(4, time(NULL));
	prhist->set_value(5, "padding");
        e = _phistory_man->add_tuple(_pssm, prhist);
        if (e.is_error()) { goto done; }

	
	/*
	  3. Update teller
	 */
	e = _pteller_man->t_index_probe_forupdate(_pssm, prt, ppin.t_id);
        if (e.is_error()) { goto done; }

	prt->get_value(2, total);
	prt->set_value(2, total + ppin.delta);
	e = _pteller_man->update_tuple(_pssm, prt);
	if (e.is_error()) { goto done; }

	
	/*
	  4. Update branch
	 */
	e = _pbranch_man->b_index_probe_forupdate(_pssm, prb, ppin.b_id);
        if (e.is_error()) { goto done; }

	prb->get_value(1, total);
	prb->set_value(1, total + ppin.delta);
	e = _pbranch_man->update_tuple(_pssm, prb);
	if (e.is_error()) { goto done; }
	


        /*
	  5. commit
	*/
        e = _pssm->commit_xct();
        if (e.is_error()) { goto done; }

    } // goto

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);


#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prb->print_tuple();
    prt->print_tuple();
    pracct->print_tuple();
    prhist->print_tuple();
#endif


done:
    // return the tuples to the cache
    _pbranch_man->give_tuple(prb);
    _pteller_man->give_tuple(prt);
    _paccount_man->give_tuple(pracct);
    _phistory_man->give_tuple(prhist);
    return (e);

} // EOF: ACCT UPDATE





w_rc_t ShoreTPCBEnv::xct_populate_db(const int xct_id, 
				     trx_result_tuple_t& trt,
                                     populate_db_input_t& ppin)
{
    w_rc_t e = RCOK;

    // ensure a valid environment    
    assert (_pssm);
    assert (_initialized);
    // we probably are *not* loaded!
    //assert (_loaded);

    // account update trx touches 4 tables:
    // branch, teller, account, and history -- just like TPC-C payment:

    // get table tuples from the caches
    row_impl<branch_t>* prb = _pbranch_man->get_tuple();
    assert (prb);

    row_impl<teller_t>* prt = _pteller_man->get_tuple();
    assert (prt);

    row_impl<account_t>* pracct = _paccount_man->get_tuple();
    assert (pracct);

    row_impl<history_t>* prhist = _phistory_man->get_tuple();
    assert (prhist);

    trt.set_id(xct_id);
    trt.set_state(UNSUBMITTED);
    rep_row_t areprow(_paccount_man->ts());

    // allocate space for the biggest of the 4 table representations
    areprow.set(_paccount_desc->maxsize()); 

    prb->_rep = &areprow;
    prt->_rep = &areprow;
    pracct->_rep = &areprow;
    prhist->_rep = &areprow;

//     /* 0. initiate transaction */
//     W_DO(_pssm->begin_xct());

    { // make gotos safe

	if(ppin._first_a_id < 0) {
	    /*
	      Populate the branches and tellers
	    */
	    for(int i=0; i < ppin._sf; i++) {
		prb->set_value(0, i);
		prb->set_value(1, 0.0);
		prb->set_value(2, "padding");
		e = _pbranch_man->add_tuple(_pssm, prb);
		if (e.is_error()) { goto done; }
	    }
	    fprintf(stderr, "Loaded %d branches\n", ppin._sf);
	    
	    for(int i=0; i < ppin._sf*TPCB_TELLERS_PER_BRANCH; i++) {
		prt->set_value(0, i);
		prt->set_value(1, i/TPCB_TELLERS_PER_BRANCH);
		prt->set_value(2, 0.0);
		prt->set_value(3, "padding");
		e = _pteller_man->add_tuple(_pssm, prt);
		if (e.is_error()) { goto done; }
	    }
	    fprintf(stderr, "Loaded %d tellers\n", ppin._sf*TPCB_TELLERS_PER_BRANCH);
	}
	else {
	    /*
	      Populate 10k accounts
	    */
	    for(int i=0; i < TPCB_ACCOUNTS_CREATED_PER_POP_XCT; i++) {
		int a_id = ppin._first_a_id + (TPCB_ACCOUNTS_CREATED_PER_POP_XCT-i-1);
		pracct->set_value(0, a_id);
		pracct->set_value(1, a_id/TPCB_ACCOUNTS_PER_BRANCH);
		pracct->set_value(2, 0.0);
		pracct->set_value(3, "padding");
		e = _paccount_man->add_tuple(_pssm, pracct);
		if (e.is_error()) { goto done; }
	    }
	}


        /*
	  Commit
	*/
        e = _pssm->commit_xct();
        if (e.is_error()) { goto done; }

    } // goto

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);


#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prb->print_tuple();
    prt->print_tuple();
    pracct->print_tuple();
    prhist->print_tuple();
#endif


done:
    // return the tuples to the cache
    _pbranch_man->give_tuple(prb);
    _pteller_man->give_tuple(prt);
    _paccount_man->give_tuple(pracct);
    _phistory_man->give_tuple(prhist);
    return (e);

} // EOF: ACCT UPDATE





EXIT_NAMESPACE(tpcb);
