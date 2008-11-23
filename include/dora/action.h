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
typedef base_action_t*              BaseActionPtr;
typedef vector<BaseActionPtr>       BaseActionPtrList;
typedef BaseActionPtrList::iterator BaseActionPtrIt;


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

    // base action init
    inline void _base_set(const tid_t& atid, xct_t* axct, rvp_t* prvp, const int numkeys) 
    {
        _tid = atid;
        _xct = axct;
        _prvp = prvp;
        _keys_needed = numkeys;
    }

public:

    base_action_t() :
        _prvp(NULL), _xct(NULL), _keys_needed(0)
    { }

    virtual ~base_action_t() { 
        _xct = NULL;
        _prvp = NULL;
        _keys_needed = 0;
    }

    // access methods
    inline rvp_t* rvp() { return (_prvp); }
    inline xct_t* xct() { return (_xct); }    
    inline tid_t  tid() { return (_tid); }
    inline const tid_t tid() const { return (_tid); }

    // needed keys operations

    inline const int needed() { return (_keys_needed); }

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
          _tid(rhs._tid), _keys_needed(rhs._keys_needed)
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

    // enqueues self on the committed list of committed actions
    virtual void notify()=0; 

    // should give memory back to the atomic trash stack
    virtual void giveback()=0;

}; // EOF: base_action_t



/******************************************************************** 
 *
 * @class: action_t
 *
 * @brief: (template-based) Aabstract class for the actions
 *
 * @note:  Actions are similar with packets in staged dbs
 *
 ********************************************************************/

template <class DataType>
class action_t : public base_action_t
{
public:
    
    typedef key_wrapper_t<DataType>  Key;
    typedef vector<Key>              KeyVec;
    typedef vector<Key*>             KeyPtrVec;
    typedef partition_t<DataType>    Partition;

    typedef KALReq_t<DataType>       KALReq;
    typedef KALReq*                  KALReqPtr;
    typedef vector<KALReqPtr>        KALReqPtrVec;

protected:

    // a vector of pointers to keys
    vector<Key*>   _keys;

    //pointer to the partition
    Partition*     _partition;

    inline void _act_set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
                         const int numkeys)
    {
        _base_set(atid,axct,prvp,numkeys);
        assert (numkeys);
        _keys.reserve(numkeys);
    }

public:

    action_t() : 
        base_action_t(), _partition(NULL) 
    { }
    virtual ~action_t() { 
        _partition = NULL;
        _keys.clear();
    }


    // copying allowed
    action_t(action_t const& rhs) 
        : base_action_t(rhs), _keys(rhs._keys), _partition(rhs._partition)
    { }

    action_t& operator=(action_t const& rhs) {
        if (this != &rhs) {
            base_action_t::operator=(rhs);
            _keys = rhs._keys;
            _partition = rhs._partition;
        }
        return (*this);
    }

    
    // access methods

    vector<Key*>* keys() { return (&_keys); }    

    inline Partition* get_partition() const { return (_partition); }
    inline void set_partition(Partition* ap) {
        assert (ap);
        _partition = ap;
    }
   
    
    // INTEFACE

    virtual w_rc_t trx_exec()=0;
    virtual const bool trx_acq_locks()=0;
    virtual void giveback()=0;

    const int trx_rel_locks(BaseActionPtrList& readyList, 
                            BaseActionPtrList& promotedList) 
    {
        assert (_partition);
        return (_partition->release(this,readyList,promotedList));
    }

    void notify() {
        assert (_partition);
        _partition->enqueue_commit(this);
    }
    
}; // EOF: action_t



EXIT_NAMESPACE(dora);

#endif /** __DORA_ACTION_H */

