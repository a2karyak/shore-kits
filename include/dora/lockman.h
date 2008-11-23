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
typedef vector<base_action_t*>       BaseActionPtrList;


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
 * @struct: ActionLockReq
 *
 * @brief:  Struct for representing an Action entry to the list of holders
 *          of a logical lock
 *
 ********************************************************************/

struct ActionLockReq
{
    ActionLockReq()  
        : _action(NULL)
    { }

    ActionLockReq(base_action_t* action, tid_t& atid,
                  const eDoraLockMode adlm = DL_CC_NOLOCK)
        : _action(action), _tid(atid), _dlm(adlm)
    { }

    ~ActionLockReq() 
    { 
        _action = NULL;
    }

    // copying allowed
    ActionLockReq(const ActionLockReq& rhs)
        : _action(rhs._action), _tid(rhs._tid), _dlm(rhs._dlm)
    { }

    ActionLockReq& operator=(const ActionLockReq& rhs)
    { 
        _action = rhs._action;
        _tid = rhs._tid;
        _dlm = rhs._dlm;
        return (*this);
    }

    // access methods
    inline const eDoraLockMode dlm() { return (_dlm); }
    inline base_action_t* action() { return (_action); }
    inline tid_t* tid() { return (&_tid); }

    // friend function
    friend std::ostream& operator<<(std::ostream& os, const ActionLockReq& rhs);


protected:

    // data
    base_action_t* _action;
    tid_t          _tid;
    eDoraLockMode  _dlm;


}; // EOF: ActionLockReq


typedef vector<ActionLockReq>             ActionLockReqVec;
typedef ActionLockReqVec::iterator        ActionLockReqVecIt;
typedef deque<ActionLockReq>              ActionLockReqDeque;
typedef ActionLockReqDeque::iterator      ActionLockReqDequeIt;



/******************************************************************** 
 *
 * @struct: KALReq_t
 *
 * @brief:  Template-based class that extends the Action Lock request 
 *          with a pointer to a key
 *
 ********************************************************************/

template<class DataType>
struct KALReq_t : public ActionLockReq
{
    typedef key_wrapper_t<DataType> Key;
    typedef action_t<DataType>      Action;

    KALReq_t()
        : ActionLockReq(), _pkey(NULL)
    { }

    KALReq_t(const ActionLockReq& alr, Key* akey)
        : ActionLockReq(alr), _key(akey)
    { }

    KALReq_t(base_action_t* action, const eDoraLockMode adlm, Key* akey)
        : ActionLockReq(action,action->tid(),adlm), _key(akey)
    { }

    ~KALReq_t()
    { 
        _key = NULL;
    }

    // copying allowed
    KALReq_t(const KALReq_t& rhs)
        : ActionLockReq(rhs), _key(rhs._key)
    { }

    KALReq_t& operator=(const KALReq_t& rhs) {
        if (this!=rhs) {
            ActionLockReq::operator=(rhs);
            _key = rhs._key;
        }
        return (*this);
    }


    // access methods
    Key* key() { return (_key); }

    // data
    Key* _key;


}; // EOF: KALReq_t



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
    typedef base_action_t*         BaseActionPtr;
    typedef vector<base_action_t*> BaseActionPtrList;

    LogicalLock() 
        : _dlm(DL_CC_NOLOCK),_waiters(30)
    { 
        _owners.reserve(2);
        _owners.clear();
        _waiters.clear();
    }

    ~LogicalLock() 
    { 
        assert (is_clean());
    }


    const eDoraLockMode dlm() const { return (_dlm); }
    const ActionLockReqVec&   owners() const { return (_owners); }
    const ActionLockReqDeque& waiters() const { return (_waiters); }

    // acquire operation
    // if it fails, it enqueues the trx_ll_entry to the queue of waiters
    // and returns false
    const bool acquire(ActionLockReq& alr);

    // release operation, appends to the list of promoted actions and returns the
    // number of promoted. The lock manager should
    // (1) associate the promoted with the particular key in the trx-to-key map
    // (2) decide which of those are ready to run and return them to the worker
    const int release(BaseActionPtr anowner, BaseActionPtrList& promotedList);

    // is clean
    // returns true if no locked
    inline const bool is_clean() { 
        return ((_owners.empty()) && (_waiters.empty())); 
    }

    const bool has_owners()  { return (!_owners.empty()); }
    const bool has_waiters() { return (!_waiters.empty()); }

    void reset() {
        _owners.clear();
        _waiters.clear();
    }
    
private:

    // data
    eDoraLockMode        _dlm;       // logical lock
    ActionLockReqVec     _owners;    // vector of owners
    ActionLockReqDeque   _waiters;   // deque of waiters - we want to push/pop both sides


    // can acquire
    const bool _head_can_acquire();

    // update lock mode
    const bool _upd_dlm();

}; // EOF: struct LogicalLock


