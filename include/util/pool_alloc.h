/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __POOL_ALLOC_H
#define __POOL_ALLOC_H

#include <pthread.h>
#include <cassert>

struct in_progress_block;

struct alloc_pthread_specific {
    pthread_key_t _ptkey;
    alloc_pthread_specific();
    operator in_progress_block*() {
	return (in_progress_block*) pthread_getspecific(_ptkey);
    }
    in_progress_block* operator=(in_progress_block* const ptr) {
	pthread_setspecific(_ptkey, ptr);
	return ptr;
    }
};

class pool_alloc {
    alloc_pthread_specific _in_progress;
    char const* _name;
public:
    pool_alloc(char const* name="<nameless>") : _name(name) { }
    void* alloc(int size);
    void free(void* ptr);
};

#endif
