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

template<class action_t>
struct srmwqueue {
    typedef std::vector<action_t*> action_list;
    action_list _for_writers;
    action_list _for_readers;
    action_list::iterator _read_pos;
    mcs_lock _lock;
    int volatile _empty;

    srmwqueue() : _empty(true) { }
    
    action_t* pop() {
	if ((_read_pos == _for_readers.end()) && (!wait_for_input()))
	    return (NULL);

	return (*(_read_pos++));
    }

    void push(action_t* a) {
        assert (a);
	CRITICAL_SECTION(cs, _lock);
	_for_writers.push_back(a);
	_empty = false;
    }
  
    bool wait_for_input(WorkerControl volatile* wc) {
        assert (a);
	while(*&_empty) {
	    if(*wc != WC_ACTIVE)
		return false;
	}
    
	{
	    CRITICAL_SECTION(cs, _lock);
	    _for_readers.clear();
	    _for_writers.swap(_for_readers);
	    _empty = true;
	}
    
	_read_pos = _for_readers.begin();
	return true;
    }
  
}; // EOF: struct srmwqueue


EXIT_NAMESPACE(dora);

#endif /** __DORA_SRMW_QUEUE_H */

