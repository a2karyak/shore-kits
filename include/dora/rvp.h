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

/** @file:   rvp.h
 *
 *  @brief:  Rendezvous points
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_RVP_H
#define __DORA_RVP_H


#include "sm/shore/shore_reqs.h"
#include "sm/shore/shore_env.h"
#include "sm/shore/shore_trx_worker.h"

#include "util/countdown.h"

#include "dora/common.h"
#include "dora/base_action.h"

using namespace shore;

ENTER_NAMESPACE(dora);

class DoraEnv;


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

struct rvp_t : public base_request_t
{
public:

    //typedef PooledVec<base_action_t*>::Type    baseActionsList;
    typedef std::vector<dora::base_action_t*>    baseActionsList;
    typedef baseActionsList::iterator baseActionsIt;

protected:

    // the countdown
    countdown_t       _countdown;
    ushort_t volatile _decision;

    // list of actions that report to this rvp
    baseActionsList   _actions;
    tatas_lock        _actions_lock;

    void _set(xct_t* pxct, const tid_t& atid, const int axctid,
              const trx_result_tuple_t& presult, 
              const int intra_trx_cnt, const int total_actions) 
    { 
        base_request_t::set(pxct,atid,axctid,presult);

#ifndef ONLYDORA
        assert (pxct);
#endif

        assert (intra_trx_cnt>0);
        assert (total_actions>=intra_trx_cnt);
        _countdown.reset(intra_trx_cnt);
        _decision = AD_UNDECIDED;

        _actions.reserve(total_actions);
    }

public:

    rvp_t();

    rvp_t(xct_t* axct, const tid_t& atid, const int axctid,
          const trx_result_tuple_t& presult, 
          const int intra_trx_cnt, const int total_actions);

    virtual ~rvp_t() { }    

    // copying allowed
    rvp_t(const rvp_t& rhs);
    rvp_t& operator=(const rvp_t& rhs);

    trx_result_tuple_t result() { return (_result); }

    // Actions-related
    int copy_actions(const baseActionsList& actionList);
    int append_actions(const baseActionsList& actionList);
    int add_action(dora::base_action_t* paction);

    inline bool post(bool is_error=false) { 
        if (is_error) abort();        
        return (_countdown.post(false)); 
    }

    // decides to abort this trx
    inline ushort_t abort() { 
        _decision = AD_ABORT;
        return (*&_decision);
    }

    inline bool isAborted() {
        return (*&_decision == AD_ABORT);
    }

    // INTERFACE 

    // default action on rvp - commit trx
    virtual w_rc_t run()=0;  

    // notifies for any committed actions 
    // only the final_rvps may notify
    // for midway rvps it should be a noop
    virtual int notify() { return(0); } 

//     // notifies the client
//     void notify_client();

    // should give memory back to the atomic trash stack
    virtual void giveback()=0;


    // CACHEABLE INTERFACE

    void init() 
    {
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

    terminal_rvp_t(xct_t* axct, const tid_t& atid, const int axctid, 
                   trx_result_tuple_t &presult, 
                   const int intra_trx_cnt, const int total_actions) 
        : rvp_t(axct, atid, axctid, presult, intra_trx_cnt, total_actions)
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


#ifdef CFG_FLUSHER
    void notify_on_abort();
#endif

    // INTERFACE
    virtual w_rc_t run()=0;   // default action on rvp - commit trx           
    int notify();       // notifies for committed actions    

    virtual void upd_committed_stats()=0; // update the committed trx stats
    virtual void upd_aborted_stats()=0;   // update the aborted trx stats

protected:

    w_rc_t _run(ss_m* db, DoraEnv* denv);

}; // EOF: terminal_rvp_t



EXIT_NAMESPACE(dora);

#endif /** __DORA_RVP_H */

