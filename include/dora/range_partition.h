/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file range_partition.h
 *
 *  @brief Range partition class in DORA
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __DORA_RANGE_PARTITION_H
#define __DORA_RANGE_PARTITION_H

#include <cstdio>

#include "util.h"
#include "sm/shore/shore_env.h"

#include "dora.h"



ENTER_NAMESPACE(dora);


using namespace shore;



/******************************************************************** 
 *
 * @enum:  RangePartitionState
 *
 * @brief: Status for the range data partition
 *
 ********************************************************************/

enum RangePartitionState { RPS_UNSET, RPS_SET };



/******************************************************************** 
 *
 * @class: range_partition_impl
 *
 * @brief: Template-based class for the range data partition
 *
 * @note:  There is a single data type for the comparing fields
 *
 ********************************************************************/

template <class DataType>
class range_partition_impl : public partition_t<DataType>
{
public:
    
    typedef range_action_impl<DataType> range_action;

private:

    RangePartitionState _rp_state;
    int _part_field_cnt;

    // the two bounds
    part_key _down;
    part_key _up;

    /** helper functions */

    const bool _is_between(const part_key& contDown, 
                           const part_key& contUp) const;

public:

    range_partition_impl(ShoreEnv* env, table_desc_t* ptable,                          
                         const int field_cnt,
                         int apartid = 0, processorid_t aprsid = PBIND_NONE) 
        : partition_t(env, ptable, apartid, aprsid),
          _rp_state(RPS_UNSET),
          _part_field_cnt(field_cnt)
    {
        assert (_part_field_cnt>0);
    }


    ~range_partition_impl() { }    

    const part_key* down() { return (&_down); }
    const part_key* up()   { return (&_up); }    

    // sets new limits
    const bool resize(const part_key& downLimit, const part_key& upLimit);

    // returns true if action can be enqueued here
    const bool verify(part_action& rangeaction);

}; // EOF: range_partition_impl



/** Helper functions */


/****************************************************************** 
 *
 * @fn:    _is_between
 *
 * @brief: Returns true if (BaseDown < ContenderDown) (ContenderUp <= BaseUp)
 *
 ******************************************************************/

template <class DataType>
inline const bool range_partition_impl<DataType>::_is_between(const part_key& contDown,
                                                              const part_key& contUp) const
{
    assert (_down<=_up);
    assert (contDown<=contUp);
    //    cout << "Checking if (" << contDown << " - " << contUp << ") between (" << _down << " - " << _up << ")\n";

    // !!! WARNING !!!
    // The partition boundaries should always be on the left side
    return ((_down<contDown) && !(_up<=contUp));
}




/****************************************************************** 
 *
 * @fn:    resize()
 *
 * @brief: Resizes the assigned range partition to the new limits
 *
 ******************************************************************/

template <class DataType>
inline const bool range_partition_impl<DataType>::resize(const part_key& downLimit, 
                                                         const part_key& upLimit) 
{
    if (upLimit<downLimit) {
        TRACE( TRACE_ALWAYS, "Error in new bounds\n");
        return (false);
    }

    // copy new limits
    _down = downLimit;
    _up = upLimit;
    _rp_state = RPS_SET;

    TRACE( TRACE_DEBUG, "RangePartition resized\n");
    cout << "Down: " << _down << " - Up: " << _up << "\n";
    return (true);
}



/****************************************************************** 
 *
 * @fn:    verify()
 *
 * @brief: Verifies if action can be enqueued here.
 *
 ******************************************************************/
 
template <class DataType>
const bool range_partition_impl<DataType>::verify(part_action& rangeaction)
{
    assert (_rp_state == RPS_SET);
    assert (rangeaction.keys()->size()==2);
    return (_is_between(*rangeaction.keys()->front(), *rangeaction.keys()->back()));
}   



EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_PARTITION_H */

