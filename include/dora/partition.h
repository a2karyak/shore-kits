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


/******************************************************************** 
 *
 * @enum:  ePATState
 *
 * @brief: Working thread status for the partition
 *
 *         - PATS_SINGLE   : only the primary-owner thread is active
 *         - PATS_MULTIPLE : some of the standby threads are active
 *
 ********************************************************************/

enum ePATState { PATS_UNDEF, PATS_SINGLE, PATS_MULTIPLE };



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

    typedef action_t<DataType>      PartAction;
    typedef worker_t<DataType>      PartWorker;

    typedef srmwqueue<PartAction>          PartQueue;
    typedef std::list<PartAction*>         ActionList;
    typedef typename ActionList::iterator  ActionListIt; 

    typedef key_wrapper_t<DataType>           PartKey;
    typedef key_lm_t<DataType>                LockRequest;  // pair of <Key,LM>
    typedef vector<LockRequest>               LockRequestVec;
    typedef typename LockRequestVec::iterator LockRequestVecIt;

    typedef lock_man_t<DataType>              PartLockManager;

protected:

    ShoreEnv*       _env;    
    table_desc_t*   _table;

    int              _part_id;
    ePartitionPolicy _part_policy;

    // partition active thread state and count
    ePATState volatile _pat_state; 
    int volatile       _pat_count;
    tatas_lock         _pat_count_lock;

    // pointers to primary owner and pool of standby worker threads
    PartWorker*   _owner;        // primary owner
    tatas_lock    _owner_lock;

    PartWorker*   _standby;      // standby pool
    int           _standby_cnt;
    tatas_lock    _standby_lock;

    // processor binding
    processorid_t _prs_id;

    // lock manager for the partition
    PartLockManager  _plm;


    // Each partition has three lists of Actions
    //
    // 1. (_committed_list)
    //    The list of actions that were committed.
    //    If an action is committed the partition's lock manager needs to remove
    //    all the locks occupied by this particular action.
    //
    // 2. (_input_queue)
    //    The queue were multiple writers input new requests (Actions).
    //    This is the main entrance point for new requests for the partition. 
    //  
    // 3. (_wait_list)
    //    A list of actions that were dequeued by the input queue but
    //    the lock manager didn't allow them to proceed due to (logical lock) 
    //    conflicts with other outstanding transactions.
    //
    //
    // The active worker thread does the following cycle:
    //
    // - Checks if there is any action to be committed, and releases the 
    //   corresponding locks.
    // - If indeed there were actions committed goes over the list of 
    //   waiting (unserved) actions since now some of them may be able
    //   to proceed.
    // - If no actions were committed and the list of waiting is searched
    //   then is goes to the queue and waits for new inputs
    //

    // queue of new Actions
    // single reader - multiple writers
    PartQueue    _input_queue; 

    // list of Actions that have been dequeued but not served, because
    // there was a lock conflict with an outstanding trx
    ActionList    _wait_list;
    ActionListIt  _wait_it;

    // queue of committed Actions
    // single reader - multiple writers
    PartQueue    _committed_queue;

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

        _wait_it = _wait_list.begin();
    }


    virtual ~partition_t() { }    

    /** Access methods */

    // partition policy
    const ePartitionPolicy get_part_policy() { return (_part_policy); }
    void set_part_policy(const ePartitionPolicy aPartPolicy) {
        assert (aPartPolicy!=PP_UNDEF);
        _part_policy = aPartPolicy;
    }

    // partition state and active threads
    const ePATState get_pat_state();
    const ePATState dec_active_thr();
    const ePATState inc_active_thr();

    // get lock manager
    PartLockManager* plm() { return (&_plm); }


    /** Control partition */

    // resets/initializes the partition, possibly to a new processor
    virtual const int reset(const processorid_t aprsid = PBIND_NONE,
                            const int standby_sz = DF_NUM_OF_STANDBY_THRS);


    // stops the partition
    virtual void stop();


    /** Action-related methods */

    // releases a trx
    void release(tid_t& atid) { _plm.release(atid); }
    const bool acquire(tid_t& atid, LockRequestVec& arvec);

    // returns true if action can be enqueued it this partition
    virtual const bool verify(PartAction& action)=0;


    // enqueue lock needed to enforce an ordering across trxs
    mcs_lock _enqueue_lock;


    // input for normal actions
    // enqueues action, 0 on success
    const int enqueue(PartAction* pAction);
    PartAction* dequeue();
    const bool is_input_owner(base_worker_t* aworker) {
        return (_input_queue.is_control(aworker));
    }

    // deque of actions to be committed
    const int enqueue_commit(PartAction* apa);
    PartAction* dequeue_commit();
    inline int has_committed(void) const { return (!_committed_queue.is_empty()); }
    const bool is_committed_owner(base_worker_t* aworker) {
        return (_committed_queue.is_control(aworker));
    }

    // list of waiting actions - taken from input queue but not served
    const int enqueue_wait(PartAction* apa);
    int has_waiting(void) const { return (!_wait_list.empty()); }
    PartAction* get_first_wait(void); // takes the iterator to the beginning of the list
    PartAction* get_next_wait(void);  // goes to the next action on the list
    void remove_wait(void); // removes the currently pointed action


    /** For debugging */

    // dumps information
    void dump() const {
        TRACE( TRACE_DEBUG, "Policy            (%d)\n", _part_policy);
        TRACE( TRACE_DEBUG, "Active Thr Status (%d)\n", _pat_state);
        TRACE( TRACE_DEBUG, "Active Thr Count  (%d)\n", _pat_count);
        _plm.dump();
    }



