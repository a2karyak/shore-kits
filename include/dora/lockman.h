/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   lockmap.h
 *
 *  @brief:  Lock manager for DORA partitions
 *
 *  @note:   Enumuration and Compatibility Matrix of locks, 
 *
 *  @author: Ippokratis Pandis, Oct 2008
 *
 */

#ifndef __DORA_LOCK_MAN_H
#define __DORA_LOCK_MAN_H

#include <cstdio>
#include <map>
#include <vector>
#include <deque>

#include "dora/dora_error.h"
#include "dora/key.h"

#include "sm/shore/shore_env.h"

#include "stages/tpcc/common/tpcc_random.h"


ENTER_NAMESPACE(dora);


using std::map;
using std::multimap;
using std::vector;
using std::deque;


struct actionLLEntry;
std::ostream& operator<<(std::ostream& os, const actionLLEntry& rhs);

struct LogicalLock;
std::ostream& operator<<(std::ostream& os, const LogicalLock& rhs);

class base_action_t;



/******************************************************************** 
 *
 * @enum:  eDoraLockMode
 *
 * @brief: Possible lock types in DORA
 *
 *         - DL_CC_NOLOCK : unlocked
 *         - DL_CC_SHARED : shared lock
 *         - DL_CC_EXCL   : exclusive lock
 *
 ********************************************************************/

enum eDoraLockMode {
    DL_CC_NOLOCK    = 0,
    DL_CC_SHARED    = 1,
    DL_CC_EXCL      = 2,

    DL_CC_MODES     = 3 // @careful: Not an actual lock mode
};

static eDoraLockMode DoraLockModeArray[DL_CC_MODES] =
    { DL_CC_NOLOCK, DL_CC_SHARED, DL_CC_EXCL };



/******************************************************************** 
 *
 * Lock compatibility Matrix
 *
 *
 ********************************************************************/

static int DoraLockModeMatrix[DL_CC_MODES][DL_CC_MODES] = { {1, 1, 1},
                                                            {1, 1, 0},
                                                            {1, 0, 0} };



/******************************************************************** 
 *
 * @struct: ActionLLEntry
 *
 * @brief:  Struct for representing an Action entry to the list of holder
 *          of a logical lock
 *
 ********************************************************************/

struct ActionLLEntry
{
    ActionLLEntry()  
        : _action(NULL), _dlm(DL_CC_NOLOCK)
    { }

    ActionLLEntry(base_action_t* action, const eDoraLockMode adlm = DL_CC_NOLOCK)
        : _action(action), _dlm(adlm)
    { }

    ~ActionLLEntry() 
    { 
        _action = NULL;
        _dlm = DL_CC_NOLOCK;
    }

    // access methods
    const eDoraLockMode dlm() { return (_dlm); }
    tid_t tid();
    inline base_action_t* action() { return (_action); }

    // friend function
    friend std::ostream& operator<<(std::ostream& os, const ActionLLEntry& rhs);

private:

    // data
    base_action_t* _action;
    eDoraLockMode  _dlm;

}; // EOF: ActionLLEntry
    

typedef vector<ActionLLEntry>             ActionLLEntryVec;
typedef ActionLLEntryVec::iterator        ActionLLEntryVecIt;
typedef ActionLLEntryVec::const_iterator  ActionLLEntryVecCit;

typedef deque<ActionLLEntry>                ActionLLEntryDeque;
typedef ActionLLEntryDeque::iterator        ActionLLEntryDequeIt;
typedef ActionLLEntryDeque::const_iterator  ActionLLEntryDequeCit;

typedef base_action_t*              BaseActionPtr;
typedef vector<BaseActionPtr>       BaseActionPtrList;
typedef BaseActionPtrList::iterator BaseActionPtrIt;


/******************************************************************** 
 *
 * @class: key_ale_t
 *
 * @brief: Key and Action Logical lock Entry (request) structure
 *
 ********************************************************************/

template<class DataType>
struct key_ale_t
{
    typedef key_wrapper_t<DataType> Key;

    key_ale_t(Key* akp, ActionLLEntry& ale) {
        assert (akp);
        _key = akp;
        _ale = alr;
    }

    key_ale_t(Key* akp, base_action_t* action, const eDoraLockMode& alm) {
        assert (akp);
        assert (action);
        _key = akp;
        _ale = ActionLLEntry(action,alm);
    }

    ~key_ale_t() { }

    inline Key* key() { return (_key); }
    inline ActionLLEntry& ale() { return (_ale); }
    inline tid_t tid() { return (_ale.tid()); }

private:

