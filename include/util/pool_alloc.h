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

#define DECLARE_POOL_ALLOC_NEW_AND_DELETE() \
    void* operator new(size_t size); \
    void operator delete(void* ptr)

#define DEFINE_POOL_ALLOC_NEW_AND_DELETE(class, name) \
static pool_alloc name##_alloc(#name); \
void* class::operator new(size_t size) { \
    return name##_alloc.alloc(size); \
} \
void class::operator delete(void* ptr) { \
    name##_alloc.free(ptr); \
}
    
#endif
