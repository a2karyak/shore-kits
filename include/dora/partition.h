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

#include "dora/lockman.h"
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


const int ACTIONS_PER_INPUT_QUEUE_POOL_SZ = 60;
const int ACTIONS_PER_COMMIT_QUEUE_POOL_SZ = 60;


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

    typedef action_t<DataType>         Action;
    typedef dora_worker_t<DataType>    Worker;
    typedef srmwqueue<Action>          Queue;
    typedef typename key_wrapper_t<DataType>    Key;
    typedef typename lock_man_t<DataType>       LockManager;

    typedef KALReq_t<DataType> KALReq;
    //typedef typename PooledVec<KALReq>::Type     KALReqVec;
    typedef vector<KALReq>     KALReqVec;


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
    Worker*       _owner;        // primary owner
    tatas_lock    _owner_lock;

    Worker*   _standby;      // standby pool
    int           _standby_cnt;
    tatas_lock    _standby_lock;

    // processor binding
    processorid_t _prs_id;

    // lock manager for the partition
    guard<LockManager>  _plm;


    // Each partition has two lists of Actions
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
    //
    // The active worker thread does the following cycle:
    //
    // - Checks if there is any action to be committed, and releases the 
    //   corresponding locks.
    // - The release of committed actions, may make waiting actions
    //   to grab the locks and be ready to be executed. The worker
    //   executes all those.
    // - If no actions were committe, it goes to the queue and waits for new inputs.
    //

    // queue of new Actions
    // single reader - multiple writers
    guard<Queue>    _input_queue; 

    // queue of committed Actions
    // single reader - multiple writers
    guard<Queue>    _committed_queue;

    // pools for actions for the srmwqueues
    guard<Pool>     _actionptr_input_pool;
    guard<Pool>     _actionptr_commit_pool;


public:

    partition_t(ShoreEnv* env, table_desc_t* ptable, 
                const int keyEstimation,
                const int apartid = 0, 
                processorid_t aprsid = PBIND_NONE) 
        : _env(env), _table(ptable), 
          _part_policy(PP_UNDEF), _pat_state(PATS_UNDEF), _pat_count(0),
          _owner(NULL), _standby(NULL), _standby_cnt(DF_NUM_OF_STANDBY_THRS),
          _part_id(apartid), _prs_id(aprsid)          
    {
        assert (_env);
        assert (_table);

        _plm = new LockManager(keyEstimation);


        _actionptr_input_pool = new Pool(sizeof(Action*),ACTIONS_PER_INPUT_QUEUE_POOL_SZ);
        _input_queue = new Queue(_actionptr_input_pool.get());

        _actionptr_commit_pool = new Pool(sizeof(Action*),ACTIONS_PER_COMMIT_QUEUE_POOL_SZ);
        _committed_queue = new Queue(_actionptr_commit_pool.get());
    }


    virtual ~partition_t() 
    { 
        _input_queue.done();
        _actionptr_input_pool.done();
        
        _committed_queue.done();
        _actionptr_commit_pool.done();
    }    


    //// Access methods ////

    const int part_id() const { return (_part_id); }
    const table_desc_t* table() const { return (_table); } 

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
    LockManager* plm() { return (_plm); }
    


    //// Partition Control ////


    // resets/initializes the partition, possibly to a new processor
    virtual const int reset(const processorid_t aprsid = PBIND_NONE,
                            const int standby_sz = DF_NUM_OF_STANDBY_THRS);


    // stops the partition
    virtual void stop();

    // prepares the partition for a new run
    virtual void prepareNewRun();



    //// Action-related methods ////


    // releases a trx
    inline const int release(Action* action,
                             BaseActionPtrList& readyList, 
                             BaseActionPtrList& promotedList) 
    { 
        return (_plm->release_all(action,readyList,promotedList)); 
    }

    inline const bool acquire(KALReqVec& akalvec) 
    {
        return (_plm->acquire_all(akalvec));
    }


    // returns true if action can be enqueued in this partition
    virtual const bool verify(Action& action)=0;


    // enqueue lock needed to enforce an ordering across trxs
    mcs_lock _enqueue_lock;


    // input for normal actions
    // enqueues action, 0 on success
    const int enqueue(Action* pAction, const bool bWake);
    Action* dequeue();
    inline const int has_input(void) const { 
        return (!_input_queue->is_empty()); 
    }    
    const bool is_input_owner(base_worker_t* aworker) {
        return (_input_queue->is_control(aworker));
    }

    // deque of actions to be committed
    const int enqueue_commit(Action* apa, const bool bWake=true);
    Action* dequeue_commit();
    inline const int has_committed(void) const { 
        return (!_committed_queue->is_empty()); 
    }
    const bool is_committed_owner(base_worker_t* aworker) {
        return (_committed_queue->is_control(aworker));
    }


    const int abort_all_enqueued();



    //// Debugging ////

    // stats
    void statistics(worker_stats_t& gather) {
        assert (_owner); 
        gather += _owner->get_stats();
        _owner->reset_stats();
    }

    // dumps information
    void dump() {
        TRACE( TRACE_DEBUG, "Policy            (%d)\n", _part_policy);
        TRACE( TRACE_DEBUG, "Active Thr Status (%d)\n", _pat_state);
        TRACE( TRACE_DEBUG, "Active Thr Count  (%d)\n", _pat_count);
        _plm->dump();
    }



