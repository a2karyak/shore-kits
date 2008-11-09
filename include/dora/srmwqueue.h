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
#include "dora/common.h"

ENTER_NAMESPACE(dora);

// How many loops it will spin before wait on cond var
const int IDLE_LOOPS = 1000;

template<class Action>
struct srmwqueue 
{
    typedef std::vector<Action*> ActionVec;

    ActionVec _for_writers;
    ActionVec _for_readers;
    typename ActionVec::iterator _read_pos;
    mcs_lock      _lock;
    int volatile  _empty;

    eWorkingState _my_ws;
    
    // owner thread
    base_worker_t* _owner;
    tatas_lock     _owner_lock;    

    srmwqueue() : 
        _empty(true), _my_ws(WS_UNDEF), _owner(NULL) 
    { 
        _read_pos = _for_readers.begin();
    }

    ~srmwqueue() { }


    // sets the pointer of the queue to the controls of a specific worker thread
    void set(eWorkingState aws, base_worker_t* owner) {
        CRITICAL_SECTION(q_cs, _owner_lock);
        _my_ws = aws;
        _owner = owner;
    }

    // returns true if the passed control is the same
    const bool is_control(base_worker_t* athread) const { return (_owner==athread); }  

    // !!! @note: should be called only by the reader !!!
    int is_empty(void) const {
        return ((_read_pos == _for_readers.end()) && (*&_empty));
    }

    // spins until new input is set
    bool wait_for_input() 
    {
        assert (_owner);
        int loopcnt = 0;

        // 1. start spinning
	while (*&_empty) {

            // 2. if thread was signalled to stop
	    if (_owner->get_control() != WC_ACTIVE) {
                _owner->set_ws(WS_FINISHED);
		return (false);
            }

            // 3. if thread was signalled to go to other queue
            if (!_owner->can_continue(_my_ws)) return (false);
            
            // 4. if spinned too much, start waiting on the condex
            if (++loopcnt > IDLE_LOOPS) {
                loopcnt = 0;
                
                TRACE( TRACE_DEBUG, "Condex sleeping (%d)...\n", _my_ws);
                assert (_my_ws==WS_INPUT_Q); // can sleep only on input queue
                loopcnt = _owner->condex_sleep();
                TRACE( TRACE_DEBUG, "Condex woke (%d) (%d)...\n", _my_ws, loopcnt);

                // after it wakes up, should do the loop again.
                // if something has been pushed then _empty will be false
                // and it will proceed normally.
                // if signalled because it should stop, it will do a loop 
                // and return false.
                // if signalled because it should go to other queue, it will
                // do a loop and return false.
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
        // pops an action from the input vector, or waits for one to show up
	if ((_read_pos == _for_readers.end()) && (!wait_for_input()))
	    return (NULL);
	return (*(_read_pos++));
    }

    void push(Action* a) {
        assert (a);
        assert (_owner);

        // push action
        {
            CRITICAL_SECTION(cs, _lock);
            _for_writers.push_back(a);
            _empty = false;
        }
        
        // wake up if assigned worker thread sleeping
        _owner->set_ws(_my_ws);
    }

    // resets queue
    void clear() {
        // clear owner
        {
            CRITICAL_SECTION(q_cs, _owner_lock);
            _owner = NULL;
        }
        // clear lists
        {
            CRITICAL_SECTION(cs, _lock);
            _for_writers.clear();
            _empty = true;
        }
        _for_readers.clear();
    }    
  
}; // EOF: struct srmwqueue


EXIT_NAMESPACE(dora);

#endif /** __DORA_SRMW_QUEUE_H */

