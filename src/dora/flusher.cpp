/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   flusher.h
 *
 *  @brief:  The dora-flusher.
 *
 *  @author: Ippokratis Pandis, Feb 2010
 */

#include "dora/flusher.h"
#include "xct.h"

ENTER_NAMESPACE(dora);


/****************************************************************** 
 *
 * @struct: dora_flusher_stats_t
 * 
 ******************************************************************/

// copied from sm/log.h
const uint smBLOCK_SIZE=8192;
const uint smSEGMENT_SIZE=128*smBLOCK_SIZE;

dora_flusher_stats_t::dora_flusher_stats_t()
    : served(0), flushes(0), logsize(0), alreadyFlushed(0), waiting(0),
      trigByXcts(0), trigBySize(0), trigByTimeout(0)
{
}


dora_flusher_stats_t::~dora_flusher_stats_t() 
{
}


void dora_flusher_stats_t::print() const 
{
    static const long MEGABYTE = 1024*1024;

    TRACE( TRACE_STATISTICS, "Flushes:     (%d)\n", flushes);
    TRACE( TRACE_STATISTICS, "Xcts:        (%d)\t(%.2f)\n", 
           served, (double)served/(double)flushes);
    TRACE( TRACE_STATISTICS, "Logsize MBs: (%ld)\t(%.2f)\n", 
           logsize/MEGABYTE, (double)logsize/(double)(flushes*MEGABYTE));
    TRACE( TRACE_STATISTICS, "Already:     (%d)\t(%.2f%%)\n", 
           alreadyFlushed, (double)(100*alreadyFlushed)/(double)served);
    TRACE( TRACE_STATISTICS, "Waiting:     (%d)\t(%.2f)\n", 
           waiting, (double)waiting/(double)flushes);

    TRACE( TRACE_STATISTICS, "By Xcts:     (%d)\t(%.2f%%)\n", 
           trigByXcts,(double)(100*trigByXcts)/(double)flushes);
    TRACE( TRACE_STATISTICS, "By Size:     (%d)\t(%.2f%%)\n", 
           trigBySize,(double)(100*trigBySize)/(double)flushes);
    TRACE( TRACE_STATISTICS, "By Timeout:  (%d)\t(%.2f%%)\n", 
           trigByTimeout,(double)(100*trigByTimeout)/(double)flushes);
}

void dora_flusher_stats_t::reset()
{
    served = 0;
    flushes = 0;

    logsize = 0;
    alreadyFlushed = 0;
    waiting = 0;

    trigByXcts = 0;
    trigBySize = 0;
    trigByTimeout = 0;
}


/******************************************************************** 
 *
 * @struct: dora_flusher_t
 * 
 ********************************************************************/

const int DFLUSHER_BUFFER_EXPECTED_SZ = 3000; // pulling this out of the thin air

dora_flusher_t::dora_flusher_t(ShoreEnv* env, 
                               c_str tname,
                               processorid_t aprsid, 
                               const int use_sli) 
    : base_worker_t(env, tname, aprsid, use_sli)
{ 
    _pxct_toflush_pool = new Pool(sizeof(xct_t*),DFLUSHER_BUFFER_EXPECTED_SZ);
    _toflush = new Queue(_pxct_toflush_pool.get());
    assert (_toflush.get());
    _toflush->set(WS_COMMIT_Q,this,2000,0);  // wake-up immediately, spin 2000

    _pxct_flushing_pool = new Pool(sizeof(xct_t*),DFLUSHER_BUFFER_EXPECTED_SZ);
    _flushing = new Queue(_pxct_flushing_pool.get());
    assert (_flushing.get());
    _flushing->set(WS_COMMIT_Q,this,0,0);  // wake-up immediately

    // Reads the partition size
    _logpart_sz = (envVar::instance()->getSysVarInt("logbufsize"))/8;
    fprintf(stdout, "LogPartitionSize: %d\n", _logpart_sz);
    _logpart_sz = floor(_logpart_sz + smSEGMENT_SIZE - 1, smSEGMENT_SIZE);
    fprintf(stdout, "LogPartitionSize: %d\n", _logpart_sz);

    // Create and start the notifier
    fprintf(stdout, "Starting dora-notifier...\n");
    _notifier = new dora_notifier_t(_env, c_str("DNotifier"));
    assert(_notifier.get());
    _notifier->fork();
    _notifier->start();
}

dora_flusher_t::~dora_flusher_t() 
{ 
    // -- clear queues --
    // they better be empty by now
    assert (_toflush->is_empty());
    _toflush.done();
    _pxct_toflush_pool.done();

    assert (_flushing->is_empty());
    _flushing.done();
    _pxct_flushing_pool.done();
}

int dora_flusher_t::statistics()
{
    _stats.print();
    _stats.reset();
    return (0);
}


