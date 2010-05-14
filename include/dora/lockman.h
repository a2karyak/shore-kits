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

#include "util/stl_pooled_alloc.h"



ENTER_NAMESPACE(dora);


using std::map;
using std::multimap;
using std::vector;
using std::deque;


class ActionLockReq;
//typedef PooledVec<ActionLockReq>::Type      ActionLockReqVec;
typedef vector<ActionLockReq>               ActionLockReqVec;
typedef ActionLockReqVec::iterator          ActionLockReqVecIt;
typedef PooledList<ActionLockReq>::Type     ActionLockReqList;
typedef ActionLockReqList::iterator         ActionLockReqListIt;
typedef ActionLockReqList::const_iterator   ActionLockReqListCit;


class base_action_t;
typedef base_action_t*                      BaseActionPtr;
//typedef PooledVec<base_action_t*>::Type     BaseActionPtrList;
typedef vector<base_action_t*>     BaseActionPtrList;



struct actionLLEntry;
std::ostream& operator<<(std::ostream& os, const actionLLEntry& rhs);

struct LogicalLock;
std::ostream& operator<<(std::ostream& os, const LogicalLock& rhs);


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

    ActionLockReq(base_action_t* action, const tid_t& atid,
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
    inline eDoraLockMode dlm() { return (_dlm); }
    inline base_action_t* action() { return (_action); }
    inline tid_t* tid() { return (&_tid); }

    inline bool isSame(const ActionLockReq& alr) { return (_tid==alr._tid); }

    // friend function
    friend std::ostream& operator<<(std::ostream& os, const ActionLockReq& rhs);


protected:

    // data
    base_action_t* _action;
    tid_t          _tid;
    eDoraLockMode  _dlm;


}; // EOF: ActionLockReq




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

    KALReq_t(const ActionLockReq& alr, Key* akey)
        : ActionLockReq(alr), _key(akey)
    { }

    KALReq_t(base_action_t* action, const eDoraLockMode adlm, Key* akey)
        : ActionLockReq(action,action->tid(),adlm), _key(akey)
    { }

    ~KALReq_t() { }

    
    // copying allowed
    KALReq_t(const KALReq_t& rhs)
        : ActionLockReq(rhs), _key(rhs._key)
    { }

    KALReq_t& operator=(const KALReq_t& rhs) {
        if (this!=&rhs) {
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
//     LogicalLock() 
//         : _dlm(DL_CC_NOLOCK)
//     { }

    LogicalLock(ActionLockReq& anowner);
    ~LogicalLock() { }


    eDoraLockMode       dlm() const { return (_dlm); }
    ActionLockReqVec&   owners() const { return (_owners); }
    ActionLockReqList&  waiters() const { return (_waiters); }


    // acquire operation
    // if it fails, it enqueues the trx_ll_entry to the queue of waiters
    // and returns false
    bool acquire(ActionLockReq& alr);


    // release operation, appends to the list of promoted actions and returns the
    // number of promoted. The lock manager should
    // (1) associate the promoted with the particular key in the trx-to-key map
    // (2) decide which of those are ready to run and return them to the worker
    int release(BaseActionPtr anowner, BaseActionPtrList& promotedList);


    // is clean
    // returns true if no locked
    bool is_clean() const;
    bool has_owners() const  { return (!_owners.empty()); }
    bool has_waiters() const { return (!_waiters.empty()); }

    void reset();
    
private:

    // data
    eDoraLockMode       _dlm;       // logical lock
    ActionLockReqVec    _owners;    // vector of owners
    ActionLockReqList   _waiters;   // list of waiters - we want to push/pop both sides

    // can acquire
    bool _head_can_acquire();

    // update lock mode
    bool _upd_dlm();

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

static const int ENTRIES_PER_KEY_LL_MAP = 6000;

template<class DataType>
struct KeyLockMap
{
public:

    typedef key_wrapper_t<DataType>   Key;
    //typedef typename PooledVec<Key>::Type   KeyList;
    typedef vector<Key>   KeyList;

    typedef KALReq_t<DataType>              KALReq;

    typedef typename PooledMap<Key,LogicalLock>::Type  LLMap;
    typedef typename LLMap::iterator        LLMapIt;
    typedef typename LLMap::const_iterator  LLMapCit;

    typedef typename LLMap::value_type      LLMapVT;
    typedef typename _Rb_tree_node<LLMapVT> LLMapNode;

protected:

    // data
    guard<LLMap>   _ll_map;    // map of logical locks - each key has its own ll
    LLMapIt _ll_map_it; 

    // pool for KeyLL nodes
    guard<Pool> _keyll_pool;

    // pool for Keys
    //guard<Pool> _key_pool;

    // cache of Keys
    guard< object_cache_t<Key> > _key_cache;
    array_guard_t<Pool*> _key_pool_array;

public:

    KeyLockMap(const int keyEstimation) 
    { 
        // setup Key-LL map
        assert (keyEstimation);
        _keyll_pool = new Pool(sizeof(LLMapNode),  keyEstimation); 
        assert (_keyll_pool);
        _ll_map = new LLMap(std::less<Key>(),_keyll_pool.get());

        // setup Key cache
        // _key_pool = new Pool(sizeof(Key), 1000);
        // assert (_key_pool);

        _key_pool_array = new Pool*[1];
        assert (_key_pool_array);

        //_key_pool_array[0] = _key_pool.get();

        _key_cache = new object_cache_t<Key>(_key_pool_array.get());
        assert (_key_cache);
    }

    ~KeyLockMap() 
    { 
        // delete Key-LL map entries
        reset();
        
        // remove cache
        _key_cache.done();
        //_key_pool.done();
        _key_pool_array.done();

        _ll_map.done();
        _keyll_pool.done();
    }


    // acquire, return true on success
    // false means not compatible
    inline bool acquire(KALReq& akalr) 
    {
        bool bAcquire = false;

        // IP: applying the efficientAddOrUpdate optimization from "Effective STL" pp. 110.
        _ll_map_it = _ll_map->lower_bound(*akalr._key);
        
        if (_ll_map_it!=_ll_map->end() && 
            !(_ll_map->key_comp()(*akalr._key,_ll_map_it->first))) {
            
            // update
            bAcquire = _ll_map_it->second.acquire(akalr);
        }
        else {
            // insert

            // use a key from the cache
            Key* akey = _key_cache->borrow();
            akey->copy(*akalr._key);

            _ll_map->insert(_ll_map_it,LLMapVT(*akey,akalr));
            _key_cache->giveback(akey);
            bAcquire = true;
        }

        if (bAcquire) akalr.action()->gotkeys(1);
        return (bAcquire);
    }
                
    // release        
    inline int release(const Key& aKey, 
                             BaseActionPtr paction,
                             BaseActionPtrList& promotedList) 
    {        
//         ostringstream out;
//         string sout;
//         out << aKey;
//         sout = out.str();
//         TRACE( TRACE_ALWAYS, "Releasing (%s)\n", sout.c_str());

        _ll_map_it = _ll_map->find(aKey);
        LogicalLock* ll = &_ll_map_it->second;
        assert (ll);

        int rhs = ll->release(paction,promotedList);
        //        if (ll->is_clean()) _ll_map->erase(_ll_map_it);
        return (rhs);
    }


    //// Debugging ////

    // clear map
    void clear() { _ll_map->clear(); }

    // reset map
    void reset() {
        // clear all entries
        for (LLMapIt it=_ll_map->begin(); it!=_ll_map->end(); ++it) {
            it->second.reset();
        }
        // clear map
        _ll_map->clear();
    }

    // return the number of keys
    int keystouched() const { return (_ll_map->size()); }

    // returns (true) if all locks are clean
    bool is_clean() {
        // clear all entries
        bool isClean = true;
        uint dirtyCount = 0;
        for (LLMapIt it=_ll_map->begin(); it!=_ll_map->end(); ++it) {
            if (!it->second.is_clean()) {
                ++dirtyCount;
                //isClean = false;
                it->second.reset();
            }
        }
        if (dirtyCount) {
            TRACE( TRACE_ALWAYS, "(%d) dirty locks\n", dirtyCount);
        }
        return (isClean);
    }

    void dump() const {
        int sz = _ll_map->size();
        TRACE( TRACE_DEBUG, "Keys (%d)\n", sz);
        for (LLMapCit it=_ll_map->begin(); it!=_ll_map->end(); ++it) {
            cout << "K (" << it->first << ")\nL\n"; 
            cout << it->second << "\n";
        }
    }

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
public:

    typedef action_t<DataType>       Action;

    typedef key_wrapper_t<DataType>  Key;

    typedef KeyLockMap<DataType>     KeyLLMap;
    typedef pair<Key,LogicalLock>    KeyLLPair;

    typedef std::_Rb_tree_node<KeyLLPair> KeyLLMapNode;

    typedef KALReq_t<DataType>               KALReq;
    //typedef typename PooledVec<KALReq>::Type KALReqVec;
    typedef vector<KALReq> KALReqVec;
    typedef typename KALReqVec::iterator              KALReqIt;

private:

    // data
    guard<KeyLLMap>     _key_ll_m;   // map of keys to logical locks 

public:    

    lock_man_t(const int keyEstimation) 
    { 
        _key_ll_m = new KeyLLMap(keyEstimation);
    }   
    
    ~lock_man_t() { }


    // @fn:     acquire_all()
    // @brief:  Tries to acquire all the locks from a list of keys on behalf of a trx.
    // @return: (true) on success
    inline bool acquire_all(KALReqVec& akalrvec) 
    {
        bool bResult = true;
        int i=0;
        for (KALReqIt it=akalrvec.begin(); it!=akalrvec.end(); ++it) {
            // request to acquire all the locks for this partition
            ++i;
            if (!_key_ll_m->acquire(*it)) {
                // if a key cannot be acquired, return false
                TRACE( TRACE_TRX_FLOW, "Cannot acquire for (%d)\n", 
                       *((*it).tid()));
                bResult = false;
            }
        }
        assert(i); // makes sure that the action tries to acquire at least one key
        return (bResult);
    }


    // @fn:     release_all(Action*,BaseActionPtrList&,BaseActionPtrList&)
    // @brief:  Releases all the LLs help by a particular trx
    // @return: Returns a list of actions that are ready to run
    inline int release_all(Action* paction, 
                                 BaseActionPtrList& readyList, 
                                 BaseActionPtrList& promotedList) 
    {
        assert (paction);
        assert (readyList.empty() && promotedList.empty());
        TRACE( TRACE_TRX_FLOW, "Releasing (%d)\n", paction->tid());

        // 1. Release and drop all the keys of this trx
        for (KALReqIt it=paction->requests()->begin(); it!=paction->requests()->end(); ++it) {
            Key* pkey = (*it).key();
            _key_ll_m->release(*pkey,paction,promotedList);
            //pkey->drop();
        }

        // 2. Iterate over all the promoted actions
        BaseActionPtr ap = NULL;
        for (int i=0; i<promotedList.size(); ++i) {

            // if they are ready to execute return them as ready to the worker 
            ap = promotedList[i];
            ap->gotkeys(1);
            if (ap->is_ready()) {
                TRACE( TRACE_TRX_FLOW, "(%d) ready (%d)\n", paction->tid(), ap->tid());
                readyList.push_back(ap);
            }
        }                
        return (0);
    }




    //// Debugging ////

    int keystouched() const { return (_key_ll_m->keystouched()); }

    void reset() { _key_ll_m->reset(); }
    void dump() { _key_ll_m->dump(); }

    bool is_clean()    { return (_key_ll_m->is_clean()); }

}; // EOF: struct lock_man_t


EXIT_NAMESPACE(dora);

#endif /* __DORA_LOCK_MAN_H */
