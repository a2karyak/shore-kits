/* -*- mode:C++; c-basic-offset:4 -*- */
#include "util/pool_alloc.h"
#include "util/sync.h"

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

// We only have atomic ops on some arch...
#ifdef __SUNPRO_CC
#define USE_ATOMIC_OPS
#else
// Unsupported arch must use locks
#undef USE_ATOMIC_OPS
#endif
#undef TRACK_LEAKS


#undef TRACE_LEAKS
#undef TRACK_BLOCKS



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

#ifdef TRACK_BLOCKS
#include <map>
#include <vector>
#include <utility>
#include <algorithm>
namespace {struct block; }
static pthread_mutex_t block_mutex = PTHREAD_MUTEX_INITIALIZER;
struct block_entry {
    block* _block;
    char const* _name;
    pthread_t _tid;
    int _delta;
    block_entry(block* b, char const* name, pthread_t tid, int delta)
	: _block(b), _name(name), _tid(tid), _delta(delta) { }
    bool operator<(block_entry const &other) const {
	if(_name < other._name)
	    return true;
	if(_name > other._name)
	    return false;
	if(_tid < other._tid)
	    return true;
	if(_tid > other._tid)
	    return false;
	return _block < other._block;
    }
};
typedef std::map<block*, int> block_map; // block, delta
typedef std::pair<block_map, pthread_mutex_t> block_map_sync;
typedef std::pair<char const*, pthread_t> thread_alloc_key;
typedef std::pair<block_map_sync*, bool> thread_alloc_value; // map, thread live?
typedef std::map<thread_alloc_key, thread_alloc_value> thread_maps;
typedef std::vector<block_entry> block_list;
static thread_maps* live_blocks;
#endif

namespace {
    class header;

    // WARNING: sizeof(block) must be a multiple of 8 bytes
    struct block {
#ifndef USE_ATOMIC_OPS
	pthread_mutex_t _lock;
#endif
	bitmap _bitmap; // 1 bits mark in-use slots
#if defined(TRACE_LEAKS)
	const char* _name;
#endif
#if defined(TRACK_BLOCKS)
	block_map_sync* _live_blocks;
#endif
	char _data[0]; // place-holder
	
	// Whoever creates me owns all the data, and can dole it out as they choose
	block(char const* name)
	    : _bitmap(~0ull)
	{
#ifndef USE_ATOMIC_OPS
	    _lock = thread_mutex_create();
#endif
#ifdef TRACE_LEAKS
	    _name = name;
#endif
	}
	header* get_header(int index, bitmap range);
	void cut_loose();
    };

    // WARNING: sizeof(header) must be a multiple of 8 bytes
    struct header {
	bitmap _range; // 0 bits mark me
	block* _block;
	char _data[0]; // place-holder for the actual data
	void release();
    };
}

#ifdef TRACK_BLOCKS
void print_live_blocks() {
    // not locked because we only use it from the debugger anyway
    block_list blist;

    // (thread_alloc_key, thread_alloc_value) pairs
    thread_maps::iterator map = live_blocks->begin();
    while(map != live_blocks->end()) {
	thread_alloc_key const &key = map->first;
	thread_alloc_value &value = map->second;
	block_map* bmap = &value.first->first;

	// (block*, delta) pairs
	block_map::iterator it = bmap->begin();
	for( ; it != bmap->end(); ++it) 
	    blist.push_back(block_entry(it->first, key.first, key.second, it->second));
	
	// is the thread dead and all blocks deallocated?
	if(!value.second && bmap->empty()) 
	    live_blocks->erase(map++);
	else
	    ++map;
    }

    std::sort(blist.begin(), blist.end());
    block_list::iterator it=blist.begin();
    printf("Allocator  (tid): " "address           " " unit bitmap\n");
    for(; it != blist.end(); ++it) {
	printf("%10s (%3d): 0x%016p %4d ", it->_name, it->_tid, it->_block, it->_delta);
	bitmap units = it->_block->_bitmap;
	for(int i=0; i < BLOCK_UNITS; i++) {
	    putchar('0' + (units&0x1));
	    units >>= 1;
	}
	putchar('\n');    
    }
}
#endif
struct in_progress_block {
    char const* _name;
    block* _block;
    bitmap _next_unit; // a single bit that marches right to left; 0 indicates the block is full
    int _delta; // how much do I increment the offset for each allocation from the current block?
    int _offset; // current block offset in bytes
    int _alloc_count;
    int _huge_count;
    int _alloc_sizes[BLOCK_UNITS];
    int _huge_sizes[BLOCK_UNITS];
#ifdef TRACK_BLOCKS
    block_map_sync* _live_blocks;
#endif
    in_progress_block(char const* name, int delta)
	: _name(name), _block(NULL), _next_unit(0), _delta(delta), _offset(0),
	  _alloc_count(0), _huge_count(0)
    {
#ifdef TRACK_BLOCKS
	// register our private block map
	_live_blocks = new block_map_sync;
	_live_blocks->second = thread_mutex_create();
	critical_section_t cs(block_mutex);
	(*live_blocks)[std::make_pair(_name, pthread_self())] = std::make_pair(_live_blocks, true);
#endif
    }
    ~in_progress_block() {
	discard_block();
#ifdef TRACK_BLOCKS
	// mark the block as dead
	critical_section_t cs(block_mutex);
	(*live_blocks)[std::make_pair(_name, pthread_self())].second = false;
#endif
    }
    void new_block();
    header* allocate(int size);
    header* allocate_normal(int size);
    header* allocate_huge(int size);
    void discard_block();
};


