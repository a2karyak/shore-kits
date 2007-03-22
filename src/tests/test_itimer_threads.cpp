/* -*- mode:C++; c-basic-offset:4 -*- */

/**
 * @brief Unit test to see if individual threads get their own itimer
 * structures. It appears that they don't the thread that invokes
 * setitimer() last ends up setting the timer of the other threads as
 * well.
 */

#include "util.h"
#include <sys/time.h>


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


extern "C" void* thread_main(void* arg)
{

    thread_info_t* info = (thread_info_t*)arg;


    struct itimerval it_real_start, it_real_end;
    struct itimerval it_virt_start, it_virt_end;
    struct itimerval it_prof_start, it_prof_end;
    info->itimer_set(&it_real_start); 
    info->itimer_set(&it_virt_start); 
    info->itimer_set(&it_prof_start); 


    /* set up this thread's timers */
    if (setitimer(ITIMER_REAL, &it_real_start, NULL)) {
        TRACE(TRACE_ALWAYS, "setitimer() failed\n");
        return NULL;
    }
    if (setitimer(ITIMER_VIRTUAL, &it_virt_start, NULL)) {
        TRACE(TRACE_ALWAYS, "setitimer() failed\n");
        return NULL;
    }
    if (setitimer(ITIMER_PROF, &it_prof_start, NULL)) {
        TRACE(TRACE_ALWAYS, "setitimer() failed\n");
        return NULL;
    }


    info->wait_func();


    if (getitimer(ITIMER_REAL, &it_real_end)) {
        TRACE(TRACE_ALWAYS, "getitimer() failed\n");
        return NULL;
    }
    if (getitimer(ITIMER_VIRTUAL, &it_virt_end)) {
        TRACE(TRACE_ALWAYS, "getitimer() failed\n");
        return NULL;
    }
    if (getitimer(ITIMER_PROF, &it_prof_end)) {
        TRACE(TRACE_ALWAYS, "getitimer() failed\n");
        return NULL;
    }


    /* compute elapsed time */
    printf("%s Real timer start:    %s\n",
           info->name.data(),
           itimer_to_cstr(&it_real_start).data());
    printf("%s Real timer end:      %s\n",
           info->name.data(),
           itimer_to_cstr(&it_real_end).data());
    printf("%s Virtual timer start: %s\n",
           info->name.data(),
           itimer_to_cstr(&it_virt_start).data());
    printf("%s Virtual timer end:   %s\n",
           info->name.data(),
           itimer_to_cstr(&it_virt_end).data());
    printf("%s Profile timer start: %s\n",
           info->name.data(),
           itimer_to_cstr(&it_prof_start).data());
    printf("%s Profile timer end:   %s\n",
           info->name.data(),
           itimer_to_cstr(&it_prof_end).data());
    
    return NULL;
}


void wait_cond(void) {
    /* This is ugly... the helper threads releases the mutex even
       though it did not lock it! */
    /* wait for signal from root thread */
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
}


void wait_keypress(void) {

    /* wait for helper thread to record time */
    pthread_mutex_lock(&mutex);

    /* wait for keypress */
    printf("Press a key when ready...\n");
    getc(stdin);

    /* signal helper thread */
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}


int main(int, char**)
{
    util_init();


    thread_info_t root_info   = { wait_keypress,
                                  itimer_set_max,
                                  c_str("root") };
    thread_info_t helper_info = { wait_cond,
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
