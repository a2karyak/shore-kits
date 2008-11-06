/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file rvp.h
 *
 *  @brief Rendezvous points
 *
 *  @author Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_RVP_H
#define __DORA_RVP_H

#include "sm/shore/shore_env.h"
#include "core/trx_packet.h"

#include "util/countdown.h"

#include "dora/common.h"

using namespace qpipe;
using namespace shore;


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
public:

    typedef vector<base_action_t*> baseActionsList;
    typedef baseActionsList::iterator baseActionsIt;

protected:

    // the countdown
    countdown_t              _countdown;
    eActionDecision volatile _decision;
    tatas_lock               _decision_lock;

    // trx-specific
    xct_t*              _xct; // Not the owner
    tid_t               _tid;
    trx_result_tuple_t  _result;
    int                 _xct_id;

    // list of actions that report to this rvp
    baseActionsList     _actions;

public:

    rvp_t(tid_t atid, xct_t* axct, const int axctid,
          trx_result_tuple_t& presult, const int intra_trx_cnt) 
        : _countdown(intra_trx_cnt), _decision(AD_UNDECIDED),
          _tid(atid), _xct(axct), _xct_id(axctid),
          _result(presult)
    { 
        assert (_xct);
        assert (intra_trx_cnt>0);
    }

    virtual ~rvp_t() { }
    

    // access methods

    // TRX-ID-related
    inline xct_t* get_xct() const { return (_xct); }
    inline tid_t  get_tid() const { return (_tid); }

    // Actions-related
    const int add_action(base_action_t* paction);

    // Decision-related
    inline void set_decision(const eActionDecision& ad) { _decision = ad; }
    inline eActionDecision get_decision() const { return (_decision); }


    // interface
    // default action on rvp - commit trx
    virtual w_rc_t run()=0;  

    // notifies for any committed actions 
    // only the final_rvps may notify
    // for midway rvps it should be a noop
    virtual const int notify() { return(0); } 

    bool post(bool is_error=false) { 
        //assert (_countdown.remaining()); // before posting check if there is to post 
        return (_countdown.post(is_error)); 
    }

    // decides to abort this trx
    virtual eActionDecision abort() { 
        CRITICAL_SECTION(decision_cs, _decision_lock);
        _decision = AD_ABORT;
        return (_decision);
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

    terminal_rvp_t(tid_t atid, xct_t* axct, const int axctid, 
                   trx_result_tuple_t &presult, const int intra_trx_cnt) 
        : rvp_t(atid, axct, axctid, presult, intra_trx_cnt) 
    { }

    virtual ~terminal_rvp_t() { }

    // interface
    virtual w_rc_t run()=0;   // default action on rvp - commit trx           
    const int notify();       // notifies for committed actions

    virtual void upd_committed_stats()=0; // update the committed trx stats
    virtual void upd_aborted_stats()=0;   // update the committed trx stats

protected:

    w_rc_t _run(ShoreEnv* penv);

}; // EOF: terminal_rvp_t



EXIT_NAMESPACE(dora);

#endif /** __DORA_RVP_H */

