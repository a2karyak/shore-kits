// -*- mode:c++; c-basic-offset:4 -*-
#include <pthread.h>
#include <unistd.h>
#include "util/clh_rwlock.h"
#include "util/atomic_ops.h"
#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cassert>
#endif

/* I've hand-coded "plain C" versions of the three lock
   functions. They typically reduce instruction counts by 50% or more,
   but the overall performance of my ubench didn't change. In
   addition, the stripped-down versions have not been tested as
   carefully, though I know of no bugs. Finally, they assume
   sparc/solaris (though this should be easy to fix).

   So, for now, we leave them out, but they're there in case they
   become useful (or if somebody wants to see what's going on without
   parsing a bunch of C++ *stuff*
 */
#undef FAST_VERSION

#ifdef TRACE_MODE
#ifndef SERIAL_MODE
#define SERIAL_MODE
#endif
#endif

#ifdef SERIAL_MODE
#ifndef DEBUG_MODE
#define DEBUG_MODE
#endif
#endif

/* A queue node. It resembles a traditional CLH qnode, but with several differences:

   1. CLH operations actually free up a node during lock acquire, but
   wait to claim it until release() to avoid burdening the user with
   two pointers. We often need that freed-up node during acquire, and
   read locks actually have no state between acquire() and release(),
   so there's no need for a predecessor pointer other than for debug.

   2. The "waiting" flag doubles as a read count

   3. A writer may need to send a "shunt" notice that blocks new
   readers/writers while allowing existing readers to still release
   the lock. The notice is simply a pointer to the qnode the last
   reader should pass the baton to instead of its own successor.
 */
struct clh_rwlock::qnode {
    long volatile _read_count;
    qnode_ptr volatile _writer;
#ifdef DEBUG_MODE
    qnode_ptr volatile _pred;
    pthread_t _tid;
#endif
    qnode() : _read_count(0), _writer(NULL)
#ifdef DEBUG_MODE
	    , _pred(NULL), _tid(0)
#endif
    {
    }
};

#ifdef DEBUG_MODE
#include <map>
typedef std::map<clh_rwlock::qnode_ptr, size_t> node_map;
static node_map nodes;
static size_t get_id(clh_rwlock::qnode_ptr ptr) {
#ifdef SERIAL_MODE
    node_map::iterator it=nodes.find(ptr);
    size_t id;
    if(it == nodes.end())
	nodes.insert(std::make_pair(ptr, id=nodes.size()));
    else
	id = it->second;
    return id;
#else
    // if we're not serialized, the map wouldn't work reliably (sorry)
    return (size_t) ptr;
#endif
}

typedef std::map<pthread_t, size_t> indent_map;
static indent_map indents;
static char const* get_indent(pthread_t tid=pthread_self()) {
    static int const COLS = 5;
    static char const spaces[] = "          " "          " "          " "         " "          ";
	
    indent_map::iterator it=indents.find(tid);
    size_t id;
    if(it == indents.end())
	indents.insert(std::make_pair(tid, id=indents.size()));
    else
	id = it->second;

    return &spaces[(COLS-id)*10];
}
static void list_threads() {
    std::vector<pthread_t> tids(indents.size());
    for(indent_map::iterator it=indents.begin(); it!=indents.end(); ++it)
	tids[it->second] = it->first;

    for(int i=0; i < tids.size(); i++)
	fprintf(stderr, "   %2d     ", tids[i]);
    fprintf(stderr, "\n");
}
#endif

#ifdef SERIAL_MODE
pthread_mutex_t serialize = PTHREAD_MUTEX_INITIALIZER;
struct critical_section {
    pthread_mutex_t* _lock;
    critical_section(pthread_mutex_t &lock) : _lock(&lock) { pthread_mutex_lock(_lock); }
    ~critical_section() { pthread_mutex_unlock(_lock); }
};
#endif


// these have circular dependencies and couldn't go in the .h
bool clh_rwlock::lnode::is_waiting() { return _ptr->_read_count < 0; }
clh_rwlock::lnode clh_rwlock::dnode::writer() { return lnode(_ptr->_writer); }
clh_rwlock::dnode::dnode(clh_rwlock::lnode* node) : _ptr(node->_ptr) { }
clh_rwlock::lnode::lnode(clh_rwlock::dnode* node) : _ptr(node->_ptr) { }
clh_rwlock::dnode clh_rwlock::lnode::kill() { return dnode(this); }


