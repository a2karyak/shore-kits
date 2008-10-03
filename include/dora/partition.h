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
#include "dora/srmwqueue.h"
#include "dora/lockman.h"
#include "dora/worker.h"



ENTER_NAMESPACE(dora);


using namespace shore;


int DF_NUM_OF_STANDBY_THRS = 10;


/******************************************************************** 
 *
 * @enum:  PartitionPolicy
 *
 * @brief: Possible types of a data partition
 *
 *         - PP_RANGE  : range partitioning
 *         - PP_HASH   : hash-based partitioning
 *         - PP_PREFIX : prefix-based partitioning (predicate)
 *
 ********************************************************************/

enum PartitionPolicy { PP_UNDEF, PP_RANGE, PP_HASH, PP_PREFIX };


/******************************************************************** 
 *
 * @enum:  PartitionActiveThreadState
 *
 * @brief: Owning status for the data partition
 *
 *         - PATS_SINGLE   : only the primary-owner thread is active
 *         - PATS_MULTIPLE : some of the standby threads are active
 *
 ********************************************************************/

enum PartitionActiveThreadState { PATS_UNDEF, PATS_SINGLE, PATS_MULTIPLE };



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

    typedef action_t<DataType>      part_action;
    typedef worker_t<DataType>      part_worker;
    typedef srmwqueue<part_action>  part_queue;

    typedef key_wrapper_t<DataType> part_key;
    typedef lock_man_t<DataType>    part_lock_man;

protected:

    ShoreEnv*       _env;    
    table_desc_t*   _table;

    PartitionPolicy _part_policy;

    // partition active thread state
    PartitionActiveThreadState volatile _pat_state; 

    // partition active thread count
    int volatile   _pat_count;
    tatas_lock     _pat_count_lock;

    // pointers to primary owner and pool of standby worker threads
    part_worker*   _owner; // primary owner
    tatas_lock     _owner_lock;
    part_worker*   _standby; // standby pool
    tatas_lock     _standby_lock;

    // queue of requests
    part_queue     _queue; 

    // lock manager for the partition
    part_lock_man  _plm;

public:

    partition_t(ShoreEnv* env, table_desc_t* ptable) 
        : _env(env), _table(ptable), 
          _part_policy(PP_UNDEF), _pat_state(PATS_UNDEF), _pat_count(0),
          _owner(NULL), _standby(NULL)
    {
        assert (_env);
        assert (_table);
    }

    virtual ~partition_t() { }    

    /** Access methods */

    // partition policy
    const PartitionPolicy get_part_policy() { return (_part_policy); }
    void set_part_policy(const PartitionPolicy aPartPolicy) {
        assert (aPartPolicy!=PP_UNDEF);
        _part_policy == aPartPolicy;
    }

    // resets the partition
    int reset(const int standby_sz = DF_NUM_OF_STANDBY_THRS);

    // dumps information
    void dump() const {
        TRACE( TRACE_DEBUG, "Policy            (%d)\n", _part_policy);
        TRACE( TRACE_DEBUG, "Active Thr Status (%d)\n", _pat_state);
        TRACE( TRACE_DEBUG, "Active Thr Count  (%d)\n", _pat_count);
        _plm.dump();
    }
        

    // partition state and active threads
    const PartitionActiveThreadState get_pat_state();
    const PartitionActiveThreadState dec_active_thr();
    const PartitionActiveThreadState inc_active_thr();

    // get lock manager
    part_lock_man* plm() { return (&_plm); }


    // returns true if action can be enqueued it this partition
    virtual const bool verify(const part_action& action) const =0;

    // enqueues action, false on error
    const bool enqueue(part_action* paction);    

    // dequeues action
    part_action* dequeue();


private:

    const int _generate_standby_pool(const int sz = DF_NUM_OF_STANDBY_THRS);
    const int _generate_primary();


protected:    

    const int isFree(part_key akey, DoraLockMode lmode);


}; // EOF: partition_t



/** partition_t interface */


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
    TRACE( TRACE_TRX_FLOW, "dequeuing...\n");
    return (_queue.pop());
}



/****************************************************************** 
 *
 * @fn:    enqueue()
 *
 * @brief: Enqueues action, false on error.
 *
 ******************************************************************/

