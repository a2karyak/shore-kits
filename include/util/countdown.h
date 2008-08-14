/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   countdown.h
 *
 *  @brief:  Atomic countdown class
 *
 *  @author: Ryan Johnson (ryanjohn)
 *
 */

#ifndef __UTIL_COUNTDOWN_H
#define __UTIL_COUNTDOWN_H

#include <cassert>
#include <atomic.h>

struct countdown_t 
{
public:

    // constructor - sets the number to count down
    countdown_t(int count);

    // reduce thread count by one (or post an error) and return true if
    // the operation completed the countdown
    bool post(bool is_error=false);

    // return remaining threads or -1 if error
    int remaining() const;
 
private:

    enum { ERROR=0x1, NUMBER=0x2 };
    unsigned volatile _state;

    // copying not allowed
    countdown_t(countdown_t const &);
    void operator=(countdown_t const &);

}; // EOF: struct countdown_t


typedef countdown_t* countdownPtr;


#endif /* __UTIL_COUNTDOWN_H */