    Key*          _key;
    ActionLLEntry _ale;

}; // EOF: key_ale_t




/******************************************************************** 
 *
 * @struct: LogicalLock
 *
 * @brief:  Struct for representing an entry in the lock status map
 *
 * @note:   Each entry has:
 *          - The current lock value.
 *          - A list of owner trxs.
 *          - A list of waiting trxs.
 *
 ********************************************************************/

struct LogicalLock
{    
    LogicalLock() 
        : _dlm(DL_CC_NOLOCK)
    { }

    ~LogicalLock() 
    { 
        assert (is_clean());
    }


    const eDoraLockMode dlm() { return (_dlm); }
    ActionLLEntryVec&   owners() { return (_owners); }
    ActionLLEntryDeque& waiters() { return (_waiters); }

    // acquire operation
    // if it fails, it enqueues the trx_ll_entry to the queue of waiters
    // and returns false
    const bool acquire(ActionLLEntry& areq);

    // release operation, returns a list of promoted actions
    // the lock manager should
    // (1) associate the promoted with the particular key in the trx-to-key map
    // (2) decide which of those are ready to run and return them to the worker
    BaseActionPtrList release(base_action_t* anowner);

    // is clean
    // returns true if no locked
    const bool is_clean() { 
        return ((_dlm == DL_CC_NOLOCK) && (_owners.empty()) && (_waiters.empty())); 
    }

    const bool has_owners()  { return (!_owners.empty()); }
    const bool has_waiters() { return (!_waiters.empty()); }

    void reset() {
        _owners.clear();
        _waiters.clear();
    }



    // data
    eDoraLockMode        _dlm;       // logical lock
    ActionLLEntryVec     _owners;    // vector of owners
    ActionLLEntryDeque   _waiters;   // deque of waiters - we want to push/pop both sides

    
private:

    // can acquire
    const bool _head_can_acquire();

    // update lock mode
    const bool _upd_dlm();

}; // EOF: struct LogicalLock


/******************************************************************** 
 *
 * @struct: key_ll_map_t
 *
 * @brief:  Template-based class for maintaining a map between keys
 *          of the partition and the status of their logical locks.
 *
 *          (Acquire) Returns false if locked in incompatible mode.
 *
 * @note:   It is based on a map of key_wrapper_t to ll_entry
 * @note:   Never removes entries. The map will be increasing as long
 *          as new keys are queried.
 * @note:   We can have a background thread that removes entries from
 *          the map if its size becomes a problem.
 *
 ********************************************************************/

template<class DataType>
struct key_ll_map_t
{
    typedef key_wrapper_t<DataType>        Key;
    typedef map<Key, LogicalLock>          LLMap;
    typedef typename LLMap::iterator       LLMapIt;
    typedef typename LLMap::const_iterator LLMapCit;

    key_ll_map_t() { }
    ~key_ll_map_t() { }


    // acquire, return true on success
    // false means not compatible
    inline const bool acquire(const Key& aKey, ActionLLEntry& ale) {
        LogicalLock* ll = &_ll_map[aKey];
        assert (ll);
        return (ll->acquire(ale));
    }
                
    // release        
    inline BaseActionPtrList release(const Key& aKey, base_action_t* action) {
        LogicalLock* ll = &_ll_map[aKey];
        assert (ll);
        return (ll->release(action));
    }



    // clear map
    void reset() {
        // clear all entries
        for (LLMapIt it=_ll_map.begin(); it!=_ll_map.end(); ++it) {
            it->second.reset();
        }
        // clear map
        _ll_map.clear();
    }

    // return the number of keys
    const int keystouched() { return (_ll_map.size()); }

    // returns (true) if all locks are clean
    const bool is_clean() {
        // clear all entries
        for (LLMapIt it=_ll_map.begin(); it!=_ll_map.end(); ++it) {
            if (!it->second.is_clean()) return (false);
        }
        return (true);
    }


    // for debugging
    void dump() const {
        int sz = _ll_map.size();
        TRACE( TRACE_DEBUG, "Keys (%d)\n", sz);
        for (LLMapCit it=_ll_map.begin(); it!=_ll_map.end(); ++it) {
            cout << "K (" << it->first << ")\nL\n"; 
            cout << it->second << "\n";
        }
    }

private: 

    // data
    LLMap _ll_map; // map of logical locks - each key has its own ll

}; // EOF: struct key_ll_map_t




