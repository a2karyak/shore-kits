// -*- mode:c++; c-basic-offset:4 -*-
#include "util/rwlock.h"

#ifdef __SUNPRO_CC
#include <atomic.h>
#endif

#define STRESS_TEST_MODE

#ifdef STRESS_TEST_MODE
    // use fewer bits during testing so we can easily saturate the lock
#define WRITER_WAITING(tokens) ((tokens) & 0x80)
#define WRITE_LOCKED(tokens) (((tokens) & 0xff) == 0xff)
#define READ_LOCKED(tokens) ((tokens) & 0x7f)
#define LOCK_FREE(tokens) (((tokens) & 0xff) == 0x00)
#else
#define WRITER_WAITING(tokens) ((tokens) < 0)
#define WRITE_LOCKED(tokens) ((tokens) == -1l)
#define READ_LOCKED(tokens) ((tokens) > 0)
#define LOCK_FREE(tokens) ((tokens) == 0)
#endif

#ifdef STRESS_TEST_MODE
#include "util/stopwatch.h"
#include "unistd.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define YIELD sched_yield()
#else
#define YIELD  
#endif

rwlock::rwlock()
    : _tokens(0), _write_ready(false)
{
#ifdef STRESS_TEST_MODE
    _read_list._head._reader = true;
    _write_list._head._reader = false;
    _read_list._head._tid = _write_list._head._tid = pthread_self();
#endif
    pthread_mutex_init(&_lock, NULL);
}

long rwlock::cas(long old_tokens, long new_tokens) {
#ifdef STRESS_TEST_MODE
    // chop!
    new_tokens &= 0xff;
#endif
#ifdef __SUNPRO_CC
    return atomic_cas_ulong(&_tokens, old_tokens, new_tokens);
#else
    // gcc's intrinsics are overloaded for all types
    return __sync_val_compare_and_swap(&_tokens, old_tokens, new_tokens);
#endif
}

inline void rwlock::waitlist::block_me(pthread_cond_t &cond, pthread_mutex_t &lock, bool on_read) {
    // add myself to the list
    waitlist_node me(&cond);
    _tail->_next = &me;
    _tail = &me;
#ifdef STRESS_TEST_MODE
    me._reader = on_read;
    me._tid = pthread_self();
#endif
    
    // now wait for somebody to remove me
    while(me._next != &me)
	pthread_cond_wait(&cond, &lock);
}

inline bool rwlock::waitlist::notify_waiter() {
    waitlist_node* node = _head._next;
    if(!node)
	return false;

    _head._next = node->_next;
    node->_next = node; // the signal that node has left the list
    if(_tail == node)
	mark_empty();
	    
    // wake it
    pthread_cond_signal(node->_cond);
    return true;
}

// return the new token set if successful; zero on failure
inline long rwlock::claim_read_token(long &old_tokens, bool check_for_writer) {
    // NOTE: I tried bounding the loop count and it hurt performance
    while(1) {
	YIELD; // to "encourage" other threads to modify _tokens concurrently
	
	/* find a free token to claim. If there are no more read
	   tokens, this operation will try to claim the write token,
	   which we don't want.
	*/
	long itokens =  ~old_tokens;
	long my_token = itokens & -itokens;
	long new_tokens = my_token | old_tokens;

	// is there a writer around? are we out of read tokens?
	if(check_for_writer && WRITER_WAITING(new_tokens))
	    return 0;

	// attempt the swap. If we succeeded, we now hold a read lock
	long cur_tokens = cas(old_tokens, new_tokens);
	if(cur_tokens == old_tokens)
	    return new_tokens;

	// update and try again
	old_tokens = cur_tokens;
    }
}

