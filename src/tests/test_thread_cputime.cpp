/* -*- mode:C++; c-basic-offset:4 -*- */

/**
 * @brief Unit test to test timers created using the
 * CLOCK_THREAD_CPUTIME_ID clockid_t. Basically, we have a busy wait
 * thread and a thread that sleeps. We see that both threads get back
 * different values from timer_gettime().
 */
#include "util.h"
#include <time.h>
#include <sys/time.h>



#ifndef _POSIX_THREAD_CPUTIME
//#warning No per-thread timers!
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
int signaled = 0;


void itimer_set_big(struct itimerval* itimer)
{
    itimer->it_interval.tv_sec  = 1 << 15;
    itimer->it_interval.tv_usec = 0;
    itimer->it_value.tv_sec = 1 << 15;
    itimer->it_value.tv_usec = 0;
}


void* thread_main(void* arg)
{

    thread_info_t* info = (thread_info_t*)arg;

    
    /* create timer */
    timer_t timerid;
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, NULL, &timerid)) {
        TRACE(TRACE_ALWAYS, "timer_create() failed\n");
        abort();
    }
    TRACE(TRACE_ALWAYS, "Created a timer with id %d\n", timerid);


#if 0

    /* On Linux, we have... */

    struct itimerspec {
        struct timespec it_interval;    /* timer period */
        struct timespec it_value;       /* timer expiration */
    };

    struct timespec {
        time_t  tv_sec;         /* seconds */
        long    tv_nsec;        /* nanoseconds */
    };

#endif
    
    struct itimerspec interval;
    memset(&interval, 0, sizeof(struct itimerspec));
    interval.it_value.tv_sec  = 1 << 30;
    interval.it_value.tv_nsec = 0;
    if (timer_settime(timerid, 0, &interval, NULL)) {
        TRACE(TRACE_ALWAYS, "timer_settime() failed\n");
        abort();
    }


    printf("%s Set timer %d to %ld, %ld\n",
           info->name.data(),
           timerid,
           interval.it_value.tv_sec,
           interval.it_value.tv_nsec);


    info->wait_func();


    struct itimerspec elapsed;
    memset(&elapsed, 0, sizeof(struct itimerspec));
    if (timer_gettime(timerid, &elapsed)) {
        TRACE(TRACE_ALWAYS, "timer_gettime() failed\n");
        abort();
    }


    printf("%s it_value of %d is %ld, %ld\n",
           info->name.data(),
           timerid,
           elapsed.it_value.tv_sec,
           elapsed.it_value.tv_nsec);

    printf("%s it_interval of %d is %ld, %ld\n",
           info->name.data(),
           timerid,
           elapsed.it_interval.tv_sec,
           elapsed.it_interval.tv_nsec);

    
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
        TRACE(TRACE_ALWAYS, "signaled = 0\n");
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
    getc(stdin);

    /* signal helper thread */
    signaled = 1; /* if helper using wait_busy */
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    TRACE(TRACE_ALWAYS, "signaled = %d\n", signaled);
}


int main(int, char**)
{
    util_init();


  thread_info_t root_info   = { wait_keypress,
                                itimer_set_max,
                                c_str("root") };
  thread_info_t helper_info = { wait_busy,
                                itimer_set_big,
                                c_str("helper") };
  

  pthread_mutex_lock(&mutex);


  pthread_t helper;
  if (pthread_create(&helper, NULL, thread_main, &helper_info)) {
      TRACE(TRACE_ALWAYS, "pthread_create failed\n");
      return -1;
  }
  
  
  thread_main(&root_info);
  

  pthread_join(helper, NULL);
  TRACE(TRACE_ALWAYS, "Joined with helper!\n");
  

  return 0;
}