private:

    // lock-man
    inline const bool _acquire_key(tid_t& atid, LockRequest& alr) {
        return (_plm.acquire(atid,alr));
    }
                

    // thread control
    const int _start_owner();
    const int _stop_threads();
    const int _generate_primary();
    const int _generate_standby_pool(const int sz, 
                                     int& pool_sz,
                                     const processorid_t aprsid = PBIND_NONE);
    PartWorker* _generate_worker(const processorid_t aprsid, c_str wname);    

protected:    

    const int isFree(PartKey akey, eDoraLockMode lmode);


}; // EOF: partition_t



/** partition_t interface */



/****************************************************************** 
 *
 * @fn:     acquire()
 *
 * @brief:  Tries to acquire all the locks from list of keys, if it
 *          fails it releases any acquired.
 *
 * @return: (true) on success
 *
 ******************************************************************/

template <class DataType>
const bool partition_t<DataType>::acquire(tid_t& atid, LockRequestVec& arvec)
{
    for (int i=0; i<arvec.size(); ++i) {
        if (!_acquire_key(atid, arvec[i])) {
            // if a key cannot be acquired, 
            // release all the locks and return false
            release(atid);
            TRACE( TRACE_TRX_FLOW, "Cannot acquire all for (%d)\n", atid);
            return (false);
        }
    }

    TRACE( TRACE_TRX_FLOW, "Acquired all for (%d)\n", atid);
    return (true);
}


/****************************************************************** 
 *
 * @fn:     enqueue()
 *
 * @brief:  Enqueues action at the input queue
 *
 * @return: 0 on success, see dora_error.h for error codes
 *
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::enqueue(PartAction* pAction)
{
    assert (_part_policy!=PP_UNDEF);
    if (!verify(*pAction)) {
        TRACE( TRACE_DEBUG, "Try to enqueue to the wrong partition...\n");
        return (de_WRONG_PARTITION);
    }

    pAction->set_partition(this);
    _input_queue.push(pAction);
    return (0);
}



/****************************************************************** 
 *
 * @fn:    dequeue()
 *
 * @brief: Returns the action at the head of the input queue
 *
 ******************************************************************/

template <class DataType>
action_t<DataType>* partition_t<DataType>::dequeue()
{
    return (_input_queue.pop());
}




/****************************************************************** 
 *
 * @fn:     enqueue_commit()
 *
 * @brief:  Pushes an action to the partition's committed actions list
 *
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::enqueue_commit(PartAction* pAction)
{
    assert (_part_policy!=PP_UNDEF);
    assert (pAction->get_partition()==this);

    TRACE( TRACE_TRX_FLOW, "Enq committed (%d) to (%s-%d)\n", 
           pAction->get_tid(), _table->name(), _part_id);
    _committed_queue.push(pAction);
    return (0);
}



/****************************************************************** 
 *
 * @fn:     dequeue_commit()
 *
 * @brief:  Returns the action at the head of the committed queue
 *
 ******************************************************************/

template <class DataType>
action_t<DataType>* partition_t<DataType>::dequeue_commit()
{
    assert (has_committed());
    return (_committed_queue.pop());
}