/* Pass the baton to our successor (which is spinning on this
   node). The successor now owns the qnode and we had better not touch
   it again lest things break in nasty, unpredictable ways.
 */
inline
void clh_rwlock::lnode::pass_baton(long read_count, clh_rwlock::lnode writer=clh_rwlock::lnode()) {
#ifdef DEBUG_MODE
    assert(pthread_self() == _ptr->_tid);
#endif
#ifdef TRACE_MODE
    fprintf(stderr, "%s  -> (%ld)   \n", get_indent(), get_id(_ptr));
//    fprintf(stderr, "(%ld):\tt@%d ->  ?\n", get_id(_ptr), pthread_self());
#endif
    _ptr->_writer = writer._ptr;
    // FIXME: add support for GCC
    membar_exit();
    _ptr->_read_count = read_count;
}

static long const WAITING = -1;

inline
clh_rwlock::pre_baton_pair clh_rwlock::dnode::join_queue(clh_rwlock::lnode &queue, bool atomic) {

    lnode pred;
    _ptr->_writer=NULL; // clear out everything!
#ifdef DEBUG_MODE
    pthread_t tid = pthread_self();    
    _ptr->_tid = pthread_self();
#ifdef DDEBUG_MODE
    for(qnode_ptr i=((live_handle) queue)._ptr; i && i->_pred; i=i->_pred) {
	if(i->_tid == tid) {
	    // FIXME!!! These three lines of code expose a race I need to track down
	    fprintf(stderr, "Loop! t@%d joining queue: ", tid);
	    queue.print_queue();
	    assert(i->_tid != tid);
	}
    }
#endif
#endif
    _ptr->_read_count = WAITING;
    lnode me(_ptr);

    // FIXME: gcc support
    membar_exit();
    if(atomic) {
	pred = atomic::swap(queue, me);
    }
    else {
	pred = queue;
	queue = me;
    }
#ifdef DEBUG_MODE
    me->_pred = ((live_handle) pred)._ptr;
//    queue.print_queue();
#endif
    return pre_baton_pair(pred, me);
}

#ifdef DEBUG_MODE
#include <set>
bool clh_rwlock::lnode::print_queue() volatile {
    bool cycle = false;
    std::set<size_t> local_nodes;
    for(qnode_ptr i = _ptr; i && i->_pred && !cycle; i=i->_pred) {
	size_t id = get_id(i);
	if(local_nodes.find(id) == local_nodes.end()) 
	    local_nodes.insert(id);
	else
	    cycle = true;

	fprintf(stderr, "t@%ld (%ld) -> ", i->_tid, id);
    }
    
    fprintf(stderr, cycle? "<cycle>\n" : "<eoq>\n");
    return !cycle;
}
#endif
clh_rwlock::clh_rwlock() {
    /* prime both queues. The main queue starts out ready, with no
       readers or writers. The shunt queue starts out "blocked"
       because the writer will spin on it while waiting for readers to
       release() the lock.
     */
    dnode(create_handle()).join_queue(_queue, false)._me.pass_baton(0);
    dnode(create_handle()).join_queue(_shunt_queue, false);
#ifdef DEBUG_MODE
    _queue->_tid = 0;
    _shunt_queue->_tid = 0;
#endif
}

clh_rwlock::dead_handle clh_rwlock::create_handle() {
    dead_handle h = {new qnode};
    return h;
}
void clh_rwlock::destroy_handle(clh_rwlock::dead_handle h) {
    delete h._ptr;
}

enum Mode { FREE, READ, WRITE, SHUNT, BEGS, ENDS, SPLICE };
char const* mode_names[] = { "    ---   ", "  + R +   ", "  + W +   ",
			     "   SHUNT  ", "   BEGS   ", "   ENDS   ", "  SPLICE  " };
#ifdef DEBUG_MODE
#include <deque>
typedef std::pair<pthread_t, Mode> lock_action;
typedef std::deque<lock_action> action_list;
static action_list history;
static void history_push(Mode m) {
    pthread_t tid = pthread_self();
#ifdef TRACE_MODE
    fprintf(stderr, "%s%10s\n", get_indent(tid), mode_names[m]);
#endif
    
    history.push_back(std::make_pair(tid, m));    
    while(history.size() > 30)
	history.pop_front();
}
static void print_history() {
    // first pass finds the indents
    for(indent_map::iterator it=indents.begin(); it != indents.end(); ++it) {
	printf("%s%10d", get_indent(it->first), it->first);
    }
    printf("\n");
    for(action_list::iterator it=history.begin(); it != history.end(); ++it) {
	printf("%s%10s\n", get_indent(it->first), mode_names[it->second]);
    }
}
#else
inline 
static void history_push(Mode) { /* nop */ }
#endif