private:                

    // thread control
    const int _start_owner();
    const int _stop_threads();
    const int _generate_primary();
    const int _generate_standby_pool(const int sz, 
                                     int& pool_sz,
                                     const processorid_t aprsid = PBIND_NONE);
    Worker* _generate_worker(const processorid_t aprsid, c_str wname, const int use_sli);    

protected:    

    const int isFree(Key akey, eDoraLockMode lmode);


}; // EOF: partition_t



/** partition_t interface */



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
const int partition_t<DataType>::enqueue(Action* pAction, const bool bWake)
{
    //assert(verify(*pAction)); // TODO: Enable this
//     if (!verify(*pAction)) {
//         TRACE( TRACE_DEBUG, "Try to enqueue to the wrong partition...\n");
//         return (de_WRONG_PARTITION);
//     }

#ifdef WORKER_VERBOSE_STATS
    pAction->mark_enqueue();
#endif

    pAction->set_partition(this);
    _input_queue->push(pAction,bWake);
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
    return (_input_queue->pop());
}




/****************************************************************** 
 *
 * @fn:     enqueue_commit()
 *
 * @brief:  Pushes an action to the partition's committed actions list
 *
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::enqueue_commit(Action* pAction, const bool bWake)
{
    assert (pAction->get_partition()==this);

    TRACE( TRACE_TRX_FLOW, "Enq committed (%d) to (%s-%d)\n", 
           pAction->tid(), _table->name(), _part_id);
    _committed_queue->push(pAction,bWake);
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
    //assert (has_committed());
    return (_committed_queue->pop());
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
    // 2. stop the worker & standby threads
    _stop_threads();

    // 3. clear queues
    _input_queue->clear();
    _committed_queue->clear();
    
    // 4. reset lock-manager
    _plm->reset();
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
    _input_queue->clear();
    _committed_queue->clear();
    
    // 3. reset lock-manager
    _plm->reset();


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


/****************************************************************** 
 *
 * @fn:     prepareNewRun()
 *
 * @brief:  Prepares the partition for a new run
 *          Clears the lm and the queues (if the have anything)
 *
 ******************************************************************/

template <class DataType>
void partition_t<DataType>::prepareNewRun() 
{
    // make sure that no key is left locked    
    assert (_plm->is_clean());
    _plm->reset();
    _input_queue->clear(false); // clear queue but not remove owner
    _committed_queue->clear(false);
}



