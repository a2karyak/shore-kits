/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file rvp.h
 *
 *  @brief Rendezvous points
 *
 *  @author Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_RVP_H
#define __DORA_RVP_H

#include "util/countdown.h"

#include "dora.h"

#include "core/trx_packet.h"

using namespace qpipe;


ENTER_NAMESPACE(dora);


/******************************************************************** 
 *
 * @class: rvp_t
 *
 * @brief: Abstract class for rendez-vous points
 *
 * @note:  This point will contain information only needed to be
 *         communication among different phases of the trx
 *
 ********************************************************************/

class rvp_t
{
protected:

    // the countdown
    countdown_t              _countdown;
    eActionDecision volatile _decision;
    tatas_lock               _decision_lock;

    // trx-specific
    xct_t*              _xct; // Not the owner
    tid_t               _tid;
    trx_result_tuple_t _result;

public:

    rvp_t(tid_t atid, xct_t* axct,
          trx_result_tuple_t &presult, 
          const int intra_trx_cnt) 
        : _tid(atid), _xct(axct), _result(presult), _countdown(intra_trx_cnt),
          _attach_cnt(0),
          _decision(AD_UNDECIDED)
    { 
        assert (_xct);
        assert (intra_trx_cnt>0);
    }

    virtual ~rvp_t() { }
    
    /** access methods */

    // TRX-ID Related

    inline xct_t* get_xct() { return (_xct); }
    inline tid_t  get_tid() { return (_tid); }
   
    inline void set(tid_t atid, xct_t* axct, trx_result_tuple_t &presult) {
        assert (axct);
        assert (_tid == ss_m::xct_to_tid(axct));
        _tid = atid;
        _xct = axct;
        _result = presult;
    }

    // Decision related

    inline void set_decision(const eActionDecision& ad) { _decision = ad; }
    inline eActionDecision get_decision() { return (_decision); }

    /** trx-related operations */
    virtual w_rc_t run()=0; // default action on rvp - commit trx

    virtual void cleanup()=0; // does any clean up

    bool post(bool is_error=false) { 
        assert (_countdown.remaining()); // before posting check if there is to post 
        return (_countdown.post(is_error)); 
    }

    // for the cache
    void reset() {
        assert (0); // should not be called if not from cache 
        TRACE( TRACE_DEBUG, "Reseting (%d)\n", _tid);
        _xct = NULL;
        _decision = AD_UNDECIDED;
        _result = trx_result_tuple_t();
    }    


    // hack to make the intermediate RVPs be detached
    virtual w_rc_t release(thread_t* aworker) { 
        aworker->detach_xct(_xct);
        return (RCOK); 
    }

    // decides to abort this trx
    virtual eActionDecision abort() { 
        CRITICAL_SECTION(decision_cs, _decision_lock);
        _decision = AD_ABORT;
        return (_decision);
    }


    // coordinates detach sequence between threads of the same trx
    tatas_lock     _detach_lock; // enforces order across detaches


    // keeps track on the number of attached threads - not used
    int volatile   _attach_cnt;  // enforces at least one thread to be attached
    tatas_lock     _attach_lock; // enforces order across detaches
    const int get_attached() {
        CRITICAL_SECTION(attach_cs, _attach_lock);
        return (_attach_cnt);
    }
    const int inc_attached() {
        CRITICAL_SECTION(attach_cs, _attach_lock);
        ++_attach_cnt;
        return (_attach_cnt);
    }

private:

    // copying not allowed
    rvp_t(rvp_t const &);
    void operator=(rvp_t const &);
    
}; // EOF: rvp_t



/******************************************************************** 
 *
 * @class: terminal_rvp_t
 *
 * @brief: Abstract class for the terminal (commiting) rendez-vous points
 *
 * @note:  This point will try to commit and update the appropriate stats
 *
 ********************************************************************/

class terminal_rvp_t : public rvp_t
{
public:

    terminal_rvp_t(tid_t atid, xct_t* axct, 
                   trx_result_tuple_t &presult, 
                   const int intra_trx_cnt) 
        : rvp_t(atid, axct, presult, intra_trx_cnt) 
    { }

    virtual ~terminal_rvp_t() { }
    
    virtual void cleanup()=0; // does any clean up

    /** trx-related operations */
    virtual w_rc_t run()=0; // default action on rvp - commit trx    

    virtual void upd_committed_stats()=0; // update the committed trx stats
    virtual void upd_aborted_stats()=0;   // update the committed trx stats

    // hack to make the intermediate RVPs be detached
    w_rc_t release(thread_t* aworker) { return (RCOK); } // does nothing in case of terminal rvp

protected:

    w_rc_t _run(ShoreEnv* penv);

}; // EOF: terminal_rvp_t



EXIT_NAMESPACE(dora);

#endif /** __DORA_RVP_H */

