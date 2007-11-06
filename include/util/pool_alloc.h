/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __POOL_ALLOC_H
#define __POOL_ALLOC_H

struct in_progress_block;

struct alloc_pthread_specific {
    pthread_key_t _ptkey;
    alloc_pthread_specific() {
	int error = pthread_key_create(&_ptkey, NULL);
	assert(!error);
    }
    operator in_progress_block*() {
	return (in_progress_block*) pthread_getspecific(_ptkey);
    }
    in_progress_block* operator=(in_progress_block* const ptr) {
	pthread_setspecific(_ptkey, ptr);
	return ptr;
    }
};

/* A memory pool allocator. It can handle any size, though small data structures are best.
  
 */
class pool_alloc {
    alloc_pthread_specific _in_progress;
public:
    void* alloc(int size);
    void free(void* ptr);
};

/* this version assumes it always allocates the same size (and asserts
   otherwise). This is much more efficient, but also more limited.
 */
class pool_alloc_fixed {
};

#endif
