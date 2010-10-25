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

/** @file:   range_partition.h
 *
 *  @brief:  Range partition class in DORA
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_RANGE_PARTITION_H
#define __DORA_RANGE_PARTITION_H


#include <cstdio>

#include "util.h"
#include "sm/shore/shore_env.h"

#include "dora/range_action.h"
#include "dora/partition.h"

using namespace shore;

ENTER_NAMESPACE(dora);


/******************************************************************** 
 *
 * @enum:  eRangePartitionState
 *
 * @brief: Status for the range data partition
 *
 ********************************************************************/

enum eRangePartitionState { RPS_UNSET, RPS_SET };


/******************************************************************** 
 *
 * @class: range_partition_i
 *
 * @brief: Template-based class for the range data partition
 *
 * @note:  There is a single data type for the comparing fields
 *
 ********************************************************************/

template <class DataType>
class range_partition_i : public partition_t<DataType>
{
public:

    typedef key_wrapper_t<DataType>     Key;    
    typedef action_t<DataType>          Action;
    typedef range_action_impl<DataType> RangeAction;
    typedef partition_t<DataType>       Partition;

private:

    eRangePartitionState _rp_state;

    // The two boundaries as D-keys
    Key _part_down;
    Key _part_up;

    bool _is_between(const Key& contDown, 
                     const Key& contUp) const;

public:

    range_partition_i(ShoreEnv* env, table_desc_t* ptable,
                      const uint keyEstimation,
                      const uint apartid, const cvec_t& down, const cvec_t& up,
                      const processorid_t aprsid = PBIND_NONE);

    ~range_partition_i() { }

    // sets new limits
    bool resize(const Key& downLimit, const Key& upLimit);

    // returns true if action can be enqueued here
    bool verify(Action& rangeaction);

}; // EOF: range_partition_i



/****************************************************************** 
 *
 * Construction
 *
 * @note: Sets the down/up keys based on the passed cvec_t
 *
 ******************************************************************/

template <class DataType>
range_partition_i<DataType>::range_partition_i(ShoreEnv* env, 
                                               table_desc_t* ptable,
                                               const uint keyEstimation,
                                               const uint apartid, 
                                               const cvec_t& down, 
                                               const cvec_t& up,
                                               const processorid_t aprsid)
    : Partition(env, ptable, keyEstimation, apartid, aprsid),
      _rp_state(RPS_UNSET)
{
    Partition::set_part_policy(PP_RANGE);

    // Set the down/up D-keys
    _part_down.readCVec(down);    
    _part_up.readCVec(up);
}


/****************************************************************** 
 *
 * @fn:    _is_between
 *
 * @brief: Returns true if (BaseDown <= ContenderDown) && (ContenderUp < BaseUp)
 *
 ******************************************************************/

template <class DataType>
bool range_partition_i<DataType>::_is_between(const Key& contDown,
                                              const Key& contUp) const
{
    assert (contDown<=contUp);

    // !!! WARNING !!!
    // The partition boundaries should always be on the left side 
    // because their size can be smaller than the size of the key checked
    return ((_part_down<=contDown) && !(_part_up<contUp));
}




/****************************************************************** 
 *
 * @fn:    resize()
 *
 * @brief: Resizes the assigned range partition to the new limits
 *
 ******************************************************************/

template <class DataType>
bool range_partition_i<DataType>::resize(const Key& downLimit, 
                                         const Key& upLimit) 
{
    if (upLimit<downLimit) {
        TRACE( TRACE_ALWAYS, "Error in new bounds\n");
        return (false);
    }

    // copy new limits
    _part_down.copy(downLimit);
    _part_up.copy(upLimit);
    _rp_state = RPS_SET;

    ostringstream out;
    string sout;
    out << _part_down << "-" << _part_up;
    sout = out.str();

    TRACE( TRACE_DEBUG, "RangePartition (%s-%d) resized (%s)\n", 
           Partition::_table->name(), Partition::_part_id, sout.c_str());
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
bool range_partition_i<DataType>::verify(Action& rangeaction)
{
    assert (_rp_state == RPS_SET);
    assert (rangeaction.keys()->size()==2);
    return (_is_between(*rangeaction.keys()->front(), *rangeaction.keys()->back()));
}   



EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_PARTITION_H */

