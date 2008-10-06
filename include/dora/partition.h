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

#include "dora.h"


ENTER_NAMESPACE(dora);


using namespace shore;


template<class DataType> class worker_t;



/******************************************************************** 
 *
 * @enum:  PATState
 *
 * @brief: Working thread status for the partition
 *
 *         - PATS_SINGLE   : only the primary-owner thread is active
 *         - PATS_MULTIPLE : some of the standby threads are active
 *
 ********************************************************************/

enum PATState { PATS_UNDEF, PATS_SINGLE, PATS_MULTIPLE };



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

    int             _part_id;
    PartitionPolicy _part_policy;

    // partition active thread state and count
    PATState volatile _pat_state; 
    int volatile      _pat_count;
    tatas_lock        _pat_count_lock;

    // pointers to primary owner and pool of standby worker threads
    part_worker*   _owner;        // primary owner
    tatas_lock     _owner_lock;

    part_worker*   _standby;      // standby pool
    int            _standby_cnt;
    tatas_lock     _standby_lock;

    // processor binding
    processorid_t  _prs_id;

    // queue of requests
    part_queue     _queue; 

    // lock manager for the partition
    part_lock_man  _plm;

public:

    partition_t(ShoreEnv* env, table_desc_t* ptable, int apartid = 0, 
                processorid_t aprsid = PBIND_NONE) 
        : _env(env), _table(ptable), 
          _part_policy(PP_UNDEF), _pat_state(PATS_UNDEF), _pat_count(0),
          _owner(NULL), _standby(NULL), _standby_cnt(DF_NUM_OF_STANDBY_THRS),
          _part_id(apartid), _prs_id(PBIND_NONE)
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

    // partition state and active threads
    const PATState get_pat_state();
    const PATState dec_active_thr();
    const PATState inc_active_thr();

    // get lock manager
    part_lock_man* plm() { return (&_plm); }



    /** Control partition */

    // resets/initializes the partition, possibly to a new processor
    virtual const int reset(const processorid_t aprsid = PBIND_NONE,
                            const int standby_sz = DF_NUM_OF_STANDBY_THRS);


    // stops the partition
    virtual void stop();


    /** Action-related methods */

    // returns true if action can be enqueued it this partition
    virtual const bool verify(part_action& action)=0;

    // enqueues action, 0 on success
    const int enqueue(part_action* paction);    

    // dequeues action
    part_action* dequeue(WorkerControl volatile* pwc);



    /** For debugging */

    // dumps information
    void dump() const {
        TRACE( TRACE_DEBUG, "Policy            (%d)\n", _part_policy);
        TRACE( TRACE_DEBUG, "Active Thr Status (%d)\n", _pat_state);
        TRACE( TRACE_DEBUG, "Active Thr Count  (%d)\n", _pat_count);
        _plm.dump();
    }
        


private:

    // thread control
    const int    _start_owner();
    const int    _stop_threads();
    const int    _generate_primary();
    const int    _generate_standby_pool(const int sz, 
                                       int& pool_sz,
                                       const processorid_t aprsid = PBIND_NONE);
    part_worker* _generate_worker(const processorid_t aprsid, c_str wname);    

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
action_t<DataType>* partition_t<DataType>::dequeue(WorkerControl volatile* pwc)
{
    TRACE( TRACE_TRX_FLOW, "Dequeuing...\n");
    return (_queue.pop(pwc));
}



/****************************************************************** 
 *
 * @fn:     enqueue()
 *
 * @brief:  Enqueues action
 *
 * @return: 0 on success, see dora_error.h for error codes
 *
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::enqueue(part_action* pAction)
{
    assert (_part_policy!=PP_UNDEF);
    if (!verify(*pAction)) {
        TRACE( TRACE_DEBUG, "Try to enqueue to the wrong partition...\n");
        return (de_WRONG_PARTITION);
    }

    TRACE( TRACE_TRX_FLOW, "Enqueuing...\n");
    _queue.push(pAction);
    return (0);
}



/****************************************************************** 
 *
 * @fn:     stop()
 *
 * @brief:  Stops the partition
 *          - Stops and deletes all workers
 *          - Clears data structures
 *
 ******************************************************************/

