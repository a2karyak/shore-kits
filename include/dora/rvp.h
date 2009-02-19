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

#include "dora.h"

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

    //typedef PooledVec<base_action_t*>::Type    baseActionsList;
    typedef vector<base_action_t*>    baseActionsList;
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

    // set rvp
    void _set(const tid_t& atid, xct_t* axct, const int axctid,
              const trx_result_tuple_t& presult, 
              const int intra_trx_cnt, const int total_actions) 
    { 
#ifndef ONLYDORA
        assert (axct);
#endif
        w_assert3 (intra_trx_cnt>0);
        w_assert3 (total_actions>=intra_trx_cnt);
        _countdown.reset(intra_trx_cnt);
        _decision = AD_UNDECIDED;
        _tid = atid;
        _xct = axct;
        _xct_id = axctid;
        _result = presult;

        w_assert3 (_actions );
        _actions.reserve(total_actions);
    }

public:

    rvp_t() : _xct(NULL) { }

    rvp_t(const tid_t& atid, xct_t* axct, const int axctid,
          const trx_result_tuple_t& presult, 
          const int intra_trx_cnt, const int total_actions) 
    { 
        _set(atid, axct, axctid, presult,
             intra_trx_cnt, total_actions);
    }

    virtual ~rvp_t() { _xct = NULL; }    

    // copying allowed
    rvp_t(const rvp_t& rhs)
    {
        _set(rhs._tid, rhs._xct, rhs._xct_id, rhs._result,
             rhs._countdown.remaining(), rhs._actions.size());
        copy_actions(rhs._actions);
    }
    rvp_t& operator=(const rvp_t& rhs);

    trx_result_tuple_t result() { return (_result); }


    // TRX-ID-related
    inline xct_t* xct() const { return (_xct); }
    inline tid_t  tid() const { return (_tid); }

    // Actions-related
    const int copy_actions(const baseActionsList& actionList);
    const int append_actions(const baseActionsList& actionList);
    const int add_action(base_action_t* paction);

    // Decision-related
    inline void set_decision(const eActionDecision& ad) { _decision = ad; }
    inline eActionDecision get_decision() const { return (_decision); }


    inline bool post(bool is_error=false) { 
        if (is_error) {
            CRITICAL_SECTION(decision_cs,_decision_lock);
            _decision = AD_ABORT;
        }
        return (_countdown.post(false)); 
    }

    // decides to abort this trx
    virtual eActionDecision abort() { 
        CRITICAL_SECTION(decision_cs, _decision_lock);
        _decision = AD_ABORT;
        return (_decision);
    }


    // INTERFACE 

    // default action on rvp - commit trx
    virtual w_rc_t run()=0;  

    // notifies for any committed actions 
    // only the final_rvps may notify
    // for midway rvps it should be a noop
    virtual const int notify() { return(0); } 

    // should give memory back to the atomic trash stack
    virtual void giveback()=0;



    // CACHEABLE INTERFACE

    void setup(Pool** stl_pool_alloc_list) 
    {
        w_assert3 (stl_pool_alloc_list);

        // it must have 1 pool lists: 
        // stl_pool_alloc_list[0]: base_action_t* pool
        // assert (stl_pool_alloc_list[0]); 
        //_actions = new baseActionsList( stl_pool_alloc_list[0] );
    }

    void reset() 
    {
        // clear contents
        _actions.erase(_actions.begin(),_actions.end());
        _xct = NULL;
    }

    
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

    terminal_rvp_t() : rvp_t() { }

    terminal_rvp_t(const tid_t& atid, xct_t* axct, const int axctid, 
                   trx_result_tuple_t &presult, 
                   const int intra_trx_cnt, const int total_actions) 
        : rvp_t(atid, axct, axctid, presult, intra_trx_cnt, total_actions) 
    { }

    terminal_rvp_t(const terminal_rvp_t& rhs)
        : rvp_t(rhs)
    { }

    terminal_rvp_t& operator=(const terminal_rvp_t& rhs) 
    {
        terminal_rvp_t::operator=(rhs);
        return (*this);
    }

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