/* Join the main queue and wait my turn. If check_for_writer, then
   also deal with shunt notices when applicable.
*/
clh_rwlock::post_baton_pair clh_rwlock::wait_for_baton(clh_rwlock::dnode seed, bool check_for_writer) {
    lnode writer;
    post_baton_pair main_result = seed.join_queue(_queue).spin();
#ifdef DDEBUG_MODE
    fprintf(stderr, "t@%d has baton, queue: ", pthread_self()); _queue.print_queue();
#endif
    dnode &pred = main_result._pred;
    while(check_for_writer && (writer=pred.writer()).valid() ) {
	pre_baton_pair shunt_entry;
	if(pred->_read_count == 0) {
	    history_push(ENDS);
	    /* I immediately follow the last reader's release, and
	       arrived before it could splice the shunt queue back
	       into the main queue. I need to join _shunt_queue, pass
	       a baton to the writer, then spin until that baton
	       reaches me again. Note that there is nothing special
	       about getting to wake the shunt queue -- I still might
	       have writers ahead of me that will create a new one for
	       me to join or splice.

	       NOTE: after we join the _shunt_queue,
	       _shunt_queue==pred==shunt_entry._me will remain live,
	       with no successors, until the next writer takes
	       ownership and starts a new shunt.
	    */
	    shunt_entry = pred.join_queue(_shunt_queue, false);
#ifdef DEBUG_MODE
	    writer->_tid = pthread_self(); // admit whodunnit
#endif
	    writer.pass_baton(0);
	    shunt_entry._me = main_result._me;
	}
	else {
	    /* A writer got here ahead of me. Join the shunt queue,
	       then pass the baton on so readers can release(). It's
	       entirely possible that multiple writers are queued up
	       ahead of us, and we have to join a shunt queue for each
	       one that has readers ahead of it. Since all requests
	       are handled in FIFO order, we shouldn't have to loop
	       very many times.
	    */
	    history_push(SHUNT);
	    long read_count = pred->_read_count;
	    shunt_entry = pred.join_queue(_shunt_queue, false);
	    main_result._me.pass_baton(read_count, writer);
	}
	
	// now we wait...
	main_result = shunt_entry.spin();
    }
    
    return main_result;
}

#ifndef FAST_VERSION
clh_rwlock::live_handle clh_rwlock::acquire_write(clh_rwlock::dead_handle _seed) {
#ifdef SERIAL_MODE
    critical_section cs(serialize);
#endif
#ifdef TRACE_MODE
    fprintf(stderr, "%s     W    \n", get_indent());
#endif
    /* Unlike readers, writers don't pass the baton until
       release(). This prevents anyone else from acquiring the lock in
       any mode. However, without some black magic it also prevents
       any existing readers from releasing their locks...
    */
    dnode seed(_seed);
    post_baton_pair main_result = wait_for_baton(seed);
    dnode &pred = main_result._pred;
    lnode &me = main_result._me;
    long read_count = pred->_read_count;
    if(read_count) {
	/* Create a shunt so that existing readers can release their
	   locks. This will only happen once -- wait_for_baton would
	   have handled any writers waiting ahead of us.
	 
	   I will spin on the shunt's predecessor after passing it to
	   my successors as the _writer. That way, when the baton
	   finally reaches the last reader release(), the reader will
	   know who to give it to.
	*/
	history_push(BEGS);
	pre_baton_pair shunt_entry = pred.join_queue(_shunt_queue, false);
	me.pass_baton(read_count, shunt_entry._pred);
	main_result = shunt_entry.spin();
    }

    history_push(WRITE);
    // act like a normal CLH lock - _writer points to the node to recycle
    me->_writer = ((dead_handle) pred)._ptr;
    assert(me->_writer);
    // me->_read_count is no-touchie! we have real successors
    membar_enter();
    return me;
}
#else
clh_rwlock::live_handle clh_rwlock::acquire_write(clh_rwlock::dead_handle _seed) {
    qnode_ptr me = _seed._ptr;
    
    // join_queue
    me->_writer = NULL;
    membar_producer();
    me->_read_count = WAITING;
    union { lnode *_l; uint64_t* _i; } q={&_queue};
    qnode_ptr pred = (qnode_ptr) atomic_swap_64(q._i, (uint64_t) me);    
    lnode(pred).spin();
    membar_consumer();

    // wait_for_baton
    qnode_ptr writer;
    while((writer=pred->_writer)) {
	// join the shunt queue
	long read_count = pred->_read_count;
	qnode_ptr me_wait = pred;
	me_wait->_writer = NULL;
	me_wait->_read_count = WAITING;
	pred = ((live_handle)_shunt_queue)._ptr;
	_shunt_queue = lnode(me_wait);
	
	if(pred->_read_count == 0) {
	    // ENDS
	    // pass_baton to writer (me_wait becomes next _shunt_queue)
	    writer->_writer = NULL;
	    membar_producer();
	    writer->_read_count = 0;
	}
	else {
	    // SHUNT
	    // pass_baton to successor
	    me->_writer = writer;
	    membar_producer();
	    me->_read_count = read_count;
	    me = me_wait;
	}

	lnode(pred).spin();
	membar_consumer();
    }

    // now the actual write stuff
    long read_count = pred->_read_count;
    if(read_count) {
	qnode_ptr me_wait = pred;
	me_wait->_writer = NULL;
	me_wait->_read_count = WAITING;
	pred = ((live_handle)_shunt_queue)._ptr;
	_shunt_queue = lnode(me_wait);

	// start shunt notice 
	me->_writer = pred;
	membar_producer();
	me->_read_count = read_count;

	// and wait for the baton to come back
	me = me_wait;
	lnode(pred).spin();
    }

    // act like a normal CLH lock, with _writer == the node to recycle
    me->_writer = pred;
    membar_enter();
    live_handle h = {me};
    return h;
}
#endif

