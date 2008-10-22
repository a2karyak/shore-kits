/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file srmwqueue.h
 *
 *  @brief A single-reader, multiple-writer queue.
 *
 *  Queue size is unbounded and the reader spins while waiting for new
 *  elements to arrive.
 *
 *  @author Ryan Johnson (ryanjohn)
 */

#ifndef __DORA_SRMW_QUEUE_H
#define __DORA_SRMW_QUEUE_H

#include <sthread.h>
#include <vector>

#include "util.h"
#include "dora/action.h"

ENTER_NAMESPACE(dora);

// How many loops it will spin before wait on cond var
const int IDLE_LOOPS = 10000;

template<class Action>
struct srmwqueue 
{
    typedef std::vector<Action*> Action_list;

    Action_list _for_writers;
    Action_list _for_readers;
    typename Action_list::iterator _read_pos;
    mcs_lock _lock;
    int volatile _empty;

    eWorkerControl volatile* _pwc;
    WorkingState* _pws;
    condex* _pcx;
    tatas_lock _ws_lock;    

    srmwqueue() : _empty(true), _pwc(NULL), _pws(NULL), _pcx(NULL)  { }

    ~srmwqueue() { }


    // sets the pointer of the queue to the controls of a specific worker thread
    void set(eWorkerControl volatile* pwc, WorkingState* pws, condex* pcx) {
        CRITICAL_SECTION(ws_cs, _ws_lock);
        // sets variables
        _pwc = pwc;
        _pws = pws;
        _pcx = pcx;
    }


    // spins until new input is set
    bool wait_for_input() 
    {
        assert (_pwc);
        assert (_pws);
        assert (_pcx);

        int loopcnt = 0;

        // 1. initially spin
        CRITICAL_SECTION(start_ws_cs, _ws_lock);
        _pws->set_state(WS_IDLE_LOOP);
        start_ws_cs.exit();

	while (*&_empty) {
	    if (*_pwc != WC_ACTIVE) {
                CRITICAL_SECTION(empty_ws_cs, _ws_lock);
                _pws->set_state(WS_EMPTY);
		return (false);
            }

            // 2. if spinned too much, start waiting on the condvar
            if (++loopcnt > IDLE_LOOPS) {
                loopcnt = 0;
                CRITICAL_SECTION(upg_ws_cs, _ws_lock);
                // check once more if still empty 
                if (*&_empty) {
                    // update state, release lock and sleep on cond var
                    TRACE( TRACE_DEBUG, "CondVar sleeping...\n");
                    _pws->set_state(WS_IDLE_CONDVAR);
                    upg_ws_cs.exit();
                    _pcx->wait();
                    TRACE( TRACE_DEBUG, "CondVar woke...\n");
                    
                    // after it wakes up, should do the loop again.
                    // if something has been pushed then _empty will be false
                    // and it will proceed normally.
                    // if signalled because the worker thread stops it will
                    // do a loop and return false.
                }
            }
	}
    
	{
	    CRITICAL_SECTION(cs, _lock);
	    _for_readers.clear();
	    _for_writers.swap(_for_readers);
	    _empty = true;
	}
    
	_read_pos = _for_readers.begin();
	return (true);
    }
    
    Action* pop() {
        // pops an action, or waits for an action to show up
	if ((_read_pos == _for_readers.end()) && (!wait_for_input()))
	    return (NULL);
	return (*(_read_pos++));
    }

    void push(Action* a) {
        assert (a);

        assert (_pwc);
        assert (_pws);
        assert (_pcx);

        // push action
        CRITICAL_SECTION(cs, _lock);
        _for_writers.push_back(a);
        _empty = false;
        
        // wake up if assigned worker thread sleeping
        CRITICAL_SECTION(ws_cs, _ws_lock);
        if (_pws->get_state() == WS_IDLE_CONDVAR) {
            TRACE( TRACE_DEBUG, "CondVar signaling...\n");
            _pws->set_state(WS_ASSIGNED);
            _pcx->signal();
        }        
    }

    void clear() {
        {
            CRITICAL_SECTION(cs, _lock);
            _for_writers.clear();
        }
        _for_readers.clear();
        _empty = true;
    }    
  
}; // EOF: struct srmwqueue


EXIT_NAMESPACE(dora);

#endif /** __DORA_SRMW_QUEUE_H */

