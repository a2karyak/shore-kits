/* -*- mode:C++; c-basic-offset:4 -*- */
#include "util/pool_alloc.h"

#ifdef __SUNPRO_CC
#include <atomic.h>
#endif

#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#else
#include <cstdlib>
#include <cstdio>
#endif
#include <new>

#ifdef DEBUG_ALLOC
#include <cstdio>
using namespace std;
#endif

// always shift left, 0 indicates full block
typedef unsigned long bitmap;

/* This allocator uses malloc to acquire "blocks" of memory, which are
   broken up into "units" which can be handed out one (or many) at a
   time.

   Each block contains 64 units, and maintains a bitmap which marks used
   and unused units. During normal operation, the allocator
   automatically adjusts its unit size to match the largest request it
   has ever seen; this makes it most useful for allocating objects of
   the same or similar sizes.

   When the application requests memory from the allocator, the latter
   carves out enough units to fit the request and marks them as
   used. It always proceeds in sequence, never checking to see whether
   any previous units have freed up. Once a block fills up, the
   allocator purposefully "leaks" it (see deallocation, below) and
   malloc()s another. 

   Each allocation (containing one or more units) contains enough
   information to notify its owning block when the application frees
   it; the block deletes itself when it gets all its units
   back.

   Allocation and deallocation are thread-safe, even when they occurs
   in different threads at the same time or allocated memory moves
   between threads.
   
 */
// 
static int const BLOCK_UNITS = 64; // one for each bit in a 'bitmap'

namespace {
    class header;
    
    struct block {
	bitmap _bitmap; // 1 bits mark in-use slots
#ifdef TRACE_LEAKS
	char const* _name;
#endif
	char _data[0]; // place-holder
	
	// Whoever creates me owns all the data, and can dole it out as they choose
	block(char const* name)
	    : _bitmap(~0ull)
	{
#ifdef TRACE_LEAKS
	    _name = name;
#endif
	}
	header* get_header(int index);
	bitmap update_map(bitmap _range) {
	    bitmap result;
#ifdef __SUNPRO_CC
	    result = atomic_and_64_nv(&_block->_bitmap, _range);
#else
#warning "Free is not thread-safe on this architecture"
	    result = _block->_bitmap & _range;
	    _block->_bitmap = result;
#endif
	    return result;
	}
    };

    struct header {
	bitmap _range; // 0 bits mark me
	block* _block;
	char _data[0]; // place-holder for the actual data
	void release();
    };
}

struct in_progress_block {
    char const* _name;
    block* _block;
    bitmap _next_unit; // a single bit that marches right to left; 0 indicates the block is full
    int _delta; // how much do I increment the offset for each allocation from the current block?
    int _offset; // current block offset in bytes
    int _new_delta; // how much should I have been incrementing? (the max of all requested sizes)
    in_progress_block(char const* name, int delta)
	: _name(name), _block(NULL), _next_unit(0), _delta(delta), _offset(0), _new_delta(delta)
    {
    }
    void new_block() {
	// if we saw something bigger than we planned on last time around, update the delta.
	_next_unit = 1;
	_delta = _new_delta;
	int bytes = sizeof(block) + BLOCK_UNITS*_delta;
	void* data = ::malloc(bytes);
	_block = new(data) block(_name);
	_offset = 0;
#ifdef TRACE_LEAKS
	fprintf(stderr, "%s block 1 %p+%d\n", _name, _block, BLOCK_UNITS*_delta);
#endif
    }
    
    header* allocate(int size);
    void grab_unit(bitmap &range, int &size);
    header* init_header(int offset, bitmap range);    
};


namespace {
    header* block::get_header(int offset) {
	return (header*) &_data[offset];
    }
    
    void header::release() {

	// special case: oversized block (single object) allocated with malloc() 
	if(!_range) {
#ifdef TRACE_LEAKS
	    fprintf(stderr, "%s unit -1 %p %016llx (oversized)\n", _block->_name, _block, _range);
#endif
	    ::free(_block);
	    return;
	}

	// normal case: regular block (multiple objects) allocated with operator new()
#ifdef TRACE_LEAKS
	fprintf(stderr, "%s unit -1 %p %016llx\n", _block->_name, _block, _range);
#endif

	// if everybody has turned in their bits, we will never be touched again
	if(!update_map(_range)) {
#ifdef TRACE_LEAKS
	    fprintf(stderr, "%s block -1 %p\n", _block->_name, _block);
#endif
	    ::free(_block);
	}
    }
    
    
}

header* in_progress_block::init_header(int offset, bitmap range) {
    header* h = _block->get_header(offset);
    h->_range = ~range;
    h->_block = _block;
    return h;
}

