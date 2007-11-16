// -*- mode:c++; c-basic-offset:4 -*-
#ifndef __RWLOCK_H
#define __RWLOCK_H

#include <pthread.h>

#undef STRESS_TEST_MODE

class rwlock {
    
    // represents a thread waiting on this lock
    struct waitlist_node {
	waitlist_node* volatile _next;
	pthread_cond_t* _cond;

	// these two used during debugging/stress testing
	pthread_t _tid;
	bool _reader;
	
	waitlist_node(pthread_cond_t* cond=NULL)
	    : _next(NULL), _cond(cond)
	{
	}
    };


    // Handles the blocking and waking of threads that wait on this lock
    struct waitlist {
	waitlist_node _head;
	waitlist_node* _tail;
	waitlist() { mark_empty(); }
	void mark_empty() { _tail = &_head; }
	void block_me(pthread_cond_t &cond, pthread_mutex_t &lock, bool on_read);
	bool notify_waiter();
	// debugging aid
	void print();
    };

    /** A set of tokens. The sign bit is the "write" token, while the
	others are "read" tokens. A reader needs one read token, while
	a writer needs the write token and all read tokens. When a
	writer is around, the sense of the read token bits reverses -
	1 means held by the writer, while 0 means held by a
	reader.

	This variable largely determines the state of the lock. Using
	a four-bit token set as an example, with sign bit at left:

	0000 - lock free	
	0001 - read mode (one reader)
	0010
	0100
	.
	.
	0111 - read mode (saturated - new readers will block)
	1000 - pending write mode (readers saturated)
	.
	.
	1011 - pending write mode (one reader)
	1101
	1110
	.
	.	
	1111 - write mode
     */
    unsigned long volatile _tokens;    
    pthread_mutex_t _lock;

    waitlist _read_list;
    waitlist _write_list;
    
    bool volatile _write_ready;
public:
    rwlock();

    /* for maximum efficiency the user can pass in the cond var to use
       in case of blocking. Otherwise the lock will have to create
       one */
    void acquire_read(pthread_cond_t &cond);
    void acquire_write(pthread_cond_t &cond);
    
    void acquire_read() {
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	acquire_read(cond);
    }
    void acquire_write() {
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	acquire_write(cond);
    }
    void release();

    // debug aid
    void assert_free();
private:
    long cas(long old_tokens, long new_tokens);
    long claim_read_token(long &old_tokens, bool check_for_writer);
};


#endif
