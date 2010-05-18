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

/** @file:   range_action.h
 *
 *  @brief:  Encapsulation of each range-based actions
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_RANGE_ACTION_H
#define __DORA_RANGE_ACTION_H


#include "dora/action.h"

using namespace shore;

ENTER_NAMESPACE(dora);



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
public:
    typedef key_wrapper_t<DataType>  Key;
    typedef action_t<DataType>       Action;
    typedef KALReq_t<DataType>       KALReq;

protected:

    // the range of keys
    Key _down;
    Key _up;

    inline void _range_act_set(xct_t* axct, const tid_t& atid, rvp_t* prvp, 
                               const int keylen) 
    {
        // the range action has two keys: 
        // the low and hi value that determine the range
        Action::_act_set(axct,atid,prvp,2); 
        assert (keylen);
        _down.reserve(keylen);
        _up.reserve(keylen);
    }

public:

    range_action_impl()
        : Action()
    { }

    range_action_impl(const range_action_impl& rhs)
        : Action(rhs), _down(rhs._down), _up(rhs._up)
    { assert (0); }

    range_action_impl operator=(const range_action_impl& rhs)
    {
        if (this != &rhs) {
            assert (0); // IP: TODO check
            Action::operator=(rhs);
            _down = rhs._down;
            _up = rhs._up;
        }
        return (*this);
    }

    virtual ~range_action_impl() 
    { 
//         if (_down) delete (_down);
//         if (_up) delete (_up);
    }    
    

    // INTERFACE 

    virtual void calc_keys()=0; 

    virtual w_rc_t trx_exec()=0;

    virtual int trx_upd_keys()
    {
        if (base_action_t::_keys_set) return (1); // if already set do not recalculate

        calc_keys();

        assert (Action::_keys.empty()); // make sure using an empty action
        Action::_keys.push_back(&_down);
        Action::_keys.push_back(&_up);

        eDoraLockMode req_lm = DL_CC_EXCL;
        if (base_action_t::is_read_only()) req_lm = DL_CC_SHARED;

        base_action_t::setkeys(1); // indicates that it needs only 1 key
        KALReq akr(this,req_lm,&_down);
        Action::_requests.push_back(akr); // range action

        base_action_t::_keys_set = 1;
        return (0);
    }

    virtual void giveback()=0;

    virtual void setup(Pool** /* stl_pool_list */)
    {
        //        Action::setup(stl_pool_list);

        // it must have 3 pool lists
        // stl_pool_list[0]: KeyPtr pool
        // stl_pool_list[1]: KALReq pool
        // stl_pool_list[2]: DataType pool (for the Keys)
//         assert (stl_pool_list[2]);         

//         _down = new Key( stl_pool_list[2] );
//         _up = new Key( stl_pool_list[2] );
    }

    virtual void reset()
    {
        Action::reset();

        // clear contents
        _down.reset();
        _up.reset();
    }

}; // EOF: range_action_impl



EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_ACTION_H */

