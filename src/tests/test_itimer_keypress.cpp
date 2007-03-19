/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include <sys/time.h>






int main(int, char**)
{
    util_init();

    /* create a virtual timer and wait for keypress */
    struct itimerval it_real_start, it_real_end;
    struct itimerval it_virt_start, it_virt_end;

    /* make interval very long */
    itimer_set_max(&it_real_start);
    itimer_set_max(&it_real_end);
    itimer_set_max(&it_virt_start);
    itimer_set_max(&it_virt_end);
  
    if (setitimer(ITIMER_REAL, &it_real_start, NULL)) {
        TRACE(TRACE_ALWAYS, "setitimer() failed\n");
        return -1;
    }
    if (setitimer(ITIMER_VIRTUAL, &it_virt_start, NULL)) {
        TRACE(TRACE_ALWAYS, "setitimer() failed\n");
        return -1;
    }

    /* wait for keypress */
    printf("Press a key when ready...\n");
    getc(stdin);

    if (getitimer(ITIMER_REAL, &it_real_end)) {
        TRACE(TRACE_ALWAYS, "getitimer() failed\n");
        return -1;
    }
  
    if (getitimer(ITIMER_VIRTUAL, &it_virt_end)) {
        TRACE(TRACE_ALWAYS, "getitimer() failed\n");
        return -1;
    }

    /* compute elapsed time */
    printf("Real timer start:    %s\n", itimer_to_cstr(&it_real_start).data());
    printf("Real timer end:      %s\n", itimer_to_cstr(&it_real_end).data());
    printf("Virtual timer start: %s\n", itimer_to_cstr(&it_virt_start).data());
    printf("Virtual timer end:   %s\n", itimer_to_cstr(&it_virt_end).data());

    return 0;
}
