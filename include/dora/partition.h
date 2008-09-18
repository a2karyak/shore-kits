/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file partition.h
 *
 *  @brief Encapsulation of each table partition in DORA
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __DORA_PARTITION_H
#define __DORA_PARTITION_H

#include <cstdio>

#include "util.h"

#include "sm/shore/shore_env.h"
#include "sm/shore/shore_table.h"

#include "dora/action.h"


ENTER_NAMESPACE(dora);


using namespace shore;



/******************************************************************** 
 *
 * @enum:  PartitionType
 *
 * @brief: Possible types of a data partition
 *
 ********************************************************************/

enum PartitionType { PT_UNDEF, PT_RANGE, PT_HASH };


/******************************************************************** 
 *
 * @enum:  PartitionOwnerState
 *
 * @brief: Owning status for the data partition
 *
 ********************************************************************/

enum PartitionOwnerState { POS_UNDEF, POS_SINGLE, POS_MULTIPLE };



/******************************************************************** 
 *
 * @class: partition_t
 *
 * @brief: Abstract class for the data partitions
 *
 ********************************************************************/

template <class DataType>
class partition_t
{
public:
    typedef action_t<DataType> part_action;

protected:

    ShoreEnv*           _env;    
    table_desc_t*       _table;

    PartitionType       _part_type;
    PartitionOwnerState _part_owner;

public:

    partition_t(ShoreEnv* env, table_desc_t* ptable) 
        : _env(env), _table(ptable),
          _part_type(PT_UNDEF), _part_owner(POS_UNDEF)
    {
        assert (_env);
        assert (_table);
    }

    virtual ~partition_t() { }    


    const PartitionType get_part_type() { return (_part_type); }
    const PartitionOwnerState get_part_owner_state() { return (_part_owner); }


    /** returns true if action can be enqueued here */
    virtual const bool verify(part_action* pAction)=0;

    /** enqueues action, false on error */
    virtual const bool enqueue(part_action* pAction)=0;    

    /** dequeues action */
    part_action* dequeue();


}; // EOF: partition_t



/****************************************************************** 
 *
 * @fn:    dequeue()
 *
 * @brief: Returns the action at the head of the queue
 *
 ******************************************************************/

template <class DataType>
action_t<DataType>* partition_t<DataType>::dequeue() 
{
    TRACE( TRACE_ALWAYS, "dequeuing...\n");

    assert (0); // TODO: dequeue from queue
    // return (queue.get());

    return (NULL);
}


EXIT_NAMESPACE(dora);

#endif /** __DORA_PARTITION_H */