namespace {
    header* block::get_header(int offset, bitmap range) {
	header* h = (header*) &_data[offset];
	h->_block = this;
	h->_range = range;
	return h;
    }

    void header::release() {

#ifdef TRACE_LEAKS
	fprintf(stderr, "%s unit -1 %p %016llx\n", _block->_name, _block, _range);
#endif

	bitmap result;
	bitmap clear_range = ~_range;
#ifdef USE_ATOMIC_OPS
#ifdef __SUNPRO_CC
	membar_enter();
	result = atomic_and_64_nv(&_block->_bitmap, clear_range);
	membar_exit();
#else
#error "Sorry, atomic ops not supported on this architecture"
#endif
#else
	{
	    critical_section_t cs(_block->_lock);
	    result = _block->_bitmap & clear_range;
	    _block->_bitmap = result;
	}
#endif
	
	// have all the units come back?
	if(!result) {
#ifdef TRACE_LEAKS
	    if(!clear_range)
		fprintf(stderr, "%s block -1 %p %016llx (oversized)\n", _block->_name, _block, _range);
	    else
		fprintf(stderr, "%s block -1 %p\n", _block->_name, _block);
#endif
#ifdef TRACK_BLOCKS
	    {
		critical_section_t cs(_block->_live_blocks->second);
		_block->_live_blocks->first.erase(_block);
	    }
#endif
	    // note: frees the memory 'this' occupies
	    ::free(_block);
	}
    }
    
    
}

static int const ADJUST_GOAL = 5; // 1/x
static int const K_MAX = (BLOCK_UNITS+ADJUST_GOAL-1)/ADJUST_GOAL;

static int kth_biggest(int* array, int array_size) {
    int top_k[K_MAX];
    int k = (array_size+ADJUST_GOAL-1)/ADJUST_GOAL;
    int last=0; // last entry of the top-k list
    top_k[0] = array[0];
    for(int i=1; i < array_size; i++) {
	int size = array[i];

	// find the insertion point
	int index=0;
	while(index <= last && size < top_k[index]) ++index;
	if(index == k)
	    continue; // too small...
	
	// bump everything else over to make room...
	for(int j=k; --j > index; )
	    top_k[j] = top_k[j-1];
	
	// ...and insert
	top_k[index] = size;
	if(index > last)
	    last = index;
    }

    return top_k[last];
}

void in_progress_block::discard_block() {
    // allocate whatever is left of this block and throw it away
    if(_next_unit) {
	_block->get_header(_offset, ~(_next_unit-1))->release();
	_next_unit = 0;
    }
}
    
void in_progress_block::new_block() {
    // adjust the unit size? We want no more than 1/x requests to allocate 2+ units
    

    _huge_count = 0;
    _alloc_count = 0;
    _next_unit = 1;
    int bytes = sizeof(block) + BLOCK_UNITS*_delta;
    void* data = ::malloc(bytes);
    if(!data)
	throw std::bad_alloc();
    
    _block = new(data) block(_name);
    _offset = 0;
#ifdef TRACK_BLOCKS
    {
	_block->_live_blocks = _live_blocks;
	critical_section_t cs(_live_blocks->second);
	_live_blocks->first[_block] = _delta;
    }
#endif
}

