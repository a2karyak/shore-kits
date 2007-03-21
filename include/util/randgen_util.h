/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __RANDGEN_UTIL_H
#define __RANDGEN_UTIL_H

#include "util/randgen.h"


class randgen_util_t {

public:

    static int random_in_range(int min_inclusive, int max_inclusive,
                               randgen_t* randgenp) {
        /* Make sure we have a space to search in. */
        assert(min_inclusive <= max_inclusive);
        int range_size = max_inclusive - min_inclusive + 1;
        return min_inclusive + randgenp->rand(range_size);
    }
  
    
    template <class T>
    static void shuffle(T* vals, int n, randgen_t* randgenp) {

        assert(n > 0);
        for (int i = 0; i < n-1; i++) {
            /* Select an element to swap with element i. Do the selection from
               the elements after i in the array. */
            int swap_index = random_in_range(i+1, n-1, randgenp);
            T temp = vals[i];
            vals[i] = vals[swap_index];
            vals[swap_index] = temp;
        }
    }
    
    
    static void random_in_range(int* results, int num_results,
                                int min_inclusive, int max_inclusive,
                                randgen_t* randgenp);

};
    

#endif
