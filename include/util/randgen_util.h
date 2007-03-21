/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __RANDGEN_UTIL_H
#define __RANDGEN_UTIL_H

#include "util/randgen.h"


class randgen_util_t {

public:

    template <class T>
    static void randgen_shuffle(T* vals, int n, randgen_t* randgenp) {

        assert(n > 0);
        for (int i = 0; i < n-1; i++) {
            /* Select an element to swap with element i. Do the selection from
               the elements after i in the array. */
            unsigned int num_choices = n-1 - i;
            unsigned int choice = randgenp->rand(num_choices);
            unsigned int swap_index = i+1 + choice;
            
            T temp = vals[i];
            vals[i] = vals[swap_index];
            vals[swap_index] = temp;
        }
    }
    
};
    

#endif