/****************************************************************** 
 *
 * @fn:     _work_ACTIVE_impl()
 *
 * @brief:  Implementation of the ACTIVE state (StagedGroupCommit)
 *          The dflusher monitors the toflush queue and decides when
 *          it is good time to issue a flush
 *
 * @uses:   A couple of threshold values to decide whether to flush
 *
 * @return: 0 on success
 * 
 ******************************************************************/

const int DFLUSHER_GROUP_SIZE_THRESHOLD = 100; // Flush every 100 xcts
const int DFLUSHER_LOG_SIZE_THRESHOLD = 200000; // Flush every 200K
const int DFLUSHER_TIME_THRESHOLD = 1000; // Flush every 1000usec (msec)

int dora_flusher_t::_work_ACTIVE_impl()
{    
    envVar* ev = envVar::instance();
    int binding = ev->getVarInt("dora-cpu-binding",0);
    if (binding==0) _prs_id = PBIND_NONE;
    TRY_TO_BIND(_prs_id,_is_bound);

    // read configuration 
    int maxGroupSize = ev->getVarInt("dflusher-group-size",DFLUSHER_GROUP_SIZE_THRESHOLD);
    int maxLogSize = ev->getVarInt("dflusher-log-size",DFLUSHER_LOG_SIZE_THRESHOLD);
    int maxTimeIntervalusec = ev->getVarInt("dflusher-timeout",DFLUSHER_TIME_THRESHOLD);

    uint waiting = 0;
    terminal_rvp_t* prvp = NULL;   
    lsn_t currlsn, durablelsn, xctlsn, maxlsn;
    bool bShouldFlush = false;
    long logWaiting = 0;
    struct timespec start, ts;
    static long const BILLION = 1000*1000*1000;
    bool bSleepNext = false;

    clock_gettime(CLOCK_REALTIME, &start);

    // set timeout
    ts = start;
    ts.tv_nsec += maxTimeIntervalusec * 1000;
    if (ts.tv_nsec > BILLION) {
        ts.tv_nsec -= BILLION;
        ts.tv_sec++;
    }

    // Check if signalled to stop
    while (get_control() == WC_ACTIVE) {        

        // Reset the flags for the new loop
        prvp = NULL;
        set_ws(WS_LOOP);
        bShouldFlush = false;

        // Read the durable lsn
        _env->db()->get_durable_lsn(durablelsn);
        maxlsn = durablelsn;

        // Check if there are xcts waiting at the "to flush" queue 
        while ((!_toflush->is_empty()) || (bSleepNext)) {
            bSleepNext = false;
            _stats.served++;

            // Pop and read the xct info 
            prvp = _toflush->pop();

            // The only way for pop() to return NULL is when signalled to stop
            if (prvp) {
                xctlsn = prvp->my_last_lsn();

                TRACE( TRACE_TRX_FLOW, "TID (%d) lastLSN (%d) durableLSN (%d)\n",
                       prvp->tid(), xctlsn, maxlsn);

                // If the xct is already durable (had been flushed) then
                // send it directly back to the executor of its final-rvp
                if (durablelsn > xctlsn) {
                    //_send_to_last_executor(prvp);
                    _notifier->enqueue_tonotify(prvp);
                    _stats.alreadyFlushed++;
                }
                else {
                    // Otherwise, add the rvp to the syncing (in-flight) list,
                    // and update statistics
                    maxlsn = std::max(maxlsn,xctlsn);
                    _flushing->push(prvp,false);
                    waiting++;
                }
            }
        }

        // Decide whether to flush or not
        if (waiting >= maxGroupSize) {
            // Do we have already too many waiting?
            bShouldFlush = true;
            _stats.trigByXcts++;
        }
        else {
            logWaiting = _log_diff(maxlsn,durablelsn);
            if (logWaiting >= maxLogSize) {
                // Is the log to be flushed already big?
                bShouldFlush = true;
                _stats.trigBySize++;
            }
            else {
                // Not enough requests or log to flush the group 

                // When was the last time we flushed?
                clock_gettime(CLOCK_REALTIME, &start);
                if ((start.tv_sec > ts.tv_sec) ||
                    (start.tv_sec == ts.tv_sec && start.tv_nsec > ts.tv_nsec)) {
                    bShouldFlush = true;
                    _stats.trigByTimeout++;
                    
                    // set next timeout
                    ts = start;
                    ts.tv_nsec += maxTimeIntervalusec * 1000;
                    if (ts.tv_nsec > BILLION) {
                        ts.tv_nsec -= BILLION;
                        ts.tv_sec++;
                    }
                }
                else {
                    // Set a flag which will put it to sleep in the next loop,
                    // unless a new request arrives. But, before sleeping call
                    // for a lazy flush 
                    if (waiting) _env->db()->sync_log();                    
                    bSleepNext = true;
                }
            }
        }

        if (get_control() != WC_ACTIVE) return(0);

        // If yes, flush 
        //
        // IP: Another option is to flush asynchronously at this point and 
        // block on the first "flushing" request that has not been durable 
        // already
        if (bShouldFlush) {
            _stats.flushes++;
            _stats.waiting += waiting;
            _stats.logsize += logWaiting;
            _env->db()->sync_log(); // it will block
            
            waiting = 0;
            logWaiting = 0;
        }

        // At this point we know that everyone on the "flushing" queue is durable
        // Move everybody to the executor of the final-rvp
        
        // Re-read the durable lsn, just for sanity checking
        _env->db()->get_durable_lsn(durablelsn);        
        while (!_flushing->is_empty()) {
            prvp = _flushing->pop();
            xctlsn = prvp->my_last_lsn();
            assert (xctlsn < durablelsn);
            //_send_to_last_executor(prvp);
            _notifier->enqueue_tonotify(prvp);
        }
    }
    return (0);
}