void rwlock::acquire_read(pthread_cond_t &cond) {
    long old_tokens = _tokens;

    /* Try to grab a read token. If we fail, block. There's a race
       here, where we might fail to grab a token, only to have one
       become available before we enter our critical section. This is
       never a problem if at least one token is held because we will
       be first in line when it gets released. However, if there are
       no tokens held we could theoretically be the last thread to
       ever request this lock and would never unblock. Fortunately,
       tokens are only released from within critical sections, so we
       can easily avoid sleeping on a free lock by just testing again
       after acquiring the mutex.
     */
    if(!claim_read_token(old_tokens, true)) {
	YIELD; // to encourage a delay in entering the critical section
	pthread_mutex_lock(&_lock);

	YIELD; // allow changes to _tokens but not to waitlists
	
	// never sleep on a free lock!
	if(cas(0, 1))
	    _read_list.block_me(cond, _lock, true);
	
	pthread_mutex_unlock(&_lock);
    }
}

void rwlock::acquire_write(pthread_cond_t &cond) {
    long old_tokens = _tokens;
    /* NOTE: we can't bound this loop because the writer *must* get
       the write token before blocking (or the readers will ignore it).
     */
    while(1) {
	YIELD; // to encourage other threads to change _tokens on me
	
	// already a writer around?
	if(WRITER_WAITING(old_tokens)) break;

	/* attempt the swap. if we succeeded, we now hold all
	   previously unclaimed tokens, including the writer
	   token. Existing readers are now marked by '0' instead of
	   '1', and will each set one bit when they release their read
	   lock, rather than clearing it.
	 */
	long cur_tokens = cas(old_tokens, ~old_tokens);

	// if, by some miracle, the lock was free and the swap succeeded, we can go now
	if(cur_tokens == old_tokens) {
	    if(LOCK_FREE(old_tokens))
		return;
	    else
		break;
	}

	old_tokens = cur_tokens;
    }

    /* block until somebody hands us the lock. See comments in
       acquire_read() for an explanation of the race the extra cas
       operation helps us avoid.

       There is a second race for writers: If we acquire the write
       token while readers hold the lock, and those readers all
       release their tokens before we enter our critical section, we
       face deadlock: we hold the lock in write mode but will never
       wake up because we slept too late.

       Unfortunately, we can't avoid this by simply testing for -1.
       If we are not the first writer in line, one of them is supposed
       to wake up. Because that writer may not have entered its
       critical section yet, either, it would perform the same test as
       we did, and we would end up with two writers at once.

       Solution: the reader that turns in the last token will set a
       flag if it can't find any writers to wake up. Because the flag
       is protected by the mutex, we will either see it still set
       (making us first in line) or some other writer will get to it
       first and we will wait. In the latter case, however, we now
       know the lock is held and that we can safely sleep.
     */
    YIELD; // to delay entry into the critical section
    pthread_mutex_lock(&_lock);

    // did the lock come to us while we were acquiring the mutex?
    YIELD; // allow changes to _tokens but not to waitlists
    old_tokens = _tokens;
    if(_write_ready)
	_write_ready = false;

    /* Watch out! There's a race here --
       1. some other thread acquires the write lock
       2. we decide to block
       3. other thread releases the lock with readers waiting
       4. we enter this critical section
       
       If we sleep under these circumstances, we will be invisible
       because the other writer cleared the write token; readers will
       happily keep working, leaving us stuck in limbo until another
       writer comes to the rescue (by not hitting this race). If this
       happens, start over from scratch.
     */
    else if(WRITER_WAITING(old_tokens))
	_write_list.block_me(cond, _lock, false);

    pthread_mutex_unlock(&_lock);

    // did we take too long to enter the critical section? start over
    if(!WRITER_WAITING(old_tokens))
	acquire_write(cond);
}