#ifndef FAST_VERSION
clh_rwlock::live_handle clh_rwlock::acquire_read(clh_rwlock::dead_handle _seed) {
#ifdef SERIAL_MODE
    critical_section cs(serialize);
#endif
#ifdef TRACE_MODE
    fprintf(stderr, "%s     R    \n", get_indent());
#endif
    /* Readers pass the baton as soon as they verify that there are no
       writers waiting ahead of them in line and can update the read
       count.
    */
    dnode seed(_seed);
    assert(_seed._ptr);
    post_baton_pair result = wait_for_baton(seed);
    history_push(READ);
    dnode &pred = result._pred;
    result._me.pass_baton(pred->_read_count+1);

    /* mark 'me' so release knows we have a read lock. Note that this
       node isn't actually live at all, but we have to match
       acquire_write()...
    */
    lnode me(&result._pred);
    me->_read_count = 1;
    me->_writer = NULL;
    membar_enter();
    return me;
}
#else
clh_rwlock::live_handle clh_rwlock::acquire_read(clh_rwlock::dead_handle _seed) {
    qnode_ptr me = _seed._ptr;
    assert(me);

    // join_queue
    me->_writer = NULL;
    membar_producer();
    me->_read_count = WAITING;
    union { lnode *_l; uint64_t* _i; } q={&_queue};
    qnode_ptr pred = (qnode_ptr) atomic_swap_64(q._i, (uint64_t) me);    
    lnode(pred).spin();
    membar_consumer();

    // wait_for_baton
    qnode_ptr writer;
    while((writer=pred->_writer)) {
	// join the shunt queue
	long read_count = pred->_read_count;
	qnode_ptr me_wait = pred;
	me_wait->_writer = NULL;
	me_wait->_read_count = WAITING;
	pred = ((live_handle)_shunt_queue)._ptr;
	_shunt_queue = lnode(me_wait);
	
	if(pred->_read_count == 0) {
	    // ENDS
	    // pass_baton to writer (me_wait becomes next _shunt_queue)
	    writer->_writer = NULL;
	    membar_producer();
	    writer->_read_count = 0;
	}
	else {
	    // SHUNT
	    // pass_baton to successor
	    me->_writer = writer;
	    membar_producer();
	    me->_read_count = read_count;
	    me = me_wait;
	}

	lnode(pred).spin();
	membar_consumer();
    }

    // now the actual read stuff
    me->_writer = NULL;
    membar_producer();
    me->_read_count = pred->_read_count+1;
    
    pred->_read_count = 1;
    pred->_writer = NULL;
    membar_enter();
    live_handle h = {pred};
    return h;
}
#endif

