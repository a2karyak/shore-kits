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
    
    // stats
    // (ip) to be removed
    int _tuple_requests; 
    tatas_lock _requests_lock;
    int _tuple_setups;
    tatas_lock _setups_lock;

    /* Creates a new tuple node
     */
    tuple_node* create_node() 
    {
        CRITICAL_SECTION( scs, _setups_lock);
        ++_tuple_setups;
        return (new tuple_node(_ptable));
    }
                

public:

    row_cache_t(TableDesc* ptable, int init_count = 0) 
        : _head(NULL), _ptable(ptable), _tuple_requests(0), _tuple_setups(0)
    { 
        assert (_ptable);
        assert (init_count >= 0); 
        for (int i=0; i<init_count; i++) {
            ++_tuple_setups;
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
        
        print_stats();
        TRACE( TRACE_STATISTICS, "Deleted: (%d)\n", icount);
    }


    /* For debugging
     */
    void walkthrough() {
        fprintf( stderr, "requests (%d)\n", _tuple_requests);
        fprintf( stderr, "setups   (%d)\n", _tuple_setups);
        for (tuple_node* cur=_head; cur; ) {
            cur->_tuple->print_tuple_no_tracing();
            cur = cur->_next;
        }
    }
    
    int  setup_count() { return (_tuple_setups); }
    int  request_count() { return (_tuple_requests); }
    void print_stats() {
        TRACE( TRACE_STATISTICS, "Requests: (%d)\n", _tuple_requests);
        TRACE( TRACE_STATISTICS, "Setups  : (%d)\n", _tuple_setups);
    }
 

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

        CRITICAL_SECTION( rcs, _requests_lock);
        ++_tuple_requests;

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
