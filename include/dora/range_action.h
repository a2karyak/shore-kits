/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file range_action.h
 *
 *  @brief Encapsulation of each range-based actions
 *
 *  @author Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_RANGE_ACTION_H
#define __DORA_RANGE_ACTION_H

#include "dora/action.h"


ENTER_NAMESPACE(dora);

using namespace shore;


/******************************************************************** 
 *
 * @class: range_action_t
 *
 * @brief: Abstract template-based class for range-based actions
 *
 * @note:  The vector has only two members: {down, up}
 *         Vector.Front = down, Vector.Back = up
 *
 ********************************************************************/

template <class DataType>
class range_action_impl : public action_t<DataType>
{
protected:
    // the range of keys
    Key _down;
    Key _up;

    inline void _range_act_set(tid_t atid, xct_t* axct, rvp_t* prvp, 
                               const int keylen) 
    {
        // the range action has two keys: 
        // the low and hi value that determine the range
        _act_set(atid,axct,prvp,2); 
        assert (keylen);
        _down.reserve(keylen);
        _up.reserve(keylen);
    }

public:

    range_action_impl() :
        action_t<DataType>()
    { }
    virtual ~range_action_impl() { 
        _down.reset();
        _up.reset();
    }
    

    // access methods

    void set_key_range() {
        calc_keys();
        _set_keys();
    }

    virtual void calc_keys()=0; // pure virtual 

    Key& down() { return (_down); }
    const Key& down() const { return (_down); }
    Key& up() { return (_up); }
    const Key& up() const { return (_up); }
    

    // interface 

    virtual w_rc_t trx_exec()=0;
    virtual const bool trx_acq_locks()=0;
    virtual void giveback()=0;

private:

    void _set_keys() {
        assert (_keys.empty()); // make sure using an empty action
        _keys.push_back(&_down);
        _keys.push_back(&_up);
    }

    // copying not allowed
    range_action_impl(range_action_impl const &);
    void operator=(range_action_impl const &);
    
}; // EOF: range_action_impl



EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_ACTION_H */