template <class DataType>
const bool partition_t<DataType>::enqueue(part_action* pAction)
{
    assert (_part_policy!=PP_UNDEF);
    if (!verify(*pAction)) {
        TRACE( TRACE_DEBUG, "Try to enqueue to the wrong partition...\n");
        return (false);
    }

    TRACE( TRACE_TRX_FLOW, "Enqueuing...\n");
    _queue.push(pAction);
    return (true);
}



/****************************************************************** 
 *
 * @fn:     reset()
 *
 * @brief:  Resets the partition
 *
 * @param:  (optional) The size of the standby pool
 *
 * @return: Returns 0 on success
 *
 * @note:   Check dora_error.h for error codes
 *
 ******************************************************************/

template <class DataType>
int partition_t<DataType>::reset(const int sz) 
{        
    CRITICAL_SECTION(owner_cs, _owner_lock);
    CRITICAL_SECTION(standby_cs, _standby_lock);
    CRITICAL_SECTION(pat_cs, _pat_count_lock);

    // generate primary
    if (_generate_primary()) {
        TRACE (TRACE_ALWAYS, "Failed to generate primary\n");
        return (de_GEN_PRIMARY_WORKER);
    }

    // generate standby pool
    if (_generate_standby_pool(sz)<sz) {
        TRACE (TRACE_ALWAYS, "Failed to generate pool of (%d)\n");
        return (de_GEN_STANDBY_POOL);
    }

    // set active count to 1
    _pat_count = 1;

    // kick-off primary
    assert (0); // TODO

    return (0);
}    


// active threads functions


/****************************************************************** 
 *
 * @fn:    get_pat_state()
 *
 * @brief: Returns the state of the partition 
 *         (single or multiple threads active)
 *
 ******************************************************************/

template <class DataType>
const PartitionActiveThreadState partition_t<DataType>::get_pat_state() 
{ 
    CRITICAL_SECTION(pat_cs, _pat_count_lock);
    return (_pat_state); 
}



/****************************************************************** 
 *
 * @fn:     dec_active_thr()
 *
 * @brief:  Decreases active thread count by 1.
 *          If thread_count is 1, changes state to PATS_SINGLE
 *
 * @return: Retuns the updated state of the partition
 *
 ******************************************************************/

template <class DataType>
const PartitionActiveThreadState partition_t<DataType>::dec_active_thr() 
{
    assert (_pat_count>1);
    assert (_pat_state==PATS_MULTIPLE);
    CRITICAL_SECTION(pat_cs, _pat_count_lock);
    _pat_count--;
    if (_pat_count==1) {
        _pat_state = PATS_SINGLE;
        return (PATS_SINGLE);
    }
    assert (_pat_count>0);
    return (PATS_MULTIPLE);
}


/****************************************************************** 
 *
 * @fn:     inc_active_thr()
 *
 * @brief:  Increases active thread count by 1.
 *          If thread_count is >1, changes state to PATS_MULTIPLE
 *
 * @return: Retuns the updated state of the partition
 *
 ******************************************************************/

template <class DataType>
const PartitionActiveThreadState partition_t<DataType>::inc_active_thr() 
{
    CRITICAL_SECTION(pat_cs, _pat_count_lock);
    _pat_count++;
    if (_pat_count>1) {
        _pat_state = PATS_MULTIPLE;
        return (PATS_MULTIPLE);
    }
    assert (_pat_count>0);
    return (_pat_state);
}



/** partition_t helper functions */



/****************************************************************** 
 *
 * @fn:     _generate_standby_pool()
 *
 * @brief:  Generates (sz) standby worker threads, linked together
 *
 * @return: Retuns the number of threads actually generated
 *
 * @note:   Assumes lock on pool head pointer is already acquired
 *
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::_generate_standby_pool(const int sz)
{
    assert (sz>0);

    assert (0); // TODO

    int i=0;
    for (i=0; i<sz; i++) {
        // 1. generate worker
        // 2. set it as previous next
        // 3. set first as head of pool
    }
    
    return (i);
}


/****************************************************************** 
 *
 * @fn:     _generate_primary()
 *
 * @brief:  Generates a worker thread and promotes it to primary owner
 *
 * @return: Retuns 0 on sucess
 *
 * @note:   Assumes lock on owner pointer is already acquired
 *
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::_generate_primary() 
{
    assert (0); // TODO

    // 1. create worker thread
    // 2. set self as worker's partition
    // 3. set worker state as primary
    // 4. update partion owner pointer

    return (1);
}



EXIT_NAMESPACE(dora);

#endif /** __DORA_PARTITION_H */