//// partition_t helper functions



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
    _input_queue->set(WS_UNDEF,NULL,0,0); 
    _committed_queue->set(WS_UNDEF,NULL,0,0); 


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
    assert (_standby==NULL); // prevent losing thread pointer 

    Worker* pworker = NULL;
    Worker* pprev_worker = NULL;
    pool_sz=0;

    if (sz>0) {
        // 1. generate the head of standby pool

        int use_sli = envVar::instance()->getVarInt("db-worker-sli",0);

        // (ip) We can play with the binding of the standby threads
        pworker = _generate_worker(_prs_id, 
                                   c_str("%s-P-%d-STBY-%d", _table->name(), _part_id, pool_sz),
                                   use_sli);
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
                                       c_str("%s-P-%d-STBY-%d", _table->name(), _part_id, pool_sz),
                                       use_sli);
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

    envVar* pe = envVar::instance();

    int use_sli = pe->getVarInt("db-worker-sli",0);

    Worker* pworker = _generate_worker(_prs_id, 
                                       c_str("%s-P-%d-PRI",_table->name(), _part_id),
                                       use_sli);
    if (!pworker) {
        TRACE( TRACE_ALWAYS, "Problem generating worker thread\n");
        return (de_GEN_WORKER);
    }

    _owner = pworker;
    _owner->set_data_owner_state(DOS_ALONE);

    // read from env params the loopcnt
    string sInpQ = c_str("%s-%s",_table->name(),"inp-q-sz").data();
    string sComQ = c_str("%s-%s",_table->name(),"com-q-sz").data();

    int lc = pe->getVarInt("db-worker-queueloops",0);
    int thres_inp_q = pe->getVarInt(sInpQ,0);
    int thres_com_q = pe->getVarInt(sComQ,0);

    // it is safer the thresholds not to be larger than the client batch size
    int batch_sz = pe->getVarInt("db-cl-batchsz",0);
    if (batch_sz < thres_inp_q) thres_inp_q = batch_sz;
    if (batch_sz < thres_com_q) thres_com_q = batch_sz;

    //TRACE( TRACE_ALWAYS, "%s %d %d\n", _table->name(), thres_inp_q, thres_com_q);

    // pass worker thread controls to the two queues
    _input_queue->set(WS_INPUT_Q,_owner,lc,thres_inp_q);  
    _committed_queue->set(WS_COMMIT_Q,_owner,lc,thres_com_q);  

    _owner->fork();

    //    TRACE( TRACE_DEBUG, "Owner worker thread forked\n");
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
inline dora_worker_t<DataType>* partition_t<DataType>::_generate_worker(const processorid_t prsid,
                                                                        c_str strname,
                                                                        const use_sli) 
{
    // 1. create worker thread
    // 2. set self as worker's partition
    Worker* pworker = new Worker(_env, this, strname, prsid, use_sli); 
    return (pworker);
}




/****************************************************************** 
 *
 * @fn:     abort_all_enqueued()
 *
 * @brief:  Goes over the queue and aborts any unprocessed request
 * 
 ******************************************************************/

template <class DataType>
const int partition_t<DataType>::abort_all_enqueued()
{
    // 1. go over all requests
    Action* pa;
    int reqs_read  = 0;
    int reqs_write = 0;
    int reqs_abt   = 0;

    assert (_owner);

    // go over the readers list
    while (_input_queue->_read_pos != _input_queue->_for_readers->end()) {
        pa = *(_input_queue->_read_pos++);
        ++reqs_read;
        if (_owner->abort_one_trx(pa->xct())) 
            ++reqs_abt;        
    }

    // go over the writers list
    {
        CRITICAL_SECTION(q_cs, _input_queue->_lock);
        for (_input_queue->_read_pos = _input_queue->_for_writers->begin();
             _input_queue->_read_pos != _input_queue->_for_writers->end();
             _input_queue->_read_pos++) 
            {
                pa = *(_input_queue->_read_pos);
                ++reqs_write;
                if (_owner->abort_one_trx(pa->xct())) 
                    ++reqs_abt;
            }
    }

    if ((reqs_read + reqs_write) > 0) {
        TRACE( TRACE_ALWAYS, "(%d) aborted before stopping. (%d) (%d)\n", 
               reqs_abt, reqs_read, reqs_write);
    }
    return (reqs_abt);
}


EXIT_NAMESPACE(dora);

#endif /** __DORA_PARTITION_H */

