/* -*- mode:C++; c-basic-offset:4 -*- */
#include "util/pool_alloc.h"

#ifdef __SUNPRO_CC
#include <atomic.h>
#endif

#ifdef __SUNPRO_CC
#include <stdlib.h>
#else
#include <cstdlib>
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
	char _data[0]; // place-holder
	
	// Whoever creates me owns all the data, and can dole it out as they choose
	block()
	    : _bitmap(~0ull)
	{
	}
	header* get_header(int index);
    };

    struct header {
	bitmap _range; // 0 bits mark me
	block* _block;
	char _data[0]; // place-holder for the actual data
	void release();
    };
}

struct in_progress_block {	
    block* _block;
    bitmap _next_unit; // a single bit that marches right to left; 0 indicates the block is full
    int _delta; // how much do I increment the offset for each allocation from the current block?
    int _offset; // current block offset in bytes
    int _new_delta; // how much should I have been incrementing? (the max of all requested sizes)
    in_progress_block(int delta)
	: _block(NULL), _next_unit(0), _delta(delta), _offset(0), _new_delta(delta)
    {
    }
    void new_block() {
	// if we saw something bigger than we planned on last time around, update the delta.
#ifdef DEBUG_ALLOC
	fprintf(stderr, "Allocating a new block with %d-byte units\n", _new_delta);
#endif
	_next_unit = 1;
	_delta = _new_delta;
	int bytes = sizeof(block) + BLOCK_UNITS*_delta;
	void* data = ::malloc(bytes);
	_block = new(data) block;
	_offset = 0;
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
#ifdef DEBUG_ALLOC
	    fprintf(stderr, "Freeing oversized block %p\n", _block);
#endif
	    ::free(_block);
	    return;
	}

	// normal case: regular block (multiple objects) allocated with operator new()
	bitmap result;
#ifdef __SUNPRO_CC
	result = atomic_and_64_nv(&_block->_bitmap, _range);
#else
	//#error Sorry, only sparc supported so far
	result = _block->_bitmap & _range;
	_block->_bitmap = result;
#endif

#ifdef DEBUG_ALLOC
	fprintf(stderr, "Freeing allocation %p from block %p. Range: 0x%016llx, new block map: 0x%016llx\n",
		this, _block, _range, _block->_bitmap);
#endif
	// if everybody has turned in their bits, we will never be touched again
	if(!result) {
#ifdef DEBUG_ALLOC
	    fprintf(stderr, "Freeing block %p\n", _block);
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
    
#ifdef DEBUG_ALLOC
    int bytes = 0;
#endif
    
    // fast track version for well-behaved sizes
    if(size <= _delta) {
	grab_unit(range, size);
#ifdef DEBUG_ALLOC
	    bytes += _delta;
#endif
    }
    
    // slow track version handles odd sizes
    else {
	if(_new_delta < size)
	    _new_delta = size;
	
	while(_next_unit != 0 && size > 0) {
	    grab_unit(range, size);
#ifdef DEBUG_ALLOC
	    bytes += _delta;
#endif
	}
	// out of space?
	if(size > 0) {
#ifdef DEBUG_ALLOC
	    fprintf(stderr, "Block %p can't fit %d bytes. Freeing leftovers (range %016llx)\n",
		    _block, size, range);
#endif
	    //  "Free" the rest of the block so it doesn't leak, and move on.
	    init_header(offset, range)->release();
	    return NULL;
	}

    }

    // fill out the header and return it
    header* h = init_header(offset, range);
#ifdef DEBUG_ALLOC
    fprintf(stderr, "Allocated %d bytes from block %p, starting at %p with range %016llx\n",
	    bytes, _block, h, h->_range);
#endif
    return h;
}

void* pool_alloc::alloc(int size) {
    // factor in the header and align it to the nearest 8 bytes
    size = (size + sizeof(header) + 7) & -8;
    in_progress_block* b = _in_progress;

    // first time?
    if(!b) {
#ifdef DEBUG_ALLOC
	fprintf(stderr, "Creating new pre-thread allocator state\n");
#endif
	b = _in_progress = new in_progress_block(size);
    }

    // does the current block have space?
    header* h = b->allocate(size);
    if(h) {
#ifdef DEBUG_ALLOC
	fprintf(stderr, "Allocated %p from block %p with range %016llx\n", h, h->_block, h->_range);
#endif
    }
    
    // too big? 
    else /*if(size > BLOCK_UNITS/2*b->_delta) */{
#ifdef DEBUG_ALLOC
	fprintf(stderr, "Oversized allocation detected. Reverting to malloc\n");
#endif
	/* We can't just use regular malloc because the
	   deallocator would die. Instead, fake it out:

	   1. Allocate a "block" that has enough extra space to fit the request
	   2. Set up a fake in_progress_block to allocate from
	   3. Set the "size" to the whole block
	*/
	void* data = ::malloc(sizeof(block) + sizeof(header) + size);
	if(!data)
	    return NULL;
	block* b = new(data) block;
	h = b->get_header(0);
	h->_block = b;
	h->_range = 0; // this marks us as a special block
    }
#if 0
    // time for the next block
    else {
	// unreachable?
	assert(false);
	// better work this time!
	b->new_block();
	h = b->allocate(size);
	assert(h);
#ifdef DEBUG_ALLOC
	fprintf(stderr, "Allocated %p from new block %p with range %016llx\n", h, h->_block, h->_range);
#endif
    }
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
	    // force an overflow so we don't leak the rest of the block
	    b->allocate(BLOCK_UNITS*b->_delta);
	}
	    
    }
}
alloc_palloc_pthread_specific::alloc_pthread_specific() {
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