#ifndef FAST_VERSION
clh_rwlock::dead_handle clh_rwlock::release(clh_rwlock::live_handle _me) {
#ifdef SERIAL_MODE
    critical_section cs(serialize);
#endif
#ifdef TRACE_MODE
    fprintf(stderr, "%s     -    \n", get_indent());
#endif
    membar_exit();
    lnode me = _me;
    long my_count = me->_read_count;
    qnode_ptr my_writer = me->_writer;
    dnode result;
    if(me.is_waiting()) {
	history_push(FREE);
	// we hold the baton... pass it on to release the write lock
	result = dnode(me->_writer);
	me.pass_baton(0);      
    }
    else if(my_count == 1) {
	// wait for the baton, ignoring shunt requests
	dnode seed = me.kill();
	post_baton_pair main_result = wait_for_baton(seed, false);
	history_push(FREE);
	dnode &pred = main_result._pred;
	lnode writer = pred.writer();
	long new_read_count = pred->_read_count - 1;
	
	/* If we are the last reader blocking some writer, we need to
	   pass the baton to the writer instead of our successor, and
	   for our successor to get the baton once the shunt queue
	   clears (because our successor comes after the end of the
	   shunt queue in FIFO order). There are two ways we can pull
	   this off.

	   If we already have a successor, we simply pass the baton
	   with a writer and _read_count==0. Our successor will join
	   the shunt queue as usual, but pass the baton to the writer
	   before spinning.

	   If we don't already have a successor (ie, we can pull
	   ourselves out of the queue with a CAS), we have to splice
	   in the shunt queue ourselves or risk deadlock (all other
	   threads could be waiting in the shunt queue).
	*/
#ifdef TRACE_MODE
	if(writer.valid() && new_read_count == 0) {
	    fprintf(stderr,
		    "Attempting CAS(%p, %p, %p):\n",
		    get_id(((live_handle)_queue)._ptr),
		    get_id(((live_handle) main_result._me)._ptr),
		    get_id(((live_handle) _shunt_queue)._ptr));
//	    if(_queue != main_result._me)
	//	printf("Should fail!\n");
	}
#endif
	if(writer.valid() && new_read_count == 0 && atomic::cas(_queue, main_result._me, _shunt_queue) == main_result._me) {
	    history_push(SPLICE);
	    _shunt_queue = main_result._me; // becomes the seed for the next shunt queue
#ifdef DEBUG_MODE
	    _shunt_queue->_pred = NULL;
	    _shunt_queue->_tid = 0;
	    writer->_tid = pthread_self(); // admit that we did the dirty deed
#endif
	    writer.pass_baton(0);
	} else {
	    main_result._me.pass_baton(new_read_count, writer);
	}
	result = main_result._pred;
    }
    else {
	// eek!
	assert(me.is_waiting() || my_count == 1);
    }

    // done
#ifdef DEBUG_MODE
    result->_tid = 0;
    result->_pred = NULL;
#endif
    assert(((dead_handle)result)._ptr);
    return result;
}
#else
clh_rwlock::dead_handle clh_rwlock::release(clh_rwlock::live_handle _me) {
    membar_exit();
    qnode_ptr me = _me._ptr;
    qnode_ptr pred;
    switch(me->_read_count) {
    case WAITING:
	// writer
	pred = me->_writer; // stored here earlier...
	me->_writer = NULL;
	membar_producer();
	me->_read_count = 0;
	break;
    case 1: {
	// reader
	// wait for baton (no shunts)
	me->_writer = NULL;
	membar_producer();
	me->_read_count = WAITING;
	union { lnode *_l; uint64_t* _i; } q={&_queue}, s={&_shunt_queue};
	uint64_t mi = (uint64_t) me;
	pred = (qnode_ptr) atomic_swap_64(q._i, mi);    
	lnode(pred).spin();
	membar_consumer();

	// last reader and nobody behind me (yet)?
	long new_read_count = pred->_read_count-1;
	qnode_ptr writer = pred->_writer;
	if(writer && new_read_count == 0 && mi == atomic_cas_64(q._i, mi, *s._i)) {
	    // SPLICE
	    *s._i = mi;
	    writer->_writer = NULL;
	    membar_producer();
	    writer->_read_count = 0;
	}
	else {
	    // forward shunt notice
	    me->_writer = writer;
	    membar_producer();
	    me->_read_count = new_read_count;
	}
    }
	break;
    default:
	// eek!
	assert(me->_read_count == WAITING || me->_read_count == 1);
    }

    // done!
    dead_handle h = {pred};
    return h;
}
#endif
