/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __RANDGEN_H
#define __RANDGEN_H

#ifdef __GCC
#include <cstdlib>
#else
#include <stdlib.h> /* On Sun's CC <stdlib.h> defines rand_r,
                       <cstdlib> doesn't */
#endif


class randgen_t {

    unsigned int _seed;
    
public:

    randgen_t() {
        reset(0);
    }
    
    randgen_t(unsigned int seed) {
        reset(seed);
    }

    randgen_t(void* addr) {
        // losses the top 4 bytes
        reset((unsigned int)((long)addr));
    }

    void reset(unsigned int seed) {
        _seed = seed;
    }
    
    int rand() {
        return rand_r(&_seed);
    }
    
    /**
     * Returns a pseudorandom, uniformly distributed int value between
     * 0 (inclusive) and the specified value (exclusive).
     *
     * Source http://java.sun.com/j2se/1.5.0/docs/api/java/util/Random.html#nextInt(int)
     */
    int rand(int n) {
        assert(n > 0);

        if ((n & -n) == n)  // i.e., n is a power of 2
            return (int)((n * (uint64_t)rand()) / (RAND_MAX+1));

        int bits, val;
        do {
            bits = rand();
            val = bits % n;
        } while(bits - val + (n-1) < 0);
        
        return val;
    }
    
};


#endif
