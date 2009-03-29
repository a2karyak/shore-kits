/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file action.h
 *
 *  @brief Encapsulation of each action in DORA
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __DORA_ACTION_H
#define __DORA_ACTION_H

#include <cstdio>

#include "util.h"
#include "dora.h"

#include "sm/shore/shore_env.h"
#include "sm/shore/shore_worker.h"


#include <atomic.h>

template<int SIZE>
struct is_valid_machine_word_size;

template<>
struct is_valid_machine_word_size<sizeof(int)> {
  typedef unsigned int TYPE;
  static TYPE atomic_cas(TYPE volatile* ptr, TYPE expected, TYPE value) {
    return atomic_cas_32(ptr, expected, value);
  }
  static TYPE atomic_swap(TYPE volatile* ptr, TYPE value) {
    return atomic_swap_32(ptr, value);
  }
};

template<>
struct is_valid_machine_word_size<sizeof(long)> {
  typedef unsigned long TYPE;
  static TYPE atomic_cas(TYPE volatile* ptr, TYPE expected, TYPE value) {
    return atomic_cas_64(ptr, expected, value);
  }
  static TYPE atomic_swap(TYPE volatile* ptr, TYPE value) {
    return atomic_swap_64(ptr, value);
  }
};

template<typename T>
T atomic_swap(T volatile* ptr, T value) {
  typedef is_valid_machine_word_size<sizeof(T)> Helper;
  typedef typename Helper::TYPE HTYPE;
  union { T volatile* p; HTYPE volatile* h; } in = {ptr};
  union { T v; HTYPE h; } v = {value};
  union { HTYPE h; T v; } out = {Helper::atomic_swap(in.h, v.h)};
  return out.v;
}

template<typename T>
T atomic_cas(T volatile* ptr, T expected, T value) {
  typedef is_valid_machine_word_size<sizeof(T)> Helper;
  typedef typename Helper::TYPE HTYPE;
  union { T volatile* p; HTYPE volatile* h; } in = {ptr};
  union { T v; HTYPE h; } e = {expected}, v = {value};
  union { HTYPE h; T v; } out = {Helper::atomic_cas(in.h, e.h, v.h)};
  return out.v;
}



ENTER_NAMESPACE(dora);


using namespace shore;


class base_action_t;
//typedef PooledVec<base_action_t*>::Type       BaseActionPtrList;
typedef vector<base_action_t*>       BaseActionPtrList;
typedef BaseActionPtrList::iterator  BaseActionPtrIt;


/******************************************************************** 
 *
 * @class: base_action_t
 *
 * @brief: (non-template-based) abstract base class for the actions
 *
 ********************************************************************/

class base_action_t
{
protected:

    // rendez-vous point
    rvp_t*         _prvp;

    // trx-specific
    xct_t*         _xct; // Not the owner
    tid_t          _tid;

    int            _keys_needed;

    // flag indicating whether this action is read-only or not
    bool           _read_only;

    // flag showing that this action has set its keys
    bool           _keys_set;


#ifdef WORKER_VERBOSE_STATS
    stopwatch_t    _since_enqueue;
#endif

    // base action init
    inline void _base_set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
                          const int numkeys, const bool ro, const bool keysset) 
    {
        _tid = atid;
        _xct = axct;
        _prvp = prvp;
        _keys_needed = numkeys;
        _read_only = ro;
        _keys_set = 0;
    }

public:

    base_action_t() :
        _prvp(NULL), _xct(NULL), _keys_needed(0), _read_only(false), _keys_set(0)
    { }

    virtual ~base_action_t() { }


    // access methods
    inline rvp_t* rvp() { return (_prvp); }
    inline xct_t* xct() { return (_xct); }    
    inline tid_t  tid() { return (_tid); }
    inline const tid_t tid() const { return (_tid); }

    // read only
    inline const bool is_read_only() { return (_read_only==true); }
    inline void set_read_only() { 
        assert (!_keys_set); // this can happen only if keys are not set yet
        _read_only = true;
    }

    // needed keys operations
    inline const int needed() { return (_keys_needed); }
    inline const bool are_keys_set() { return (_keys_set); }

    inline const bool is_ready() { 
        // if it does not need any other keys, 
        // it is ready to proceed
        return (_keys_needed==0);
    }

    inline const int setkeys(const int numLocks = 1) {
        assert (numLocks);
        _keys_needed = numLocks;
        return (_keys_needed);
    } 

    inline const bool gotkeys(const int numLocks = 1) {
        // called when acquired a number of locks
        // decreases the needed keys counter 
        // and returns if it is ready to proceed        

        // should need at least numLocks locks
        assert ((_keys_needed-numLocks)>=0); 
        _keys_needed -= numLocks;
        return (_keys_needed==0);
    }


    // copying allowed
    base_action_t(base_action_t const& rhs)
        : _prvp(rhs._prvp), _xct(rhs._xct), 
          _tid(rhs._tid), _keys_needed(rhs._keys_needed),
          _read_only(rhs._read_only),
          _keys_set(rhs._keys_set)
    { }

    base_action_t& operator=(base_action_t const& rhs);


    // INTERFACE

    // executes action body
    virtual w_rc_t trx_exec()=0;          

    // acquires the required locks in order to proceed
    virtual const bool trx_acq_locks()=0;

    // releases acquired locks
    virtual const int trx_rel_locks(BaseActionPtrList& readyList, 
                                    BaseActionPtrList& promotedList)=0;

    // hook to update the keys for this action
    virtual const int trx_upd_keys()=0; 

    // enqueues self on the committed list of committed actions
    virtual void notify()=0; 

    // should give memory back to the atomic trash stack
    virtual void giveback()=0;

