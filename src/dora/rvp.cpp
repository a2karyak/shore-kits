/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   lockman.cpp
 *
 *  @brief:  Implementation of Rendezvous points for DORA
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */

#include "dora/rvp.h"
#include "dora/action.h"


using namespace dora;



/****************************************************************** 
 *
 * @fn:    assignment operator
 *
 ******************************************************************/

rvp_t& rvp_t::operator=(const rvp_t& rhs)
{
    if (this != &rhs) {
        _set(rhs._tid,rhs._xct,rhs._xct_id,rhs._result,
             rhs._countdown.remaining(),rhs._actions.size());
        copy_actions(rhs._actions);
    }
    return (*this);
}

/****************************************************************** 
 *
 * @fn:    copy_actions()
 *
 * @brief: Initiates the list of actions of a rvp
 *
 ******************************************************************/

const int rvp_t::copy_actions(const baseActionsList& actionList)
{
    _actions.reserve(actionList.size());
    _actions.assign(actionList.begin(),actionList.end()); // copy list content
    return (0);
}


/****************************************************************** 
 *
 * @fn:    append_actions()
 *
 * @brief: Appends a list of actions to the list of actions of a rvp
 *
 * @note:  Thread-safe
 *
 ******************************************************************/

const int rvp_t::append_actions(const baseActionsList& actionList)
{
    CRITICAL_SECTION(action_cs, _decision_lock);
    // append new actionList to the end of the list
    _actions.insert(_actions.end(), actionList.begin(),actionList.end()); 
    return (0);
}


/****************************************************************** 
 *
 * @fn:    add_action()
 *
 * @brief: Adds an action to the list of actions of a rvp
 *
 ******************************************************************/

const int rvp_t::add_action(base_action_t* paction) 
{
    w_assert3 (paction);
    w_assert3 (this==paction->rvp());
    _actions.push_back(paction);
    return (0);
}


/****************************************************************** 
 *
 * @fn:    notify()
 *
 * @brief: Notifies for any committed actions
 *
 ******************************************************************/

const int terminal_rvp_t::notify()
{
    for (baseActionsIt it=_actions.begin(); it!=_actions.end(); ++it)
        (*it)->notify();
    return (_actions.size());
}



/****************************************************************** 
 *
 * @fn:    _run()
 *
 * @brief: Code executed at terminal rendez-vous point
 *
 * @note:  The default action is to commit. However, it may be overlapped
 * @note:  There are hooks for updating the correct stats
 *
 ******************************************************************/

w_rc_t terminal_rvp_t::_run(ShoreEnv* penv)
{
    w_assert3 (penv);

    // attach to this xct
#ifndef ONLYDORA
    w_assert3 (_xct);
    smthread_t::me()->attach_xct(_xct);
#endif

    // try to commit    
    w_rc_t rcdec;
    if (_decision == AD_ABORT) {

#ifndef ONLYDORA
        rcdec = penv->db()->abort_xct();
#endif

        if (rcdec.is_error()) {
            TRACE( TRACE_ALWAYS, "Xct (%d) abort failed [0x%x]\n",
                   _tid, rcdec.err_num());
        }
        else {
            TRACE( TRACE_TRX_FLOW, "Xct (%d) aborted\n", _tid);
            upd_aborted_stats(); // hook - update aborted stats
        }
    }
    else {

#ifndef ONLYDORA
        rcdec = penv->db()->commit_xct();    
#endif

        if (rcdec.is_error()) {
            TRACE( TRACE_ALWAYS, "Xct (%d) commit failed [0x%x]\n",
                   _tid, rcdec.err_num());
            upd_aborted_stats(); // hook - update aborted stats

#ifndef ONLYDORA
            w_rc_t eabort = penv->db()->abort_xct();
            if (eabort.is_error()) {
                TRACE( TRACE_ALWAYS, "Xct (%d) abort failed [0x%x]\n",
                       _tid, eabort.err_num());
            }
#endif
        }
        else {
            TRACE( TRACE_TRX_FLOW, "Xct (%d) committed\n", _tid);
            upd_committed_stats(); // hook - update committed stats
        }
    }               

    // signal cond var
    if(_result.get_notify()) {
        TRACE( TRACE_TRX_FLOW, "Xct (%d) notifying client (%x)\n", _tid, _result.get_notify());
	_result.get_notify()->signal();
    }
    else
        TRACE( TRACE_TRX_FLOW, "Xct (%d) not notifying client\n", _tid);
    return (rcdec);
}
