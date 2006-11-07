/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util/thread.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <cstring>
#include <unistd.h>



/* internal data structures */

static pthread_key_t THREAD_KEY_SELF;



/* internal helper functions */

static void thread_destroy(void* thread_object);
extern "C" void* start_thread(void *);



/* method definitions */


/**
 *  @brief thread_t base class constructor. Every subclass constructor
 *  goes through this. Subclass should invoke the thread_t static
 *  initialization functions (init_thread_name() or
 *  init_thread_name_v()) to set up a new thread object's name.
 */
thread_t::thread_t(const c_str &name)
    : _thread_name(name), _delete_me(true)
{
    // do nothing...
}


void thread_t::reset_rand() {

    int fd = open("/dev/urandom", O_RDONLY);
    assert(fd != -1);
    
    int read_size = read(fd, &_rand_seed, sizeof(int));
    assert(read_size == sizeof(int));

    close(fd);
}


class root_thread_t : public thread_t {
    
public:
    
    /**
     *  @brief 
     */
    root_thread_t(const c_str &name)
        : thread_t(name)
    {
    }


    virtual void* run() {
        // must be overwridden, but never called for the root thread
        assert(false);
        return NULL;
    }
};



/**
 *  @brief Initialize thread module.
 *
 *  @return void
 */

void thread_init(void)
{
    int err = pthread_key_create( &THREAD_KEY_SELF, thread_destroy );
    THROW_IF(ThreadException, err);
    
    root_thread_t* root_thread = new root_thread_t("root-thread");

    err = pthread_setspecific(THREAD_KEY_SELF, root_thread);
    THROW_IF(ThreadException, err);
}
 


thread_t* thread_get_self(void)
{
    // It would be nice to verify that the name returned is not
    // NULL. However, the name of the root thread can be NULL if we have
    // not yet completely initialized it.
    return (thread_t*)pthread_getspecific(THREAD_KEY_SELF);
}



/**
 *  @brief Creates a new thread and starts it.
 *
 *  @param thread A pointer to a pthread_t that will store the ID of
 *  the newly created thread.
 *
 *  @param t - A thead that contains the function to run.
 *
 *  @return 0 on success. Non-zero on error.
 */

pthread_t thread_create(thread_t* t)
{
    int err;
    pthread_t tid;
    pthread_attr_t pattr;

  
    // create a new kernel schedulable thread
    err = pthread_attr_init( &pattr );
    THROW_IF(ThreadException, err);
  
    err = pthread_attr_setscope( &pattr, PTHREAD_SCOPE_SYSTEM );
    THROW_IF(ThreadException, err);
  
    err = pthread_create(&tid, &pattr, start_thread, t);
    THROW_IF(ThreadException, err);

    return tid;
}



pthread_mutex_t thread_mutex_create(const pthread_mutexattr_t* attr)
{

    pthread_mutexattr_t        mutex_attr;
    const pthread_mutexattr_t* ptr_mutex_attr;
    int err;

  
    if (attr == NULL)
    {
        err = pthread_mutexattr_init(&mutex_attr);
        THROW_IF(ThreadException, err);

        err = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
        THROW_IF(ThreadException, err);
    
        err = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_PRIVATE);
        THROW_IF(ThreadException, err);

        ptr_mutex_attr = &mutex_attr;
    }
    else
    {
        ptr_mutex_attr = attr;
    }


    pthread_mutex_t mutex;
    err = pthread_mutex_init(&mutex, ptr_mutex_attr);
    THROW_IF(ThreadException, err);
    return mutex;
}



void thread_mutex_lock(pthread_mutex_t &mutex)
{
    int err = pthread_mutex_lock(&mutex);
    THROW_IF(ThreadException, err);
}



void thread_mutex_unlock(pthread_mutex_t &mutex)
{
    int err = pthread_mutex_unlock(&mutex);
    THROW_IF(ThreadException, err);
}



void thread_mutex_destroy(pthread_mutex_t &mutex)
{
    int err = pthread_mutex_destroy(&mutex);
    THROW_IF(ThreadException, err);
}



pthread_cond_t thread_cond_create(const pthread_condattr_t* attr)
{
    pthread_cond_t cond;
    int err = pthread_cond_init(&cond, attr);
    THROW_IF(ThreadException, err);
    return cond;
}



void thread_cond_destroy(pthread_cond_t &cond)
{
    int err = pthread_cond_destroy(&cond);
    THROW_IF(ThreadException, err);
}



void thread_cond_signal(pthread_cond_t &cond)
{
    int err = pthread_cond_signal(&cond);
    THROW_IF(ThreadException, err);
}



void thread_cond_broadcast(pthread_cond_t &cond)
{
    int err = pthread_cond_broadcast(&cond);
    THROW_IF(ThreadException, err);
}



void thread_cond_wait(pthread_cond_t &cond, pthread_mutex_t &mutex)
{
    int err = pthread_cond_wait(&cond, &mutex);
    THROW_IF(ThreadException, err);
}

bool thread_cond_wait(pthread_cond_t &cond, pthread_mutex_t &mutex,
                           struct timespec &timeout)
{
    int err = pthread_cond_timedwait(&cond, &mutex, &timeout);
    switch(err) {
    case 0: return true;
    case ETIMEDOUT: return false;
    default: THROW_IF(ThreadException, err);
    }
    // unreachable
    assert(false);
}



/**
 *  @brief thread_main function for newly created threads. Receives a
 *  thread_t object as its argument and it calls its run() function.
 *
 *  @param thread_object A thread_t*.
 *
 *  @return The value returned by thread_object->run().
 */

void* start_thread(void* thread_object)
{
    thread_t* thread = (thread_t*)thread_object;

    int err;

    // Register local data. Should not fail since we only need two
    // pieces of thread-specific storage.
    err = pthread_setspecific(THREAD_KEY_SELF, thread);
    THROW_IF(ThreadException, err);
  
    thread->reset_rand();
    return thread->run();
}



static void thread_destroy(void* thread_object)
{
    thread_t* thread = (thread_t*)thread_object;
    if(thread && thread->delete_me())
        delete thread;
}
