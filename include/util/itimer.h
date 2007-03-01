/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file itimer.h
 *
 *  @brief Exports common itimer functions.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug See itimer.c.
 */
#ifndef _ITIMER_H
#define _ITIMER_H

#include "util/c_str.h"
#include <sys/time.h>


/* exported functions */

c_str itimer_to_cstr(struct itimerval* itimer);
void  itimer_set_max(struct itimerval* itimer);


#endif
