/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   countdown.cpp
 *
 *  @brief:  Implementation of atomic countdown class
 *
 *  @author: Ryan Johnson (ryanjohn)
 */


#include "util/countdown.h"


void countdown_t::reset(const int newcount)
{
    // previous usage should have finished
    // previous usage may succeeded (0) or failed (CD_ERROR)
    //    assert ((_state==0) || (_state==CD_ERROR)); 
    _state = newcount*CD_NUMBER;
}

bool countdown_t::post(bool is_error) 
{
    int old_value = *&_state;
    while (old_value >= 2*CD_NUMBER) {
        int new_value = (is_error? CD_ERROR : old_value-CD_NUMBER);
        int cur_value = atomic_cas_32(&_state, old_value, new_value);
        if (cur_value == old_value) {
            assert(is_error || new_value >= CD_NUMBER);
            return (is_error);
        }

        // try, try again
        old_value = cur_value;
    }

    // did some other thread report an error?
    if (old_value == CD_ERROR) {
        return (false);
    }
 
    // we're the last caller -- no more atomic ops needed
    assert(old_value == CD_NUMBER);
    _state = (is_error? CD_ERROR : 0);
    return (true);
}

int countdown_t::remaining() const 
{
    int old_value = *&_state;
    return ((old_value == CD_ERROR) ? -1 : old_value/CD_NUMBER); 
}

