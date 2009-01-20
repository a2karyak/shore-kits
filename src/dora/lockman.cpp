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
 * @struct: ActionLockReq
 *
 ********************************************************************/ 

//// Helper functions


std::ostream& operator<<(std::ostream& os, const ActionLockReq& rhs) 
{
    os << "(" << rhs._tid << ") (" << rhs._dlm << ") (" << rhs._action << ")";
    return (os);
}




struct pretty_printer {
    ostringstream _out;
    string _tmp;
    operator ostream&() { return _out; }
    operator char const*() { _tmp = _out.str(); _out.str(""); return _tmp.c_str(); }
};


static void _print_logical_lock_maps(std::ostream &out, LogicalLock const &ll) 
{
    int o = ll.owners().size();
    int w = ll.waiters().size();
    out << "Owners " << o << endl;
    int i=0;
    for (i=0; i<o; ++i) {
        out << i << ". " << ll.owners()[i] << endl;
    }
    out << "Waiters " << o << endl;
    i=0;
    for (ActionLockReqListCit it=ll.waiters().begin(); it!=ll.waiters().end(); ++it) {

        out << ++i << ". " << (*it) << endl;
    }
}

char const* db_pretty_print(LogicalLock const* ll, int i=0, char const* s=0) 
{
    static pretty_printer pp;
    _print_logical_lock_maps(pp, *ll);
    return pp;
}


static void _print_key(std::ostream &out, key_wrapper_t<int> const &key) 
{    
    for (int i=0; i<key._key_v.size(); ++i) {
        out << key._key_v.at(i) << endl;
    }
}

char const* db_pretty_print(key_wrapper_t<int> const* key, int i=0, char const* s=0) 
{
    static pretty_printer pp;
    _print_key(pp, *key);
    return pp;
}



/******************************************************************** 
 *
 * @struct: LogicalLock
 *
 ********************************************************************/ 

LogicalLock::LogicalLock(ActionLockReq& anowner)
    : _dlm(anowner.dlm())
{
    // construct a logical lock with an owner already
    _owners.reserve(1);
    _owners.push_back(anowner);
    anowner.action()->gotkeys(1);
}




/******************************************************************** 
 *
 * @fn:     release()
 *
 * @brief:  Releases the lock on behalf of the particular action,
 *          and updates the vector of promoted actions.
 *          The lock manager should that receives this list should:
 *          (1) associate the promoted action with the particular key in the trx-to-key map
 *          (2) decide which of those are ready to run and return them to the worker
 *
 * @note:   The action must be in the list of owners
 *
 ********************************************************************/ 

const int LogicalLock::release(BaseActionPtr anowner, 
                               BaseActionPtrList& promotedList)
{
    w_assert3 (anowner);
    bool found = false;    
    tid_t atid = anowner->tid();
    int ipromoted = 0;

    for (ActionLockReqVecIt it=_owners.begin(); it!=_owners.end(); ++it) {
        tid_t* ownertid = (*it).tid();
        w_assert3 (ownertid);
        TRACE( TRACE_TRX_FLOW, "Checking (%d) - Owner(%d)\n", atid, *ownertid);

        if (atid==*ownertid) {
            found = true;

            // remove from list
            _owners.erase(it);

            // upgrade the dlm
            // if indeed dlm has changed upgrade some of the waiters
            if (_upd_dlm()) {
                TRACE( TRACE_TRX_FLOW, "Release of (%d) has updated dlm\n", atid);
                BaseActionPtr action = NULL;

                // as long as they are waiters that can be upgraded to owners
                while (_head_can_acquire()) {
                    ActionLockReq head = _waiters.front();
                    action = head.action();
                    
                    // add head of waiters to the promoted list (which will be returned)
                    TRACE( TRACE_TRX_FLOW, "(%d) promoting (%d)\n", atid, action->tid());
                    promotedList.push_back(action);

                    // add head of waiters to the owners list
                    _owners.push_back(head);
                    ++ipromoted;

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
    return (ipromoted);
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

const bool LogicalLock::acquire(ActionLockReq& alr)
{
    w_assert3 (alr.action());
    // check if compatible

    if (DoraLockModeMatrix[_dlm][alr.dlm()]) {

        // if compatible, enqueue to the owners
        _owners.push_back(alr);
        alr.action()->gotkeys(1);

        // update lock mode
        if (alr.dlm() != DL_CC_NOLOCK) _dlm = alr.dlm();

        // indicate acquire success
        return (true);
    }        

    // not compatible, enqueue to the waiting list
    w_assert3 (_owners.size());
    _waiters.push_back(alr);    

    // indicate failure to acquire
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

    for (ActionLockReqVecIt it=_owners.begin(); it!=_owners.end(); ++it) {
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
    os << "lock:   " << rhs.dlm() << endl; 
    os << "owners: " << rhs.owners().size() << endl; 
    for (int i=0; i<rhs.owners().size(); ++i) {
        os << rhs.owners()[i] << endl;
    }

    os << "waiters: " << rhs.waiters().size() << endl;
    for (ActionLockReqListCit it=rhs.waiters().begin(); it!=rhs.waiters().end(); ++it) {
        os << (*it) << endl;
    }

    return (os);
}


EXIT_NAMESPACE(dora);

