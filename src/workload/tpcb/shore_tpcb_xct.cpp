/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcb_env.cpp
 *
 *  @brief:  Implementation of the Baseline Shore TPC-C transactions
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#include "workload/tpcb/shore_tpcb_env.h"

#include "core/trx_packet.h"

#include <vector>
#include <numeric>
#include <algorithm>

using namespace shore;


ENTER_NAMESPACE(tpcb);

// based on long erand48(unsigned short xi[3] );
struct rng {
    unsigned short xi[3];
    rng() {
	for(int i=0; i < 3; i++)
	    xi[i] = (short)rand();
    }
    double get() { return erand48(xi); }
    int get_int(int max) { return (int) (get()*max); }
};

DECLARE_TLS(rng, my_rng);

acct_update_input_t::acct_update_input_t(int branches_queried, int specific_b_id)
    : b_id((specific_b_id < 0)? my_rng->get_int(branches_queried) : specific_b_id)
    , t_id(b_id*TELLERS_PER_BRANCH+my_rng->get_int(TELLERS_PER_BRANCH))
    , a_id(my_rng->get_int(ACCOUNTS_PER_BRANCH))
    , delta(my_rng->get_int(2000000)-1000000)
{
    int a_b_id = (my_rng->get() < .85)? b_id : my_rng->get_int(branches_queried);
    a_id += a_b_id*ACCOUNTS_PER_BRANCH;
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

w_rc_t ShoreTPCBEnv::run_one_xct(int xct_type, const int xctid, 
                                 const int whid, trx_result_tuple_t& trt)
{
    switch (xct_type) {
    case XCT_ACCT_UPDATE:
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

struct xct_stats {
    int attempted;
    int failed;
    xct_stats &operator+=(xct_stats const &other) {
	attempted += other.attempted;
	failed += other.failed;
	return *this;
    };
    xct_stats &operator-=(xct_stats const &other) {
	attempted -= other.attempted;
	failed -= other.failed;
	return *this;
    };
};

static __thread xct_stats my_stats;
typedef std::map<pthread_t, xct_stats*> statmap_t;
static statmap_t statmap;
static pthread_mutex_t statmap_lock = PTHREAD_MUTEX_INITIALIZER;

// hooks for worker_t::_work_ACTIVE_impl
extern void worker_thread_init() {
    CRITICAL_SECTION(cs, statmap_lock);
    statmap[pthread_self()] = &my_stats;
}
extern void worker_thread_fini() {
    CRITICAL_SECTION(cs, statmap_lock);
    statmap.erase(pthread_self());
}

// hook for the shell to get totals
extern xct_stats shell_get_xct_stats() {
    CRITICAL_SECTION(cs, statmap_lock);
    xct_stats rval;
    rval -= rval; // dirty hack to set all zeros
    for(statmap_t::iterator it=statmap.begin(); it != statmap.end(); ++it) 
	rval += *it->second;

    return rval;
}


/* --- with input specified --- */

w_rc_t ShoreTPCBEnv::run_acct_update(const int xct_id, 
                                 acct_update_input_t& apin,
                                 trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. ACCT UPDATE...\n", xct_id);     
    
    ++my_stats.attempted;
    w_rc_t e = xct_acct_update(&apin, xct_id, atrt);
    if (e.is_error()) {
	++my_stats.failed;
        TRACE( TRACE_TRX_FLOW, "Xct (%d) Acct Update aborted [0x%x]\n", 
               xct_id, e.err_num());

//         stringstream os;
//         os << e << ends;
//         string str = os.str();
//         TRACE( TRACE_ALWAYS, "\n%s\n", str.c_str());

	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) Acct Update abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

	// signal cond var
	if(atrt.get_notify())
	    atrt.get_notify()->signal();

        if (_measure!=MST_MEASURE) {
            return (e);
        }

//        _total_tpcb_stats.inc_pay_att();
//        _session_tpcb_stats.inc_pay_att();
        _env_stats.inc_trx_att();
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) Acct Update completed\n", xct_id);

    // signal cond var
    if(atrt.get_notify())
        atrt.get_notify()->signal();

    if (_measure!=MST_MEASURE) {
        return (RCOK); 
    }

    return (RCOK); 
}



/* --- without input specified --- */

w_rc_t ShoreTPCBEnv::run_acct_update(const int xct_id, 
                                 trx_result_tuple_t& atrt,
                                 int specificWH)
{
    acct_update_input_t pin(_queried_factor, specificWH);
    return (run_acct_update(xct_id, pin, atrt));
}


/******************************************************************** 
 *
 * @note: The functions below are private, the corresponding run_XXX are
 *        their public wrappers. The run_XXX are required because they
 *        do the trx abort in case something goes wrong inside the body
 *        of each of the transactions.
 *
 ********************************************************************/


// uncomment the line below if want to dump (part of) the trx results
//#define PRINT_TRX_RESULTS


/******************************************************************** 
 *
 * TPC-B Acct Update
 *
 ********************************************************************/
w_rc_t ShoreTPCBEnv::xct_acct_update(acct_update_input_t* ppin, 
                                 const int xct_id, 
                                 trx_result_tuple_t& trt)
{
    w_rc_t e = RCOK;

    // ensure a valid environment    
    assert (ppin);
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
	e = _paccount_man->a_index_probe_forupdate(_pssm, pracct, ppin->a_id);
        if (e.is_error()) { goto done; }

	pracct->get_value(2, total);
	pracct->set_value(2, total + ppin->delta);
	e = _paccount_man->update_tuple(_pssm, pracct);
	if (e.is_error()) { goto done; }

	
	/*
	  2. Write to History
	*/
	prhist->set_value(0, ppin->b_id);
	prhist->set_value(1, ppin->t_id);
	prhist->set_value(2, ppin->a_id);
	prhist->set_value(3, ppin->delta);
	prhist->set_value(4, time(NULL));
	prhist->set_value(5, "padding");
        e = _phistory_man->add_tuple(_pssm, prhist);
        if (e.is_error()) { goto done; }

	
	/*
	  3. Update teller
	 */
	e = _pteller_man->t_index_probe_forupdate(_pssm, prt, ppin->t_id);
        if (e.is_error()) { goto done; }

	prt->get_value(2, total);
	prt->set_value(2, total + ppin->delta);
	e = _pteller_man->update_tuple(_pssm, prt);
	if (e.is_error()) { goto done; }

	
	/*
	  4. Update branch
	 */
	e = _pbranch_man->b_index_probe_forupdate(_pssm, prb, ppin->b_id);
        if (e.is_error()) { goto done; }

	prb->get_value(1, total);
	prb->set_value(1, total + ppin->delta);
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





w_rc_t ShoreTPCBEnv::xct_populate_db(populate_db_input_t* ppin, 
				     const int xct_id, 
				     trx_result_tuple_t& trt)
{
    w_rc_t e = RCOK;

    // ensure a valid environment    
    assert (ppin);
    assert (_pssm);
    assert (_initialized);
    // we probably are *not* loaded!
    //assert (_loaded);

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

	if(ppin->_first_a_id < 0) {
	    /*
	      Populate the branches and tellers
	    */
	    for(int i=0; i < ppin->_sf; i++) {
		prb->set_value(0, i);
		prb->set_value(1, 0.0);
		prb->set_value(2, "padding");
		e = _pbranch_man->add_tuple(_pssm, prb);
		if (e.is_error()) { goto done; }
	    }
	    fprintf(stderr, "Loaded %d branches\n", ppin->_sf);
	    
	    for(int i=0; i < ppin->_sf*TELLERS_PER_BRANCH; i++) {
		prt->set_value(0, i);
		prt->set_value(1, i/TELLERS_PER_BRANCH);
		prt->set_value(2, 0.0);
		prt->set_value(3, "padding");
		e = _pteller_man->add_tuple(_pssm, prt);
		if (e.is_error()) { goto done; }
	    }
	    fprintf(stderr, "Loaded %d tellers\n", ppin->_sf*TELLERS_PER_BRANCH);
	}
	else {
	    /*
	      Populate 10k accounts
	    */
	    for(int i=0; i < ACCOUNTS_CREATED_PER_POP_XCT; i++) {
		int a_id = ppin->_first_a_id + (ACCOUNTS_CREATED_PER_POP_XCT-i-1);
		pracct->set_value(0, a_id);
		pracct->set_value(1, a_id/ACCOUNTS_PER_BRANCH);
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
