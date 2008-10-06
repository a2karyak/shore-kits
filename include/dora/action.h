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
 * @class: action_t
 *
 * @brief: Abstract template-based class for the actions
 *
 * @note:  Actions are similar with packets in staged dbs
 *
 ********************************************************************/

template <class DataType>
class action_t
{
public:
    
    typedef key_wrapper_t<DataType> key;

protected:

    // rendez-vous point
    rvp_t*         _prvp;

    // a vector of pointers to keys
    vector<key*>   _keys;

    // trx-specific
    xct_t*         _xct; // Not the owner
    tid_t          _tid;

public:

    action_t() : _prvp(NULL) { }

    virtual ~action_t() { }
    
    /** access methods */

    vector<key*> *keys() { return (&_keys); }    


    rvp_t* get_rvp() { return (_prvp); }


    inline xct_t* get_xct() { return (_xct); }
    inline tid_t  get_tid() { return (_tid); }
    
    inline void set_xct(xct_t* axct) {
        assert (axct);
        _xct = axct;
        _tid = ss_m::xct_to_tid(_xct);
        return (_tid);
    }

    inline void set_tid(tid_t atid) {
        _tid = atid;
        _xct = ss_m::tid_to_xct(_tid);
        return (_tid);
    }

    inline void set(tid_t atid, xct_t* axct, rvp_t* prvp) {
        assert (axct);
        assert (atid == ss_m::xct_to_tid(axct));
        assert (prvp);
        _tid = atid;
        _xct = axct;
        _prvp = prvp;
    }

    
    /** trx-related operations */
    virtual w_rc_t trx_exec()=0;          // pure virtual

    // for the cache
    void reset() {
        _xct = NULL;
        _prvp = NULL;
        _keys.clear();
    }

    // lock for the whole action
    tatas_lock     _action_lock;    

private:

    // copying not allowed
    action_t(action_t const &);
    void operator=(action_t const &);
    
}; // EOF: action_t


EXIT_NAMESPACE(dora);

#endif /** __DORA_ACTION_H */