/******************************************************************** 
 *
 * @class: lock_man_t
 *
 * @brief: Lock manager for the locks of a partition
 *
 * @note:  The lock manager consists of a
 *         - A map for the status of logical locks (llst_map_t)
 *         - A bimap for associating trxs with Keys
 *
 *
 ********************************************************************/

template<class DataType>
struct lock_man_t
{
    typedef action_t<DataType>        Action;

    typedef key_wrapper_t<DataType>   Key;
    typedef key_ll_map_t<DataType>    KeyLLMap;

    typedef vector<Key>               KeyList;

    typedef multimap<tid_t,Key>                 TrxKeyMap;
    typedef typename TrxKeyMap::iterator        TrxKeyMapIt;
    typedef typename TrxKeyMap::const_iterator  TrxKeyMapCit;
    typedef pair<tid_t,Key>                     TrxKeyPair;
    typedef pair<TrxKeyMapIt,TrxKeyMapIt>       TrxRange;

    typedef key_ale_t<DataType>       KeyActionLockRequest;


    lock_man_t() { }
    ~lock_man_t() { }


    // acquire ll of a key on behalf of a trx
    inline const bool acquire(KeyActionLockRequest& akalr) 
    {
        bool rhs = false;

        // if lock acquisition successful,
        // associate key to trx
        rhs = _key_ll_m.acquire(*akalr.key(), akalr.ale());
        if (rhs) {
            _trx_key_mm.insert(TrxKeyPair(akalr.tid(), *akalr.key()));
        }
        return (rhs);
    }

    // releases all the acquired LLs by a trx
    // returns a list of actions that are ready to run
    inline BaseActionPtrList release_all(Action* paction) 
    {
        assert (paction->is_ready());

        BaseActionPtrList readyList;
        BaseActionPtrList promotedList;
        KeyList promotedKeyList;

        tid_t atid = paction->tid();
        TrxRange r = _trx_key_mm.equal_range(atid);
        
        TRACE( TRACE_TRX_FLOW, "Releasing (%d)\n", atid);


        // 1. Release all the keys associated with this trx (from the trx-to-key map

        BaseActionPtrList alist;
        for (TrxKeyMapIt it=r.first; it!=r.second; ++it) {           
            // release one LL
            alist = _key_ll_m.release((*it).second,paction);

            // receive a list of promoted actions - due to this LL release
            promotedList.insert(promotedList.end(),alist.begin(),alist.end()); // append the list

            // append the corresponding key that many times in the promoted key list
            promotedKeyList.insert(promotedKeyList.end(),alist.size(),(*it).second);
        }                 


        // 2. Remove associations of this trx from the trx-to-key map

        _trx_key_mm.erase(atid);


        // 3. Iterate over all the promoted actions

        // 3a. Associate each action (tid) with the key in the trx-to-key map
        // 3b. If they are ready to execute return them to the worker 

        int sz = promotedList.size();
        assert (sz==promotedKeyList.size());
        BaseActionPtr ap = NULL;
        tid_t readytid;

        for (int i=0; i<sz; i++) {          

            // associate trx with key
            ap = promotedList[i];
            readytid = ap->tid();
            _trx_key_mm.insert(TrxKeyPair(readytid, promotedKeyList[i]));

            // if the promoted action is ready add it to the list
            ap->gotkeys(1);
            if (ap->is_ready()) {
                TRACE( TRACE_TRX_FLOW, "(%d) ready (%d)\n", atid, readytid);
                readyList.push_back(ap);
            }                    

        }
        return (readyList);
    }


    void reset() {
        _key_ll_m.reset();
        _trx_key_mm.clear();
    }


    // for debugging
    void dump() const {
        _key_ll_m.dump();                
        TRACE( TRACE_DEBUG, "(%d) trx-key pairs\n", _trx_key_mm.size());
        for (TrxKeyMapCit cit=_trx_key_mm.begin(); cit!=_trx_key_mm.end(); ++cit) {
            cout << "TRX (" << cit->first << ")";
            cout << " - K (" << cit->second << ")\n";
        }
    }

    const int keystouched() { return (_key_ll_m.keystouched()); }
    const int trxslocking() { return (_trx_key_mm.size()); }
    const bool is_clean() {
        return ((_trx_key_mm.size()==0) && (_key_ll_m.is_clean()));
    }

private:

    // data
    KeyLLMap  _key_ll_m;   // map of keys to logical locks 
    TrxKeyMap _trx_key_mm; // map of trxs to keys    

}; // EOF: struct lock_man_t


EXIT_NAMESPACE(dora);

#endif /* __DORA_LOCK_MAN_H */
