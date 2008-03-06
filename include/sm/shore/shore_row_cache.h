/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_row_cache.h
 *
 *  @brief:  Cache for tuples (row_impl<>) used in Shore
 *
 *  @note:   row_cache_t           - class for tuple caching
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_ROW_CACHE_H
#define __SHORE_ROW_CACHE_H

// sparcv9 only at the moment
#include <atomic.h>

#include "util/guard.h"

#include "shore_row.h"


// define cache stats to get some statistics about the tuple cache
#undef CACHE_STATS
//#define CACHE_STATS


ENTER_NAMESPACE(shore);


template <class TableDesc>
struct tuple_node_impl
{ 
    typedef row_impl<TableDesc> table_tuple;

    table_tuple* _tuple;
    tuple_node_impl* _next; 

    tuple_node_impl(TableDesc* aptable, 
                    tuple_node_impl* next = NULL)
        : _next(next)
    {
        assert (_tuple);
        assert (aptable);
        _tuple = new table_tuple(aptable);
    }
        
    ~tuple_node_impl()
    {
        if (_tuple)
            delete (_tuple);
    }

}; // EOF: tuple_node_impl


template <class TableDesc>
class row_cache_t 
{
    typedef tuple_node_impl<TableDesc> tuple_node;
    
private:

    tuple_node* volatile _head;

    TableDesc* _ptable; 

#ifdef CACHE_STATS    
    // stats
    int _tuple_requests; 
    tatas_lock _requests_lock;
    int _tuple_setups;
    tatas_lock _setups_lock;
#endif

    /* Creates a new tuple node
     */
    tuple_node* create_node() 
    {
#ifdef CACHE_STATS
        CRITICAL_SECTION( scs, _setups_lock);
        ++_tuple_setups;
#endif

        return (new tuple_node(_ptable));
    }
                

public:

    row_cache_t(TableDesc* ptable, int init_count = 0) 
        : _head(NULL), _ptable(ptable)
#ifdef CACHE_STATS
        , _tuple_requests(0), _tuple_setups(0)
#endif
    { 
        assert (_ptable);
        assert (init_count >= 0); 
        for (int i=0; i<init_count; i++) {
#ifdef CACHE_STATS
            ++_tuple_setups;
#endif
            tuple_node* aptn = new tuple_node(_ptable);
            giveback(aptn);
        }
    }

    
    /* Destroys the cache, deleting all the objects it is hoarding.
     */
    ~row_cache_t() {
        // no lock needed -- nobody should be using me any more...
        int icount = 0;
        for (tuple_node* cur=_head; cur; ) {
            tuple_node* next = cur->_next;
            delete (cur);
            cur = next;
            ++icount;
        }

#ifdef CACHE_STATS        
        print_stats();
#endif
        TRACE( TRACE_STATISTICS, "Deleted: (%d)\n", icount);
    }


    /* For debugging
     */
    void walkthrough() {
        fprintf( stderr, "requests (%d)\n", _tuple_requests);
        fprintf( stderr, "setups   (%d)\n", _tuple_setups);
        int cnt = 0;
        for (tuple_node* cur=_head; cur; ) {
            fprintf( stderr, "walking (%d)\n", ++cnt);
            cur = cur->_next;
        }
    }

#ifdef CACHE_STATS    
    int  setup_count() { return (_tuple_setups); }
    int  request_count() { return (_tuple_requests); }
    void print_stats() {
        TRACE( TRACE_STATISTICS, "Requests: (%d)\n", _tuple_requests);
        TRACE( TRACE_STATISTICS, "Setups  : (%d)\n", _tuple_setups);
    }
#endif 

    /* Return an unused object, if cache empty allocate and return a new one
     */
    tuple_node* borrow() 
    {
	tuple_node* old_value = _head;
	while(old_value) {
	    tuple_node* cur_value = 
                (tuple_node*) atomic_cas_ptr(&_head, old_value, old_value->_next);
	    if(old_value == cur_value)
		break; // if CAS successful (old_value) has the old head

	    // try again...
	    old_value = cur_value;
	}

#ifdef CACHE_STATS
        CRITICAL_SECTION( rcs, _requests_lock);
        ++_tuple_requests;
#endif

	return (old_value? old_value : create_node());
    }
    

    /* Returns an object to the cache. The object is reset and put on the
     * free list.
     */
    void giveback(tuple_node* ptn) 
    {        
        assert (ptn);
        // reset the object
        ptn->_tuple->reset();
      
        // enqueue it
        tuple_node* old_value = _head;
        while (1) {
            ptn->_next = old_value;
            membar_producer();
            tuple_node* cur_value = (tuple_node*) 
                atomic_cas_ptr(&_head, old_value, ptn);
            if(old_value == cur_value)
                break; // if CAS successful (ptn) is the new head

            // try again...
            old_value = cur_value;
        }
    }    

}; // EOF: row_cache_t


EXIT_NAMESPACE(shore);


#endif /* __SHORE_ROW_CACHE_H */
