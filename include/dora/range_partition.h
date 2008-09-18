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

#include "dora/partition.h"


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
private:

    int _part_field_cnt;
    RangePartitionState _rp_state;

    DataType* _down;
    DataType* _up;


    /** helper functions */

    /** returns true if Contender > Base */
    inline const bool is_greater(const DataType* base, const DataType* contender) const;

    /** returns true if Contender <= Base */
    inline const bool is_smaller_equal(const DataType* base, const DataType* contender) const;

    /** returns true if BaseDown < Contender <= BaseUp */
    inline const bool is_between(const DataType* baseDown, 
                                 const DataType* baseUp, 
                                 const DataType* contender) const;

public:

    range_partition_impl(ShoreEnv* env, table_desc_t* ptable, int field_cnt) 
        : partition_t(env, ptable),
          _part_field_cnt(field_cnt),
          _rp_state(RPS_UNSET),
          _down(NULL), _up(NULL)
    {
        assert (_part_field_cnt>0);
        _down = new DataType[_part_field_cnt];
        _up = new DataType[_part_field_cnt];
    }


    ~range_partition_impl() 
    { 
        if (_down)
            delete [] _down;

        if (_up)
            delete [] _up;
    }    


    const DataType* get_down() { return (_down); }
    const DataType* get_up() { return (_up); }
       


    /** sets new limits */
    const bool resize(DataType* downLimit, DataType* upLimit);


    /** returns true if action can be enqueued here */
    const bool verify(part_action* pAction);

    /** enqueues action, false on error */
    const bool enqueue(part_action* pAction);

}; // EOF: range_partition_impl



/** Helper functions */


/****************************************************************** 
 *
 * @fn:    is_greater
 *
 * @brief: Returns true if Contender > Base
 *
 ******************************************************************/

template <class DataType>
inline const bool range_partition_impl<DataType>::is_greater(const DataType* base,
                                                             const DataType* contender) const
{
    assert (base);
    assert (contender);

    for (int i=0; i<_part_field_cnt; i++) {
        if (base[i]>=contender[i]) 
            return (false);
    }
    return (true);
}



/****************************************************************** 
 *
 * @fn:    is_smalller_equal
 *
 * @brief: Returns true if Contender <= Base
 *
 ******************************************************************/

template <class DataType>
inline const bool range_partition_impl<DataType>::is_smaller_equal(const DataType* base,
                                                                   const DataType* contender) const
{
    assert (base);
    assert (contender);

    for (int i=0; i<_part_field_cnt; i++) {
        if (base[i]<contender[i]) 
            return (false);
    }
    return (true);
}



/****************************************************************** 
 *
 * @fn:    is_between
 *
 * @brief: Returns true if BaseDown < Contender <= BaseUp
 *
 ******************************************************************/

template <class DataType>
inline const bool range_partition_impl<DataType>::is_between(const DataType* baseDown,
                                                             const DataType* baseUp,
                                                             const DataType* contender) const
{
    assert (baseDown);
    assert (baseUp);
    assert (contender);

    for (int i=0; i<_part_field_cnt; i++) {
        if ((baseDown[i] > contender[i]) || (baseUp[i]<=contender[i]))
            return (false);
    }
    return (true);
}





/****************************************************************** 
 *
 * @fn:    resize()
 *
 * @brief: Resizes the assigned range partition to the new limits
 *
 ******************************************************************/

template <class DataType>
const bool range_partition_impl<DataType>::resize(DataType* downLimit, DataType* upLimit) 
{
    assert (downLimit);
    assert (upLimit);

    if (is_smalller_equal(downLimit, upLimit)) {
        TRACE( TRACE_ALWAYS, "Error in new bounds\n");
        return (false);
    }

    // copy new limits
    for (int i=0; i<_part_field_cnt; i++) {
        _down[i] = downLimit[i];
        _up[i] = upLimit[i];
    }

    _rp_state = RPS_SET;
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
const bool range_partition_impl<DataType>::verify(part_action* pAction)
{
    assert (_rp_state == RPS_SET);
    return (is_between(_down, _up, pAction->get_value()));
}   



/****************************************************************** 
 *
 * @fn:    enqueue()
 *
 * @brief: Enqueues action, false on error.
 *
 ******************************************************************/

template <class DataType>
const bool range_partition_impl<DataType>::enqueue(part_action* pAction)
{
    assert (_rp_state == RPS_SET);
    if (!verify(pAction)) {
        TRACE( TRACE_ALWAYS, "Enqueued to the wrong partition\n");
        return (false);
    }

    TRACE( TRACE_DEBUG, "enqueuing...\n");

    assert (0); // TODO: enqueue action to partition queue
    // queue.put(pAction);

    return (false);
}



EXIT_NAMESPACE(dora);

#endif /** __DORA_RANGE_PARTITION_H */