void rwlock::release() {
    /*
      There is one set of conditions where we do not need to grab the
      mutex to safely release a lock

      1. We are a reader
      2. There are no readers or writers waiting
      3. There are other readers as well (technically optional)
      4. We can release our token in a reasonable number of CAS attempts
     */
    long old_tokens = _tokens;
    for(int i=0; i < 3; i++) {
	// waiters?
	if(WRITER_WAITING(old_tokens) || _read_list._head._next)
	    break;
	
	long my_token = old_tokens & -old_tokens;
	long new_tokens = old_tokens & ~my_token;

	/* if we're the last one out we must grab the mutex to
	   avoid nasty races with waiters who haven't been able to
	   enter their critical sections yet.
	*/
	if(LOCK_FREE(new_tokens))
	    break;

	// try to release our token
	long cur_tokens = cas(old_tokens, new_tokens);
	if(cur_tokens == old_tokens)
	    return; // success!

	// try again
	old_tokens = cur_tokens;
    }
    
    // we can't afford to miss any threads that might decide to wait
    pthread_mutex_lock(&_lock);

    // writers take precedence to avoid starvation
    old_tokens = _tokens;
    
    YIELD; // encourage changes to _tokens after we read it
    if(WRITE_LOCKED(old_tokens)) {
	/* I am a writer. Conveniently, nobody will mess with the
	   _tokens because I have exclusive access.
	 */

	// no waiting writers? try readers then
	if(!_write_list.notify_waiter()) {
	    /* WARNING: we may have caught a potential writer between
	       deciding to block and actually entering its critical
	       section. Because we have no way to detect this, we
	       simply free the lock anyway, and depend on it to notice
	       that the write token has been cleared.
	     */
	    long new_tokens = 0;
	    // free up to 63 waiting threads (the number of read tokens that became available)
	    for(long my_token=1; READ_LOCKED(my_token) && _read_list.notify_waiter(); my_token<<=1) 
		new_tokens |= my_token;

	    // 0 <= i < 64 read locks held now
	    _tokens = new_tokens;
	}	
    }
    else if(WRITER_WAITING(old_tokens)) {
    defer_to_writer:
	/* I'm a reader, and there's a writer waiting. There's no risk
	   of that waiter disappearing because it needs to hold the
	   mutex to do so and I will only release my read lock while
	   holding it. I need to *set* one bit (thus giving
	   the token to the waiting writer). If my token was the last,
	   wake up the writer
	 */
	long new_tokens = claim_read_token(old_tokens, false);
	if(WRITE_LOCKED(new_tokens)) {
	    
	    // all the tokens are in. Wake up a writer
	    if(!_write_list.notify_waiter()) {
		/* the writer didn't make it into a critical section
		   in time. Save the signal for when it does arrive.

		   NOTE: this is almost the same race as for _tokens
		   == -1 and no writers waiting. However, this time we
		   can detect the "invisible" writer because the write
		   token is set.
		*/
		_write_ready = true;
	    }
	}
    }
    else {
	/* I'm a reader and there are (potentially) other readers
	   waiting.

	   Watch out for the case where a writer arrives while I'm
	   trying to release my lock:

	   1. If there's a waiting reader, I go ahead and wake
	   it. This does not require an update to the _tokens, so the
	   writer will just think the reader-reader hand-off occurred
	   before it arrived.

	   2. If there are no waiting readers, I go ahead and hand my
	   token to the writer. This requires an update of the
	   _tokens, but nobody else will change it out from under me
	   -- other readers will see the writer token and back off,
	   and the writer is waiting for me to release mine. Any
	   arriving readers will think the writer arrived just before
	   they did.
	 */
	// only release my token if no waiting readers
	if(!_read_list.notify_waiter()) {
	    while(1) {
		long my_token = old_tokens & -old_tokens;
		long new_tokens = old_tokens & ~my_token;
		long cur_tokens = cas(old_tokens, new_tokens);
		if(cur_tokens == old_tokens)
		    break; // success!
		
		old_tokens = cur_tokens;
		if(WRITER_WAITING(old_tokens)) {
		    // oops! a writer arrived. defer to it (my apologies for the goto)
		    goto defer_to_writer;
		}

		// try again
	    }
	}
#ifdef STRESS_TEST_MODE
	// are we missing chances to wake up readers? (so far, no)
	long itokens = ~old_tokens;
	long free_token = itokens & -itokens;
	if(READ_LOCKED(free_token) && _read_list._head._next)
	    fprintf(stderr, "*** %x\n", old_tokens);
#endif
    }

    // phew! that was hairy
    pthread_mutex_unlock(&_lock);
}

#ifdef STRESS_TEST_MODE