#ifdef WORKER_VERBOSE_STATS
    inline void mark_enqueue() { _since_enqueue.reset(); }
    inline const double waited() { return (_since_enqueue.time()); }
#endif

}; // EOF: base_action_t



/******************************************************************** 
 *
 * @class: action_t
 *
 * @brief: (template-based) abstract class for the actions
 *
 * @note:  Actions are similar with packets in staged dbs
 *
 ********************************************************************/

template <class DataType>
class action_t : public base_action_t
{
public:
    
    typedef key_wrapper_t<DataType>  Key;
    //typedef typename PooledVec<Key*>::Type    KeyPtrVec;
    typedef vector<Key*>             KeyPtrVec;

    typedef partition_t<DataType>    Partition;

    typedef KALReq_t<DataType>       KALReq;
    //typedef typename PooledVec<KALReq>::Type  KALReqVec;
    typedef vector<KALReq>           KALReqVec;

protected:

    // a vector of pointers to keys
    KeyPtrVec  _keys;

    // a vector of requests for keys
    KALReqVec  _requests;

    // pointer to the partition
    Partition*  _partition;

    inline void _act_set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
                         const int numkeys, 
                         const bool ro=false, const bool keysset=false)
    {
        _base_set(atid,axct,prvp,numkeys,ro,keysset);

        assert (numkeys);
        _keys.reserve(numkeys);
        _requests.reserve(1);
    }


public:

    action_t() : 
        base_action_t(), _partition(NULL)
    { }

    virtual ~action_t() { }


    // copying allowed
    action_t(action_t const& rhs) 
        : base_action_t(rhs), _keys(rhs._keys), _requests(rhs._requests), _partition(rhs._partition)
    { assert (0); }

    action_t& operator=(action_t const& rhs) {
        if (this != &rhs) {
            assert (0);
            base_action_t::operator=(rhs);
            _keys = rhs._keys;
            _requests = rhs._requests;
            _partition = rhs._partition;
        }
        return (*this);
    }

    
    // access methods

    KeyPtrVec* keys() { return (&_keys); }    
    KALReqVec* requests() { return (&_requests); }

    inline Partition* get_partition() const { return (_partition); }
    inline void set_partition(Partition* ap) {
        assert (ap); _partition = ap;
    }
   
    

    // INTERFACE

    virtual w_rc_t trx_exec()=0;

    const bool trx_acq_locks() 
    {
        assert (_partition);
        trx_upd_keys();
        return (_partition->acquire(_requests));
    }

    const int trx_rel_locks(BaseActionPtrList& readyList, 
                            BaseActionPtrList& promotedList)
    {
        assert (_partition);
        assert (_keys_set); // at this point the keys should already be set (at trx_acq_locks)
        //trx_upd_keys();
        return (_partition->release(this,readyList,promotedList));
    }


    virtual const int trx_upd_keys()=0;

    void notify() 
    {
        assert (_partition);
        _partition->enqueue_commit(this);
    }

    virtual void giveback()=0;                            


    virtual void setup(Pool** stl_pool_alloc_list) 
    {
        assert (stl_pool_alloc_list);

        // it must have 2 pool lists: 
        // stl_pool_list[0]: KeyPtr pool
        // stl_pool_list[1]: KALReq pool
//         assert (stl_pool_alloc_list[0] && stl_pool_alloc_list[1]); 

//         _keys = new KeyPtrVec( stl_pool_alloc_list[0] );
//         _requests = new KALReqVec( stl_pool_alloc_list[1] );        
    }

    virtual void reset() 
    {
        // clear contents
        _keys.erase(_keys.begin(),_keys.end());
        _requests.erase(_requests.begin(),_requests.end());
    }

    
}; // EOF: action_t



EXIT_NAMESPACE(dora);

#endif /** __DORA_ACTION_H */

