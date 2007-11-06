/* -*- mode:C++; c-basic-offset:4 -*- */
#include "util/pool_alloc.h"

#ifdef __SUNPRO_CC
#include <atomic.h>
#endif

#include <pthread.h>

typedef unsigned long long bitmap;

/* how many bytes to dole out at a time? The bigger the granularity
   the more data one block can track, but the more fragmentation we
   suffer...

   MUST be a power of 2!
 */
static int const ALLOC_GRANULARITY = 8;
static int const ALLOC_BLOCK = 64*ALLOC_GRANULARITY;


namespace {
    class header;
    
    struct block {
	bitmap _bitmap; // 1 bits mark in-use slots
	char _data[ALLOC_BLOCK];
	
	// Whoever creates me owns all the data, and can dole it out as they choose
	block() : _bitmap(~0ull) { }
	static void free(void* ptr);
	void free(header* h);
	static bitmap compute_range(int offset, int size);
    };

    struct header {
	block* _block;
	bitmap _range; // 0 bits mark me
    };
}

struct in_progress_block {	
    block* _block;
    int _offset; // in bytes
    in_progress_block() : _block(NULL), _offset(ALLOC_BLOCK) { }
    in_progress_block(block* b) : _block(b), _offset(0) { }
    header* allocate(int size);
};


namespace {
    void block::free(void* ptr) {
	unit* u = (unit*) ptr;
	free(u);
    }

    void block::free(header* h) {
	block* b = h->_block;

	// special case: oversized block (single object) allocated with malloc() 
	if(!h->_range) {
	    free(this);
	    return;
	}

	// normal case: regular block (multiple objects) allocated with operator new()
	bitmap result;
#ifdef __SUNPRO_CC
	result = atomic_and_64_nv(&b->_bitmap, h->_range);
#else
#error Sorry, only sparc supported so far
#endif

	// if everybody has turned in their bits, we will never be touched again
	if(!result)
	    delete this;
    }
    bitmap block::compute_range(int offset, int size) {
	int gsize = (size+ALLOC_GRANULARITY-1)/ALLOC_GRANULARITY;
	int goffset = (offset+ALLOC_GRANULARITY-1)/ALLOC_GRANULARITY;

	/**
	 * | . . . . x x x . . . |
	 */
	// start with a right-justified range of the correct size
	bitmap result = (1ull << gsize) - 1;

	// now shift it left by the correct offset
	result <<= goffset;

	// finally, complement it so 0 bits mark the range
	return ~result;
    }

    
}

header* in_progress_block::allocate(int size) {
    size += sizeof(header);
    if(size + _offset > ALLOC_BLOCK)
	return NULL;
    
    header* h = (header*) _block->_data + _offset;
    h->_block = _block;
    h->_range = block::compute_range(_offset, size);
    _offset += (size+ALLOC_GRANULARITY-1) & -ALLOC_GRANULARITY;
    return h;
}

void* pool_alloc::alloc(int size) {
    in_progress_block b = _in_progress;

    // first time?
    if(!b)
	b = _in_progress = new in_progress_block;

    // does the current block have space?
    header* h = b->allocate(size);
    if(!h) {
	in_progress_block ipb;
	if(size > ALLOC_BLOCK/2) {
	    /* We can't just use regular malloc because the
	       deallocator would die. Instead, fake it out:

	       1. Allocate a "block" that has enough extra space to fit the request
	       2. Set up a fake in_progress_block to allocate from
	       3. Set the "size" to the whole block
	     */
	    void* data = malloc(sizeof(bitmap) + sizeof(header) + size);
	    ipb = in_progress_block(new(data) block);
	    b = &ipb
	    size = ALLOC_BLOCK - sizeof(header); // claim the whole block
	}
	else {
	    *b = in_progress_block(new block);
	}
	
	// better work this time!
	h = b->allocate(size);
	assert(h);
    }
	
    return h;
}
