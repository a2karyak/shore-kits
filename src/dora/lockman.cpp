/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   lockman.cpp
 *
 *  @brief:  Implementation of a Lock manager for DORA partitions
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#include "dora/lockman.h"
#include "dora/action.h"


ENTER_NAMESPACE(dora);


#undef LOCKDEBUG
#define LOCKDEBUG


/******************************************************************** 
 *
 * @struct: ActionLLEntry
 *
 ********************************************************************/ 


tid_t ActionLLEntry::tid() { 
    assert (_action); 
    return (_action->tid()); 
}


//// Helper functions


std::ostream& operator<<(std::ostream& os, const ActionLLEntry& rhs) 
{
    os << "(" << rhs._action->tid() << ") (" << rhs._dlm << ") (" << rhs._action->needed() << ")";
    return (os);
}




/******************************************************************** 
 *
 * @struct: LogicalLock
 *
 ********************************************************************/ 



/******************************************************************** 
 *
 * @fn:     release()
 *
 * @brief:  Releases the lock on behalf of the particular action,
 *          and returns a vector of promoted actions.
 *          The lock manager should that receives this list should:
 *          (1) associate the promoted action with the particular key in the trx-to-key map
 *          (2) decide which of those are ready to run and return them to the worker
 *
 * @note:   The action must be in the list of owners
 *
 ********************************************************************/ 

vector<base_action_t*> LogicalLock::release(base_action_t* anowner)
{
    BaseActionPtrList promotedList;
    bool found = false;
    bool updated_ready = false;
    
    assert (anowner);
    
    tid_t atid = anowner->tid();
    int i=0;
    for (ActionLLEntryVecIt it=_owners.begin(); it!=_owners.end(); ++it) {
        ++i;
        TRACE( TRACE_TRX_FLOW, "Checking (%d) - Owner-%d (%d)\n", atid, i, (*it).tid());

        if (atid==(*it).tid()) {
            found = true;

            // remove from list
            _owners.erase(_owners.begin()+i);

            // upgrade the dlm
            // if indeed dlm has changed upgrade some of the waiters
            if (_upd_dlm()) {
                TRACE( TRACE_TRX_FLOW, "Release of (%d) has updated dlm\n", atid);
                BaseActionPtr action = NULL;

                // as long as they are waiters that can be upgraded to owners
                while (_head_can_acquire()) {
                    ActionLLEntry head = _waiters.front();
                    action = head.action();
                    
                    // add head of waiters to the promoted list (which will be returned)
                    TRACE( TRACE_TRX_FLOW, "(%d) promoting (%d)\n", atid, action->tid());
                    promotedList.push_back(action);

                    // add head of waiters to the owners list
                    _owners.push_back(head);

                    // remove head from the waiters list
                    _waiters.pop_front();

                    // update the lock-mode of the LL
                    _upd_dlm();
                }
            }
            break;
        }
    }    

    // assert if this trx is not in the list of owners
    if (!found) assert (0);

#ifdef LOCKDEBUG
    // for sanity check, 
    // TODO: (IP) should be removed
    for (ActionLLEntryVecIt it=_owners.begin(); it!=_owners.end(); ++it) {
        if (atid==(*it).tid())
            assert (0); // asserts if owner not removed
    }
    for (ActionLLEntryDequeIt it=_waiters.begin(); it!=_waiters.end(); ++it) {
        if (atid==(*it).tid())
            assert (0); // asserts if owner not removed
    }
#endif

    // return the new list of ready to run actions
    return (promotedList);
}



/******************************************************************** 
 *
 * @fn:     acquire()
 *
 * @brief:  Acquire the lock on behalf of the particular trx.
 *
 * @return: Returns (true) if the lock acquire succeeded.
 *          If it returns (false) the trx is enqueued to the waiters list.
 *
 ********************************************************************/ 

const bool LogicalLock::acquire(ActionLLEntry& areq)
{
    // check if compatible
    if (DoraLockModeMatrix[_dlm][areq.dlm()]) {

#ifdef LOCKDEBUG
        // for sanity check, 
        // TODO: (IP) should be removed
        for (int i=0; i=_owners.size(); ++i) {
            if (!DoraLockModeMatrix[_dlm][_owners[i].dlm()]) 
                assert (0); // asserts if two owners have incompatible modes
        }
#endif

        // if compatible, enqueue to the owners
        _owners.push_back(areq);
        areq.action()->gotkeys(1);

        // update lock mode
        if (areq.dlm() != DL_CC_NOLOCK) _dlm = areq.dlm();

        return (true);
    }        

    // not compatible, enqueue to the waiting list
    assert (_owners.size());
    _waiters.push_back(areq);    

    // return the new lm
    return (false);
}



/******************************************************************** 
 *
 * @fn:     _head_can_acquire()
 *
 * @brief:  Returns (true) if the action at the head of the waiting list
 *          can acquire the lock
 *
 ********************************************************************/ 

const bool LogicalLock::_head_can_acquire()
{
    if (_waiters.empty()) return (false); // no waiters
    return (DoraLockModeMatrix[_dlm][_waiters.front().dlm()]);
}



/******************************************************************** 
 *
 * @fn:     _upd_dlm()
 *
 * @brief:  Iterates the current owners and updates the dlm
 *
 * @return: Returns (true) if the dlm changed
 *
 ********************************************************************/ 

const bool LogicalLock::_upd_dlm()
{
    eDoraLockMode new_dlm = DL_CC_NOLOCK;
    eDoraLockMode odlm = DL_CC_NOLOCK;
    bool changed = false;    

    for (ActionLLEntryVecIt it=_owners.begin(); it!=_owners.end(); ++it) {
        odlm = (*it).dlm();
        if (!DoraLockModeMatrix[new_dlm][odlm]) 
            assert (0); // asserts if two owners have incompatible modes
        if (odlm!=DL_CC_NOLOCK) 
            new_dlm = odlm;
    }

    // set the new dlm
    changed = (_dlm != new_dlm);
    _dlm = new_dlm;

    // return if the dlm has changed
    return (changed);
}



//// Helper functions


std::ostream& operator<<(std::ostream& os, const LogicalLock& rhs) 
{
    os << "lock:   " << rhs._dlm << endl;
    os << "owners:\n";
    for (int i=0; i=rhs._owners.size(); ++i) {
        os << i << ". " << rhs._owners[i] << endl;
    }

    os << "waiters:\n";
    for (int i=0; i=rhs._waiters.size(); ++i) {
        os << i << ". " << rhs._waiters[i] << endl;
    }
    return (os);
}


EXIT_NAMESPACE(dora);