template <class DataType>
void partition_t<DataType>::stop()
{        
    TRACE( TRACE_DEBUG, "Stopping..\n");

    // 1. stop the worker & standby threads
    _stop_threads();

    // 2. clear queue 
    _queue.clear();
    
    // 3. reset lock-manager
    _plm.reset();
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
const int partition_t<DataType>::reset(const processorid_t aprsid,
                                       const int poolsz)
{        
    TRACE( TRACE_DEBUG, "part (%s-%d) - pool (%d) - cpu (%d)\n", 
           _table->name(), _part_id, poolsz, aprsid);

    // 1. stop the worker & standby threads
    _stop_threads();

    // 2. clear queue 
    _queue.clear();
    
    // 3. reset lock-manager
    _plm.reset();


    // 4. lock the primary and standby pool and generate workers
    CRITICAL_SECTION(owner_cs, _owner_lock);
    CRITICAL_SECTION(standby_cs, _standby_lock);
    CRITICAL_SECTION(pat_cs, _pat_count_lock);    

    _prs_id = aprsid;

    // generate primary
    if (_generate_primary()) {
        TRACE (TRACE_ALWAYS, "Failed to generate primary thread\n");
        assert (0); // should not happen
        return (de_GEN_PRIMARY_WORKER);
    }

    // set a single active thread
    _pat_count = 1;
    _pat_state = PATS_SINGLE;

    // kick-off primary
    _start_owner();

    // generate standby pool   
    if (_generate_standby_pool(poolsz, _standby_cnt, aprsid)) {
        TRACE (TRACE_ALWAYS, "Failed to generate pool of (%d) threads\n", poolsz);
        TRACE (TRACE_ALWAYS, "Pool of only (%d) threads generated\n", _standby_cnt);
        return (de_GEN_STANDBY_POOL);
    }
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
const PATState partition_t<DataType>::get_pat_state() 
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
const PATState partition_t<DataType>::dec_active_thr() 
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
const PATState partition_t<DataType>::inc_active_thr() 
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
 * @fn:     _start_owner()
 *
 * @brief:  Kick starts the owner
 *
 ******************************************************************/

template <class DataType>
inline const int partition_t<DataType>::_start_owner()
{
    assert (_owner);
    _owner->start();
    return (0);
}


/****************************************************************** 
 *
 * @fn:     _stop_threads()
 *
 * @brief:  Sends a stop message to all the workers
 *
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::_stop_threads()
{
    int i = 0;

    // owner
    CRITICAL_SECTION(owner_cs, _owner_lock);
    if (_owner) {
        _owner->stop();
        _owner->join();
        delete (_owner);
        ++i;
    }    
    _owner = NULL; // join()?

    // standy
    CRITICAL_SECTION(standby_cs, _standby_lock);
    if (_standby) {
        _standby->stop();
        _standby->join();
        delete (_standby);
        ++i;
    }
    _standby = NULL; // join()?
    _standby_cnt = 0;

    TRACE( TRACE_DEBUG, "part (%s-%d) - (%d) threads stopped\n",
           _table->name(), _part_id, i);

    // thread stats
    CRITICAL_SECTION(pat_cs, _pat_count_lock);
    _pat_count = 0;
    _pat_state = PATS_UNDEF;

    return (0);
}


/****************************************************************** 
 *
 * @fn:     _generate_standby_pool()
 *
 * @brief:  Generates (sz) standby worker threads, linked together
 *
 * @return: 0 on sucess
 *
 * @note:   Assumes lock on pool head pointer is already acquired
 * @note:   The second param will be set equal with the number of
 *          threads actually generated (_standby_cnt)
 *
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::_generate_standby_pool(const int sz,
                                                        int& pool_sz,
                                                        const processorid_t aprsid)
{
    assert (!_standby); // prevent losing thread pointer 

    part_worker* pworker = NULL;
    part_worker* pprev_worker = NULL;
    pool_sz=0;

    if (sz>0) {
        // 1. generate the head of standby pool

        // (ip) We can play with the binding of the standby threads
        pworker = _generate_worker(_prs_id, 
                                   c_str("%s-P-%d-STBY-%d", _table->name(), _part_id, pool_sz));
        if (!pworker) {
            TRACE( TRACE_ALWAYS, "Problem generating worker thread (%d)\n", pool_sz);
            return (de_GEN_WORKER);
        }

        ++pool_sz;
        _standby = pworker;      // set head of pool
        pprev_worker = pworker;

        _standby->fork();
                                   
        TRACE( TRACE_DEBUG, "Head standby worker thread forked\n");  

        // generate the rest of the pool
        for (pool_sz=1; pool_sz<sz; pool_sz++) {
            // 2. generate worker
            pworker = _generate_worker(_prs_id,
                                       c_str("%s-P-%d-STBY-%d", _table->name(), _part_id, pool_sz));
            if (!pworker) {
                TRACE( TRACE_ALWAYS, "Problem generating worker thread (%d)\n", pool_sz);
                return (de_GEN_WORKER);
            }

            // 3. add to linked list
            pprev_worker->set_next(pworker);
            pprev_worker = pworker;

            _standby->fork();

            TRACE( TRACE_DEBUG, "Standby worker (%d) thread forked\n", pool_sz);
        }
    }    
    return (0);
}


/****************************************************************** 
 *
 * @fn:     _generate_primary()
 *
 * @brief:  Generates a worker thread, promotes it to primary owner, 
 *          and forks it
 *
 * @return: Retuns 0 on sucess
 *
 * @note:   Assumes lock on owner pointer is already acquired
 *
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::_generate_primary() 
{
    assert (!_owner); // prevent losing thread pointer 

    part_worker* pworker = _generate_worker(_prs_id, 
                                            c_str("%s-P-%d-PRI",_table->name(), _part_id));
    if (!pworker) {
        TRACE( TRACE_ALWAYS, "Problem generating worker thread\n");
        return (de_GEN_WORKER);
    }

    _owner = pworker;
    _owner->set_data_owner_state(DOS_ALONE);

    _owner->fork();

    TRACE( TRACE_DEBUG, "Owner worker thread forked\n");
    return (0);
}



/****************************************************************** 
 *
 * @fn:     _generate_worker()
 *
 * @brief:  Generates a worker thread and assigns it to the partition
 *
 * @return: Retuns 0 on sucess
 *
 ******************************************************************/

template <class DataType>
inline worker_t<DataType>* partition_t<DataType>::_generate_worker(const processorid_t prsid,
                                                                   c_str strname) 
{
    // 1. create worker thread
    // 2. set self as worker's partition
    part_worker* pworker = new part_worker(_env, this, strname, prsid); 
    return (pworker);
}



EXIT_NAMESPACE(dora);

#endif /** __DORA_PARTITION_H */