/****************************************************************** 
 *
 * @fn:     _pre_STOP_impl()
 *
 * @brief:  Operations done before the thread stops 
 *
 * @return: 0 on success
 * 
 ******************************************************************/

int dora_flusher_t::_pre_STOP_impl() 
{ 
    uint afterStop = 0;
    terminal_rvp_t* prvp = NULL;

    // Stop notifier
    _notifier->stop();
    _notifier->join();

    // Notify the clients and clean up queues
    while (!_flushing->is_empty()) {
        ++afterStop;
        prvp = _flushing->pop();
        prvp->notify_client();
        prvp->giveback();
    }

    while (!_toflush->is_empty()) {
        ++afterStop;
        prvp = _toflush->pop();
        prvp->notify_client();
        prvp->giveback();
    }

    if (afterStop>0) 
        TRACE( TRACE_ALWAYS, 
               "Xcts flushed at stop (%d)\n",
               afterStop);

    return(0); 
}


/****************************************************************** 
 *
 * @fn:     _log_diff(const lsn_t&, const lsn_t&)
 *
 * @brief:  Returns a rought approximation of the log size between
 *          two lsn_t.
 * 
 ******************************************************************/

long dora_flusher_t::_log_diff(const lsn_t& head, const lsn_t& tail)
{
    if (head.hi()==tail.hi()) return (head.lo()-tail.lo());
    assert(head.hi()>tail.hi());
    long diff = (head.hi() - tail.hi()) * _logpart_sz;
    if (head.lo()>tail.lo()) return (diff + (head.lo()-tail.lo()));
    return (diff - (tail.lo()-head.lo()));
}






/****************************************************************** 
 *
 * @fn:     Contruction/destruction of DNotifier
 *
 ******************************************************************/

dora_notifier_t::dora_notifier_t(ShoreEnv* env, 
                                 c_str tname,
                                 processorid_t aprsid, 
                                 const int use_sli) 
    : base_worker_t(env, tname, aprsid, use_sli)
{ 
    _pxct_tonotify_pool = new Pool(sizeof(xct_t*),DFLUSHER_BUFFER_EXPECTED_SZ);
    _tonotify = new Queue(_pxct_tonotify_pool.get());
    assert (_tonotify.get());
    _tonotify->set(WS_COMMIT_Q,this,0,0);  // wake-up immediately
}

dora_notifier_t::~dora_notifier_t() 
{ 
    // -- clear queues --
    // they better be empty by now
    assert (_tonotify->is_empty());
    _tonotify.done();
    _pxct_tonotify_pool.done();
}


/****************************************************************** 
 *
 * @fn:     _work_ACTIVE_impl()
 *
 * @brief:  Implementation of the ACTIVE state for the notifier in
 *          DORA GroupCommit.
 *
 ******************************************************************/

int dora_notifier_t::_work_ACTIVE_impl()
{    
    envVar* ev = envVar::instance();
    int binding = ev->getVarInt("dora-cpu-binding",0);
    if (binding==0) _prs_id = PBIND_NONE;
    TRY_TO_BIND(_prs_id,_is_bound);

    terminal_rvp_t* prvp = NULL;

    // Check if signalled to stop
    while (get_control() == WC_ACTIVE) {        

        // Reset the flags for the new loop
        set_ws(WS_LOOP);
        
        // It will block if empty
        prvp = _tonotify->pop(); 

        if (prvp) {
            prvp->notify();
            prvp->upd_committed_stats();
            prvp->notify_client();
            prvp->giveback();
            prvp = NULL;
        }
    }
    return (0);
}



/****************************************************************** 
 *
 * @fn:     _pre_STOP_impl()
 *
 * @brief:  Operations done before the thread stops 
 *
 * @return: 0 on success
 * 
 ******************************************************************/

int dora_notifier_t::_pre_STOP_impl() 
{ 
    uint afterStop = 0;
    terminal_rvp_t* prvp = NULL;
    while (!_tonotify->is_empty()) {
        ++afterStop;

        prvp = _tonotify->pop();
        prvp->upd_committed_stats();
        prvp->notify_client();
        prvp->giveback();
        prvp = NULL;
    }

    if (afterStop>0) 
        TRACE( TRACE_ALWAYS, 
               "Xcts notified at stop (%d)\n",
               afterStop);
    return(0); 
}


EXIT_NAMESPACE(dora);
