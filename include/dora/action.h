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

public:

    base_action_t() :
        _prvp(NULL), _xct(NULL)
    { }

    virtual ~base_action_t() { }

    // access methods
    inline rvp_t* get_rvp() { return (_prvp); }
    inline xct_t* get_xct() { return (_xct); }    
    inline tid_t get_tid() { return (_tid); }

    inline void set(tid_t atid, xct_t* axct, rvp_t* prvp) {
        CRITICAL_SECTION(action_cs, _action_lock);
        _tid = atid;
        _xct = axct;
        _prvp = prvp;
    }


    // interface

    // executes action body
    virtual w_rc_t trx_exec()=0;          

    // acquires the required locks in order to proceed
    virtual const bool trx_acq_locks()=0;

    // releases acquired locks
    virtual void trx_rel_locks()=0;

    // enqueues self on the committed list of committed actions
    virtual void notify()=0; 


    // lock for the whole action
    tatas_lock     _action_lock;    

private:

    // copying not allowed
    base_action_t(base_action_t const &);
    void operator=(base_action_t const &);

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
    typedef key_lm_t<DataType>       LockRequest;  // pair of <Key,LM>
    typedef vector<LockRequest>      LockRequestVec;
    typedef partition_t<DataType>    Partition;

protected:

    // a vector of pointers to keys
    vector<Key*>   _keys;

    //pointer to the partition
    Partition*     _partition;

public:

    action_t() : 
        base_action_t(), _partition(NULL) 
    { }

    virtual ~action_t() { }

    
    // access methods
    vector<Key*> *keys() { return (&_keys); }    

    inline Partition* get_partition() const { return (_partition); }
    inline void set_partition(Partition* ap) {
        assert (ap);
        _partition = ap;
    }
    
    
    // interface
    virtual w_rc_t trx_exec()=0;
    virtual const bool trx_acq_locks()=0;

    void trx_rel_locks() {
        assert (_partition);
        _partition->release(_tid);
    }
    void notify() {
        assert (_partition);
        _partition->enqueue_commit(this);
    }


    // for the cache
    void reset() {
        CRITICAL_SECTION(action_cs, _action_lock);
        _xct = NULL;
        _prvp = NULL;
        _keys.clear();
    }

private:

    // copying not allowed
    action_t(action_t const &);
    void operator=(action_t const &);
    
}; // EOF: action_t


EXIT_NAMESPACE(dora);

#endif /** __DORA_ACTION_H */

