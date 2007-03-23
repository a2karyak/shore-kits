/* -*- mode:C++; c-basic-offset:4 -*- */

/**
 * @brief Unit test using gethrvtime(). This seems to correctly
 * measure virtual time!
 */
#include "util.h"
#include "scheduler/os_support.h"
#include <time.h>
#include <sys/time.h>



typedef void (*wait_t) (void);
typedef struct
{
    wait_t wait_func;
    c_str name;
} thread_info_t;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;
volatile int signaled = 0;



void* thread_main(void* arg)
{

    thread_info_t* info = (thread_info_t*)arg;


#ifdef FOUND_SOLARIS

#if 0
    /* On Solaris, we have... */
    typedef longlong_t      hrtime_t;
#endif

    hrtime_t start = gethrvtime();
    printf("%s timer has value %llu\n",
           info->name.data(),
           (unsigned long long)start);
    
    info->wait_func();

    hrtime_t end = gethrvtime();
    printf("%s timer has value %llu\n",
           info->name.data(),
           (unsigned long long)end);

    printf("%s elapsed time = %llu ns, which is about %llu seconds\n",
           info->name.data(),
           (unsigned long long)(end - start),
           (unsigned long long)(end - start)/1000000000);

#else

    printf("Sorry! Solaris not found!\n");
    
#endif
    
    return NULL;
}


void wait_cond(void) {
    /* This is ugly... the helper threads releases the mutex even
       though it did not lock it! */
    /* wait for signal from root thread */
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
}


void wait_busy(void) {
    /* Released mutex so root thread knows we exist */
    pthread_mutex_unlock(&mutex);

    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 10000;
    while(!signaled) {
        //fprintf(stderr, "signaled = 0\n");
        int x;
        for (x = 10000000; x--; ) {
            x = x+1;
            x = x-1;
        }
    }
}


void wait_keypress(void) {

    /* wait for helper thread to record time */
    pthread_mutex_lock(&mutex);

    /* wait for keypress */
    printf("Press a key when ready...\n");
    getchar();

    /* signal helper thread */
    signaled = 1; /* if helper using wait_busy */
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    fprintf(stderr, "signaled = %d\n", signaled);
}


int main(int, char**)
{
    util_init();


    thread_info_t root_info   = { wait_keypress,
                                  c_str("root") };
    thread_info_t helper_info = { wait_busy,
                                  c_str("helper") };
  

    pthread_mutex_lock(&mutex);


    // create a new kernel schedulable thread
    pthread_attr_t pattr;
    int err;
    err = pthread_attr_init( &pattr );
    assert(!err);
    err = pthread_attr_setscope( &pattr, PTHREAD_SCOPE_SYSTEM );
    assert(!err);
    
    pthread_t helper;
    if (pthread_create(&helper, &pattr, thread_main, &helper_info)) {
        fprintf(stderr, "pthread_create failed\n");
        return -1;
    }
  
  
    thread_main(&root_info);
  

    pthread_join(helper, NULL);
    fprintf(stderr, "Joined with helper!\n");
  

    return 0;
}