void rwlock::waitlist::print() {
    fprintf(stderr, "%s: ", _head._reader? "RD" : "WR");
    if(!_head._next) {
	fprintf(stderr, "(empty)\n");
	return;
    }
    for(waitlist_node* n=_head._next; n; n=n->_next) {
	fprintf(stderr, " t@%d", n->_tid);
    }
    fprintf(stderr, "\n");
}

void rwlock::assert_free() {
    assert(!_tokens);
    assert(!_read_list._head._next);
    assert(!_write_list._head._next);
    assert(!_write_ready);
}

int volatile barrier_count=0;
int volatile barrier_waiters=0;
pthread_mutex_t barrier_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t barrier_cond = PTHREAD_COND_INITIALIZER;

rwlock global_lock;

static int const THREADS = 32;
static int const ROUNDS = 100;
// stress test
extern "C" void* run(void* arg) {
    
    unsigned rseed = (int) pthread_self() * (int) stopwatch_t().now();
    int count=1;
    while(count <= ROUNDS) {
	// synch up at the start of each round
	pthread_mutex_lock(&barrier_lock);
	if(count > barrier_count) {
	    if(++barrier_waiters == THREADS) {
		printf("\n");
		if(barrier_count)
		    fprintf(stderr, "Checking locks...\n");
		global_lock.assert_free();
		if(barrier_count) {
		    fprintf(stderr, "Round %d complete, all assertions pass\n", barrier_count);
		}
		pthread_cond_broadcast(&barrier_cond);
		barrier_count++;
		barrier_waiters = 0;
	    }
	    else {
		printf(".");
		pthread_cond_wait(&barrier_cond, &barrier_lock);
	    }

	    count = barrier_count+1;
	}	
	pthread_mutex_unlock(&barrier_lock);

	for(int i=0; i < 1000; i++) {
	    union {
		unsigned word;
		struct {
		    unsigned sleep1 : 10;
		    unsigned sleep2 : 7;
		    unsigned read : 6;
		};
	    } rval = {rand_r(&rseed)};
	    //printf("%d %d %d %d\n", rval.sleep1, rval.sleep2, rval.read, rval.rest);
	    // sleep for 0-1k usec 
	    usleep(rval.sleep1);

	    // one in 2**n accesses will be a write
	    if(rval.read)
		global_lock.acquire_read();
	    else
		global_lock.acquire_write();

	    usleep(rval.sleep2);

	    global_lock.release();
	}
    }
    return NULL;
}

int main() {
    pthread_t tids[THREADS];

    for(long i=0; i < THREADS; i++)
	pthread_create(&tids[i], NULL, &run, (void*) i);
    for(long i=0; i < THREADS; i++)
	pthread_join(tids[i], NULL);

    return 0;
}
#else

#include "util/stopwatch.h"
#include <stdio.h>

static long const COUNT = 1l << 20;

rwlock global_lock;
extern "C" void* test_shared_rwlocks(void* arg) {
    stopwatch_t timer;
    for(long i=0; i < COUNT; i++) {
	global_lock.acquire_read();
	global_lock.release();
    }
    union {
	double d;
	void* vptr;
    } u = {timer.time()};
    return u.vptr;
}

extern "C" void* test_independent_rwlocks(void* arg) {
    rwlock local_lock;
    stopwatch_t timer;
    for(long i=0; i < COUNT; i++) {
	local_lock.acquire_read();
	local_lock.release();
    }
    
    union {
	double d;
	void* vptr;
    } u = {timer.time()};
    return u.vptr;
}

int main() {
    static int const THREADS = 32;
    pthread_t tids[THREADS];

    for(int k=1; k <= THREADS; k*=2) {
	for(long i=0; i < k; i++)
	    pthread_create(&tids[i], NULL, &test_shared_rwlocks, (void*) i);
	union {
	    void* vptr;
	    double d;
	} u;
	double total = 0;
	for(long i=0; i < k; i++) {
	    pthread_join(tids[i], &u.vptr);
	    total += u.d;
	}
	printf("%d %.3lf\n", k, total/k);
    }
    return 0;
}
#endif