/******************************************************************** 
 *
 * @struct: KeyLockMap
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
struct KeyLockMap
{
    typedef key_wrapper_t<DataType>        Key;
    typedef vector<Key>                    KeyList;

    typedef KALReq_t<DataType>             KALReq;
    typedef KALReq_t<DataType>*            KALReqPtr;

    typedef map<Key, LogicalLock>          LLMap;
    typedef typename LLMap::iterator       LLMapIt;
    typedef typename LLMap::const_iterator LLMapCit;

    KeyLockMap() { }
    ~KeyLockMap() { }


    // acquire, return true on success
    // false means not compatible
    inline const bool acquire(KALReq& akalr) {
        LogicalLock* ll = &_ll_map[*akalr._key];
        assert (ll);

        ostringstream out;
        string sout;
        out << *akalr._key << endl;
        out << "sz: " << _ll_map.size() << " " << ll << endl;
        sout = out.str();
        TRACE( TRACE_ALWAYS, "ACQ - %s", sout.c_str());
        out.str("");
        out.clear();

        return (ll->acquire(akalr));
    }
                
    // release        
    inline const int release(const Key& aKey, 
                             BaseActionPtr paction,
                             BaseActionPtrList& promotedList) 
    {
        _ll_map_it = _ll_map.find(aKey);
        LogicalLock* ll = &_ll_map_it->second;
        //LogicalLock* ll = &_ll_map[aKey];
        assert (ll);


        ostringstream out;
        string sout;
        out << aKey << endl;
        out << "sz: " << _ll_map.size() << " " << ll << endl;
        sout = out.str();
        TRACE( TRACE_ALWAYS, "REL - %s", sout.c_str());
        out.str("");
        out.clear();

        int rhs = ll->release(paction,promotedList);
        if (ll->is_clean()) _ll_map.erase(_ll_map_it);
        return (rhs);
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

protected:

    // data
    LLMap   _ll_map;    // map of logical locks - each key has its own ll
    LLMapIt _ll_map_it; 

}; // EOF: struct KeyLockMap





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
    typedef KeyLockMap<DataType>      KeyLLMap;
    typedef vector<Key>               KeyList;
    typedef vector<Key*>              KeyPtrList;

    typedef multimap<tid_t,Key>                 TrxKeyMap;
    typedef typename TrxKeyMap::iterator        TrxKeyMapIt;
    typedef typename TrxKeyMap::const_iterator  TrxKeyMapCit;
    typedef pair<tid_t,Key>                     TrxKeyPair;
    typedef pair<TrxKeyMapIt,TrxKeyMapIt>       TrxRange;

    typedef KALReq_t<DataType>    KALReq;
    typedef KALReq_t<DataType>*   KALReqPtr;
    typedef vector<KALReqPtr>     KALReqPtrVec;


    lock_man_t() { }
    ~lock_man_t() { }


    // acquire ll of a key on behalf of a trx
    inline const bool acquire(KALReq& akalr) 
    {
        bool rhs = false;

        // if lock acquisition successful,
        // associate key to trx
        rhs = _key_ll_m.acquire(akalr);
        if (rhs) {
            //cout << "I = " << akalr.action()->tid() << "-" << *akalr._key << endl;
            _trx_key_mm.insert(TrxKeyPair(akalr.action()->tid(), *akalr._key));
        }
        return (rhs);
    }

    // releases all the LLs help by a particular trx
    // returns a list of actions that are ready to run
    inline const int release_all(Action* paction, 
                                 BaseActionPtrList& readyList, 
                                 BaseActionPtrList& promotedList) 
    {
        assert (paction);
        assert (readyList.empty() && promotedList.empty());

        const tid_t& tidToRelease = paction->tid();
        TrxRange r = _trx_key_mm.equal_range(tidToRelease);
        KeyPtrList keyList;
        
        TRACE( TRACE_TRX_FLOW, "Releasing (%d)\n", tidToRelease);


        // 1. Release all the keys associated with this trx (from the trx-to-key map
        for (TrxKeyMapIt it=r.first; it!=r.second; ++it) {           

            // release one LL            
            Key theKey = (*it).second;
            int numPromoted = _key_ll_m.release(theKey,paction,promotedList);
            keyList.insert(keyList.end(),numPromoted,&theKey); // copy x times the key
        }                 


        // 2. Remove associations of this trx from the trx-to-key map
        _trx_key_mm.erase(tidToRelease);


        // 3. Iterate over all the promoted actions          
        int sz = promotedList.size();
        assert (sz==keyList.size());
        for (int i=0; i<sz; ++i) {

            // 2a. Associate each action (tid) with the key in the trx-to-key map
            BaseActionPtr ap = promotedList[i];
            _trx_key_mm.insert(TrxKeyPair(ap->tid(), *keyList[i]));

            // 2b. If they are ready to execute return them as ready to the worker 
            ap->gotkeys(1);
            if (ap->is_ready()) {
                TRACE( TRACE_TRX_FLOW, "(%d) ready (%d)\n", tidToRelease, ap->tid());
                readyList.push_back(ap);
            }
        }                
        return (0);
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
