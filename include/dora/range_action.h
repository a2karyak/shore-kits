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

    inline void _range_act_set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
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

    range_action_impl()
        : action_t<DataType>()
    { }

    range_action_impl(const range_action_impl& rhs)
        : action_t<DataType>(rhs), _down(rhs._down), _up(rhs._up)
    { assert (0); }

    range_action_impl operator=(const range_action_impl& rhs)
    {
        if (this != &rhs) {
            assert (0); // IP: to check
            action_t<DataType>::operator=(rhs);
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

    virtual const int trx_upd_keys()
    {
        if (_keys_set) return (1); // if already set do not recalculate        

        calc_keys();

        assert (_keys.empty()); // make sure using an empty action
        _keys.push_back(&_down);
        _keys.push_back(&_up);

        register eDoraLockMode req_lm = DL_CC_EXCL;
        if (is_read_only()) req_lm = DL_CC_SHARED;

        setkeys(1); // indicates that it needs only 1 key
        _requests.push_back(KALReq(this,req_lm,&_down)); // range action

        _keys_set = 1;
        return (0);
    }

    virtual void giveback()=0;

    virtual void setup(Pool** stl_pool_list)
    {
        //        action_t<DataType>::setup(stl_pool_list);

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
        action_t<DataType>::reset();

        // clear contents
        _down.reset();
        _up.reset();
    }

}; // EOF: range_action_impl



EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_ACTION_H */

