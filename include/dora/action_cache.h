/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   action_cache.h
 *
 *  @brief:  Cache for range actions (range_action_impl<>) used in DORA
 *
 *  @note:   action_cache_t           - class for action caching
 *
 *  @author: Ippokratis Pandis, Oct 2008
 *
 */

#ifndef __DORA_ACTION_CACHE_H
#define __DORA_ACTION_CACHE_H


// sparcv9 only at the moment
#include <atomic.h>

#include "util/guard.h"
#include "dora/range_action.h"

// define cache stats to get some statistics about the tuple cache
#undef CACHE_STATS
//#define CACHE_STATS

ENTER_NAMESPACE(dora);


template <typename Action>
class action_cache_t : protected atomic_stack
{
#ifdef CACHE_STATS    
private:
    // stats
    int _action_requests; 
    tatas_lock _requests_lock;
    int _action_setups;
    tatas_lock _setups_lock;
#endif              

public:

    action_cache_t(int init_count) 
        : atomic_stack(-sizeof(ptr))
#ifdef CACHE_STATS
        , _action_requests(0), _action_setups(0)
#endif
    { 
        assert (init_count); 

        // start with a non-empty pool, so that threads don't race
        // at the beginning
        ptr* head = NULL;
        for (int i=0; i<init_count; i++) {                       
            vpn u = {borrow()};
            u.p->next = head;
            head = u.p;
        }

        for (int i=0; i<init_count; i++) {
            pvn p;
            p.p = head;
            head = head->next;
            giveback((Action*)p.v);
        }
    }

    
    /* Destroys the cache, calling the destructor for all the objects 
     * it is hoarding.
     */
    ~action_cache_t() 
    {
        vpn val;
        void* v = NULL;
        int icount = 0;
        while ( (v=pop()) ) {
            val.v = v;
            val.n += +_offset; // back up to the real
            free(val.v);
            ++icount;
        }
        TRACE( TRACE_TRX_FLOW, "Deleted: (%d)\n", icount);
    }

    /* Return an unused object, if cache empty allocate and return a new one
     */
    Action* borrow() 
    {
#ifdef CACHE_STATS
        CRITICAL_SECTION( rcs, _requests_lock);
        ++_action_requests;
#endif

        void* val = pop();
        if (val) return ((Action*)(val));

#ifdef CACHE_STATS
        CRITICAL_SECTION( scs, _setups_lock);
        ++_action_setups;
#endif

        // allocates and setups a new action
        vpn u = { malloc(sizeof(Action)) };
        if (!u.v) u.v = null();
        Action* rp = (Action*)prepare(u);
        //rp->setup(_ptable);
	return (rp);
    }
    
    /* Returns an object to the cache. The object is reset and put on the
     * free list.
     */
    void giveback(Action* ptn) 
    {        
        assert (ptn);

        // reset the object
        ptn->reset();
        
        // avoid pointer aliasing problems with the optimizer
        union { Action* t; void* v; } u = {ptn};
        
        // give it back
        push(u.v);
    }    

#ifdef CACHE_STATS    
    int  setup_count() { return (_action_setups); }
    int  request_count() { return (_action_requests); }
    void print_stats() {
        TRACE( TRACE_STATISTICS, "Requests: (%d)\n", _action_requests);
        TRACE( TRACE_STATISTICS, "Setups  : (%d)\n", _action_setups);
    }
#endif 

}; // EOF: action_cache_t


EXIT_NAMESPACE(dora);


#endif /* __DORA_ACTION_CACHE_H */