/****************************************************************** 
 *
 * @fn:     enqueue_wait()
 *
 * @brief:  Pushes an action to the partition's waiting actions list
 *
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::enqueue_wait(PartAction* pAction)
{
    assert (_part_policy!=PP_UNDEF);
    assert (pAction->get_partition()==this);

    TRACE( TRACE_TRX_FLOW, "Enq waiting (%d)\n", pAction->get_tid());
    _wait_list.push_back(pAction);    
    return (0);
}


/****************************************************************** 
 *
 * @fn:     get_first_wait()
 *
 * @brief:  Takes the iterator to the beginning of the waiting list,
 *          and returns the action
 *
 * @return: NULL if empty. Otherwise, the action
 *
 ******************************************************************/

template <class DataType>
action_t<DataType>* partition_t<DataType>::get_first_wait(void)
{
    if (_wait_list.empty())
        return (NULL);
    _wait_it = _wait_list.begin();
    return (*_wait_it);
}


/****************************************************************** 
 *
 * @fn:     get_next_wait()
 *
 * @brief:  Returns the next action on the list, advances the iterator
 *          by one
 *
 * @return: NULL if empty, or at the end of the list. 
 *          Otherwise, the next action
 *
 ******************************************************************/

template <class DataType>
action_t<DataType>* partition_t<DataType>::get_next_wait(void)
{
    // 1. if empty, return NULL
    if (_wait_list.empty())
        return (NULL);

    // 2. advance iterator by one
    ++_wait_it;

    // 3. if at the end of list, return NULL
    if (_wait_it == _wait_list.end())
        return(NULL);

    // 4. else return the pointer action
    return (*_wait_it);
}


/****************************************************************** 
 *
 * @fn:     remove_wait()
 *
 * @brief:  Removes the currently pointed action from the waiting list
 *
 ******************************************************************/

template <class DataType>
void partition_t<DataType>::remove_wait(void)
{
    assert (!_wait_list.empty());
    TRACE( TRACE_TRX_FLOW, "Removing wait (%d) from (%s-%d)\n", 
           (*_wait_it)->get_tid(), _table->name(), _part_id);
    _wait_it = _wait_list.erase(_wait_it);
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
    TRACE( TRACE_DEBUG, "Stopping part (%s-%d)..\n",
           _table->name(), _part_id);

    // 1. stop the worker & standby threads
    _stop_threads();

    // 2. clear queues
    _input_queue.clear();
    _wait_list.clear();
    _committed_queue.clear();
    
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

    // 2. clear queues
    _input_queue.clear();
    _wait_list.clear();
    _committed_queue.clear();
    
    // 3. reset lock-manager
    _plm.reset();


    // 4. lock the primary and standby pool and generate workers
    CRITICAL_SECTION(owner_cs, _owner_lock);
    CRITICAL_SECTION(standby_cs, _standby_lock);
    CRITICAL_SECTION(pat_cs, _pat_count_lock);    

    _prs_id = aprsid;

    // generate primary
    if (_generate_primary()) {
        TRACE (TRACE_ALWAYS, "part (%s-%d) Failed to generate primary thread\n",
               _table->name(), _part_id);
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
        TRACE (TRACE_ALWAYS, "part (%s-%d) Failed to generate pool of (%d) threads\n",
               _table->name(), _part_id, poolsz);
        TRACE (TRACE_ALWAYS, "part (%s-%d) Pool of only (%d) threads generated\n", 
               _table->name(), _part_id, _standby_cnt);
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
const ePATState partition_t<DataType>::get_pat_state() 
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
const ePATState partition_t<DataType>::dec_active_thr() 
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
const ePATState partition_t<DataType>::inc_active_thr() 
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
 * @note:   Assumes that the owner_cs is already locked
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

    // reset queues' worker control pointers
    _input_queue.set(WS_UNDEF,NULL); 
    _committed_queue.set(WS_UNDEF,NULL); 


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

    PartWorker* pworker = NULL;
    PartWorker* pprev_worker = NULL;
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
 *          sets the queue to point to primary owner's controls,
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
    assert (_owner==NULL); // prevent losing thread pointer 

    PartWorker* pworker = _generate_worker(_prs_id, 
                                            c_str("%s-P-%d-PRI",_table->name(), _part_id));
    if (!pworker) {
        TRACE( TRACE_ALWAYS, "Problem generating worker thread\n");
        return (de_GEN_WORKER);
    }

    _owner = pworker;
    _owner->set_data_owner_state(DOS_ALONE);

    // pass worker thread controls to the two queues
    _input_queue.set(WS_INPUT_Q,_owner);  
    _committed_queue.set(WS_COMMIT_Q,_owner);  

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
    PartWorker* pworker = new PartWorker(_env, this, strname, prsid); 
    return (pworker);
}



EXIT_NAMESPACE(dora);

#endif /** __DORA_PARTITION_H */