void in_progress_block::grab_unit(bitmap &range, int &size) {
    range |= _next_unit;
    _next_unit <<= 1;
    size -= _delta;
    _offset += _delta;
}
header* in_progress_block::allocate(int size) {
    bitmap range = 0;
    if(_next_unit == 0) {
	new_block();
	if(!_block) {
	    _next_unit = 0;
	    return NULL;
	}
    }

    // save the old offset for use by the header
    int offset = _offset;

    // fast track version for well-behaved sizes
    if(size <= _delta) {
	grab_unit(range, size);
    }
    
    // slow track version handles odd sizes
    else {
	if(_new_delta < size)
	    _new_delta = size;
	
	while(_next_unit != 0 && size > 0) {
	    grab_unit(range, size);
	}
	// out of space?
	if(size > 0) {
	    //  "Free" the rest of the block so it doesn't leak, and move on.
	    init_header(offset, range)->release();
	    return NULL;
	}

    }

    // fill out the header and return it
    header* h = init_header(offset, range);
    return h;
}

void* pool_alloc::alloc(int size) {
    // factor in the header and align it to the nearest 8 bytes
    size = (size + sizeof(header) + 7) & -8;
    in_progress_block* ipb = _in_progress;

    // first time?
    if(!ipb) {
	ipb = _in_progress = new in_progress_block(_name, size);
    }

    // does the current block have space?
    header* h = ipb->allocate(size);
    if(h) {
    }
    
    // too big? 
    else /*if(size > BLOCK_UNITS/2*b->_delta) */{
	/* We can't just use regular malloc because the
	   deallocator would die. Instead, fake it out:

	   1. Allocate a "block" that has enough extra space to fit the request
	   2. Set up a fake in_progress_block to allocate from
	   3. Set the "size" to the whole block
	*/
	void* data = ::malloc(sizeof(block) + sizeof(header) + size);
	if(!data)
	    return NULL;
	block* b = new(data) block(_name);
	h = b->get_header(0);
	h->_block = b;
	h->_range = 0; // this marks us as a special block
#ifdef TRACE_LEAKS
	fprintf(stderr, "%s block 1 %p+%d (oversized)\n", _name, b, BLOCK_UNITS*ipb->_delta);
#endif
	
    }
#if 0
    // time for the next block
    else {
	// unreachable?
	assert(false);
	// better work this time!
	ipb->new_block();
	h = ipb->allocate(size);
	assert(h);
#ifdef DEBUG_ALLOC
	fprintf(stderr, "Allocated %p from new block %p with range %016llx\n", h, h->_block, h->_range);
#endif
    }
#endif
#ifdef TRACE_LEAKS
	fprintf(stderr, "%s unit 1 %p %016llx\n", _name, h->_block, h->_range);
#endif
    return h->_data;
}

void pool_alloc::free(void* ptr) {
    // we have to back up to retrieve the header info (watch out for aliasing)
    union {
	void* vptr;
	char* cptr;
    } u1 = {ptr};
    union {
	char* cptr;
	header* hptr;
    } u2 = {u1.cptr - sizeof(header)};

    u2.hptr->release();
}

extern "C" {
    static void destruct_in_progress(void* arg) {
	in_progress_block* b = (in_progress_block*) arg;
	if(b && b->_block && b->_next_unit) {
	    /* force an overflow so we don't leak the rest of the
	       block (guaranteed to fail because there is always at
	       least one unit already allocated in the block)
	     */
	    b->allocate(BLOCK_UNITS*b->_delta);
	}
	    
    }
}
alloc_pthread_specific::alloc_pthread_specific() {
    int error = pthread_key_create(&_ptkey, &destruct_in_progress);
    assert(!error);
}

#if 0
#include <vector>
#include <algorithm>
#include "stopwatch.H"

struct malloc_alloc {
    void* alloc(int size) { return ::malloc(size); }
    void free(void* ptr) { ::free(ptr); }
};

malloc_alloc ma;
pool_alloc pa;

// just try to allocate and free a bunch in a row...
void test1() {
    std::vector<void*> pointers;

    // find out how much the shuffles cost
    pointers.resize(1000);
    stopwatch_t timer;
    for(int i=0; i < 1000; i++) {
	std::random_shuffle(pointers.begin(), pointers.end());
    }
    double shuffle_time = timer.time_ms();
    for(int j=0; j < 1000;  j++) {
	pointers.clear();
	for(int i=0; i < 1000; i++)
	    pointers.push_back(pa.alloc(10));

	std::random_shuffle(pointers.begin(), pointers.end());
	for(unsigned i=0; i < pointers.size(); i++)
	    pa.free(pointers[i]);
    }
    double t = timer.time_ms();
    fprintf(stderr, "Cycled 10 bytes 1M times in %.3f ms (%.3f w/o shuffle)\n", t, t-shuffle_time);
}

int main() {
    test1();

    return 0;
}
#endif
