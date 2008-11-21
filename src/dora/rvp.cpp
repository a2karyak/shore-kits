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
    }
    return (*this);
}

/****************************************************************** 
 *
 * @fn:    add_action()
 *
 * @brief: Adds an action to the list of actions of a rvp
 *
 ******************************************************************/

const int rvp_t::copy_actions(baseActionsList& actionList)
{
    _actions = actionList;
    //return (_actions.size());
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
    assert (paction);
    assert (this==paction->rvp());
    _actions.push_back(paction);
    //return (_actions.size());
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
    assert (_xct);
    assert (penv);

    // attach to this xct
    smthread_t::me()->attach_xct(_xct);

    // try to commit
    w_rc_t ecommit = penv->db()->commit_xct();
    
    if (ecommit.is_error()) {
        TRACE( TRACE_ALWAYS, "Xct (%d) commit failed [0x%x]\n",
               _tid, ecommit.err_num());

        upd_aborted_stats(); // hook - update aborted stats

        w_rc_t eabort = penv->db()->abort_xct();
        if (eabort.is_error()) {
            TRACE( TRACE_ALWAYS, "Xct (%d) abort failed [0x%x]\n",
                   _tid, eabort.err_num());
        }

	// signal cond var
	if(_result.get_notify())
	    _result.get_notify()->signal();
        return (ecommit);
    }
    upd_committed_stats(); // hook - update committed stats
    
    TRACE( TRACE_TRX_FLOW, "Xct (%d) completed\n", _tid);

    // signal cond var
    if(_result.get_notify())
	_result.get_notify()->signal();
    return (RCOK);
}
