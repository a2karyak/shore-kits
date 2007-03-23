/* -*- mode:C++; c-basic-offset:4 -*- */

/**
 * @brief Unit test to test timers created using the
 * CLOCK_THREAD_CPUTIME_ID clockid_t. Basically, we have a busy wait
 * thread and a thread that sleeps. We see that both threads get back
 * different values from timer_gettime().
 *
 * This does not seem to work on enceladus, although I remember it
 * working on crete.
 */
#define __USE_XOPEN2K
#include "util.h"
#include <time.h>
#include <sys/time.h>



#ifndef _POSIX_THREAD_CPUTIME
#error No per-thread timers!
#endif



typedef void (*wait_t) (void);
typedef void (*itimer_set_t) (struct itimerval*);
typedef struct
{
    wait_t wait_func;
    itimer_set_t itimer_set;
    c_str name;
} thread_info_t;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;
volatile int signaled = 0;


void itimer_set_big(struct itimerval* itimer)
{
    itimer->it_interval.tv_sec  = 1 << 15;
    itimer->it_interval.tv_usec = 0;
    itimer->it_value.tv_sec = 1 << 15;
    itimer->it_value.tv_usec = 0;
}


void timespec_diff(const struct timespec* a,
                   const struct timespec* b,
                   struct timespec* result)
{
#define TIMESPEC_NANOSECOND_PER_SEC 1000000000

    /* verify that 'a' is larger */
    //assert((a->tv_sec > b->tv_sec) ||
    //((a->tv_sec == b->tv_sec) && (a->tv_nsec > b->tv_nsec)));

        

    /* compute difference */
    long nsec_diff = a->tv_nsec - b->tv_nsec;
    if (nsec_diff >= 0) {
        /* no need to borrow */
        result->tv_sec  = a->tv_sec - b->tv_sec;
        result->tv_nsec = nsec_diff;
    }
    else {
        /* need to borrow from sec */
        result->tv_sec  = a->tv_sec - b->tv_sec - 1;
        result->tv_nsec = nsec_diff + TIMESPEC_NANOSECOND_PER_SEC;
    }
}



void* thread_main(void* arg)
{

    thread_info_t* info = (thread_info_t*)arg;

    
#if 0

    /* On Linux, we have... */

    struct timespec {
        time_t  tv_sec;         /* seconds */
        long    tv_nsec;        /* nanoseconds */
    };

#endif
    
    clockid_t clockid;
    
    //clockid = CLOCK_THREAD_CPUTIME_ID;
    if (pthread_getcpuclockid(pthread_self(), &clockid)) {
        fprintf(stderr, "pthread_getcpuclockid() failed: %s\n",
              strerror(errno));
        abort();
    }
    fprintf(stderr, "Got clockid_t %d for %s\n",
          (int)clockid,
          info->name.data());
    

    struct timespec start;
    memset(&start, 0, sizeof(struct timespec));
    start.tv_sec  = 1 << 30;
    start.tv_nsec = 0;
    if (clock_settime(clockid, &start)) {
        fprintf(stderr, "clock_settime() failed: %s\n",
              strerror(errno));
        abort();
    }
    

    printf("%s timer set to %lu, %lu\n",
           info->name.data(),
           (unsigned long)start.tv_sec,
           (unsigned long)start.tv_nsec);

    
    info->wait_func();


    struct timespec end;
    memset(&end, 0, sizeof(struct timespec));
    if (clock_gettime(clockid, &end)) {
        fprintf(stderr, "clock_gettime() failed: %s\n",
              strerror(errno));
        abort();
    }


    printf("%s timer value is %lu, %lu\n",
           info->name.data(),
           (unsigned long)end.tv_sec,
           (unsigned long)end.tv_nsec);
    
    
    struct timespec result;
    timespec_diff(&end, &start, &result);
    printf("%s timer difference is %lu, %lu\n",
           info->name.data(),
           (unsigned long)result.tv_sec,
           (unsigned long)result.tv_nsec);


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


    clockid_t clockid;
    if (pthread_getcpuclockid(pthread_self(), &clockid)) {
        fprintf(stderr, "pthread_getcpuclockid() failed: %s\n",
              strerror(errno));
        abort();
    }
    fprintf(stderr, "Got clockid_t %d\n", (int)clockid);


    thread_info_t root_info   = { wait_keypress,
                                  itimer_set_max,
                                  c_str("root") };
    thread_info_t helper_info = { wait_busy,
                                  itimer_set_big,
                                  c_str("helper") };
  

    pthread_mutex_lock(&mutex);


    pthread_t helper;
    if (pthread_create(&helper, NULL, thread_main, &helper_info)) {
        fprintf(stderr, "pthread_create failed\n");
        return -1;
    }
  
  
    thread_main(&root_info);
  

    pthread_join(helper, NULL);
    fprintf(stderr, "Joined with helper!\n");
  

    return 0;
}