header* in_progress_block::allocate_huge(int size) {
    _huge_sizes[_huge_count++] = size;
    // enough "huge" requests to destroy our ADJUST_GOAL?
    if(_huge_count > K_MAX) {
	discard_block();
	_delta = kth_biggest(_huge_sizes, _huge_count);
	return allocate_normal(size);
    }

    /* Create a block that's just the size we want, and allocate
       the whole thing to this request. 
    */
    void* data = ::malloc(sizeof(block) + size);
    if(!data)
	throw std::bad_alloc();
	
    block* b = new(data) block(_name);
#ifdef TRACK_BLOCKS
    {
	b->_live_blocks = _live_blocks;
	critical_section_t cs(_live_blocks->second);
	_live_blocks->first[b] = size;
    }
#endif
#ifdef TRACE_LEAKS
    fprintf(stderr, "%s block 1 %p+%d (oversized)\n", _name, b, size);
#endif
    return b->get_header(0, ~0ull);
}

header* in_progress_block::allocate_normal(int size) {

    // is the previous block full?
    if(_next_unit == 0) {
	if(_alloc_count)
	    _delta = kth_biggest(_alloc_sizes, _alloc_count);
	new_block();
#ifdef TRACE_LEAKS
	fprintf(stderr, "%s block 1 %p+%d\n", _name, _block, _delta);
#endif
    }

    // update stats
    _alloc_sizes[_alloc_count++] = size;
    
    // initialize a header 
    header* h = _block->get_header(_offset, 0);
    
    // carve out the units this request needs
    int remaining = size;
    do {
	h->_range |= _next_unit;
	_next_unit <<= 1;
	remaining -= _delta;
	_offset += _delta;
    } while(remaining > 0 && _next_unit);

    // out of space?
    if(remaining > 0) {
	//  free this undersized allocation and try again with a new block
	h->release();
	return allocate_normal(size);
    }

    // success!
    return h;
}

header* in_progress_block::allocate(int size) {
    // how many units is "huge" ?
    static int const HUGE_THRESHOLD = BLOCK_UNITS/8;

    // huge?
    return (size > HUGE_THRESHOLD*_delta)? allocate_huge(size) : allocate_normal(size);
}

void* pool_alloc::alloc(int size) {
    // factor in the header size and round up to the next 8 byte boundary
    size = (size + sizeof(header) + 7) & -8;

    // first time?
    in_progress_block* ipb = _in_progress;
    if(!ipb) {
#ifdef TRACK_BLOCKS
	if(!live_blocks)
	    live_blocks = new thread_maps;
#endif
	ipb = _in_progress = new in_progress_block(_name, size);
    }

    header* h = ipb->allocate(size);
#ifdef TRACE_LEAKS
    fprintf(stderr, "%s unit 1 %p %016llx\n", _name, h->_block, h->_range);
#endif
    return h->_data;
}

void pool_alloc::free(void* ptr) {
    /* C++ "strict aliasing" rules allow the compiler to assume that
       void* and header* never point to the same memory.  If
       optimizations actually take advantage of this, simple casts
       would break.

       NOTE 1: char* automatically aliases everything, but I'm not sure
       if that's enough to make void* -> char* -> header* safe.

       NOTE 2: in practice I've only seen strict aliasing cause
       problems if (a) the memory is actually *accessed* through
       pointers of different types and (b) those pointers both reside
       in the same function after inlining has taken place. But, just
       to be safe...

       Unions are gcc's official way of telling the compiler that two
       incompatible pointers do, in fact, overlap. I suspect it will
       work for CC as well, since a union is rather explicit about
       two things being in the same place.
     */
    union {
	void* vptr;
	char* cptr;
    } u1 = {ptr};
    union {
	char* cptr;
	header* hptr;
	// ptr actually points to header::_data, so back up a bit..
    } u2 = {u1.cptr - sizeof(header)};

    u2.hptr->release();
}

extern "C" {
    static void destruct_in_progress(void* arg) {
	in_progress_block* b = (in_progress_block*) arg;
	delete b;
    }
}
alloc_pthread_specific::alloc_pthread_specific() {
    int error = pthread_key_create(&_ptkey, &destruct_in_progress);
    assert(!error);
}
