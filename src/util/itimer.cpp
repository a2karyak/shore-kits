/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file itimer.cpp
 *
 *  @brief Implements common itimer functions.
 */
#include "util/itimer.h" /* for prototypes */



/* definitions of exported functions */


c_str itimer_to_cstr(struct itimerval* itimer)
{
    return c_str("[INTERVAL(%ld,%ld) VALUE(%ld,%ld)]",
                 itimer->it_interval.tv_sec,
                 itimer->it_interval.tv_usec,
                 itimer->it_value.tv_sec,
                 itimer->it_value.tv_usec);
}

void itimer_set_max(struct itimerval* itimer)
{
    itimer->it_interval.tv_sec  = 1 << 30;
    itimer->it_interval.tv_usec = 0;
    itimer->it_value.tv_sec = 1 << 30;
    itimer->it_value.tv_usec = 0;
}
