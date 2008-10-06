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
    key _down;
    key _up;

public:

    range_action_impl() : _action_t() { }

    virtual ~range_action_impl() { }
    
    /** access methods */

    void set_keys(const key& adown, const key& aup) {
        CRITICAL_SECTION(action_cs, _action_lock);
        assert (_keys.size()==0); // make sure using an empty action
        _down = adown;
        _key = aup;
        _keys.push_back(&_down);
        _keys.push_back(&_up);
    }

    key& down() { return (_down); }
    const key& down() const { return (_down); }
    key& up() { return (_up); }
    const key& up() const { return (_up); }
    
    /** trx-related operations */
    virtual w_rc_t trx_exec()=0;             // pure virtual

private:

    // copying not allowed
    range_action_impl(range_action_impl const &);
    void operator=(range_action_impl const &);
    
}; // EOF: range_action_impl



EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_ACTION_H */

