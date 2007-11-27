// -*- mode:c++; c-basic-offset:4 -*-
#ifndef __ATOMIC_OPS_H
#define __ATOMIC_OPS_H

/* Provides typesafe atomic operations in a compiler- and machine-agnostic way */
    // TODO: atomic xchg might be useful on x86(_64) arches
#if defined(__SUNPRO_CC)
#include <atomic.h>
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
    // use compiler intrinsics
#else
#error Atomic compare-and-swap not supported on this target!
#endif

namespace atomic {

    /** ATOMIC COMPARE-AND-SWAP **/
#if defined(__SUNPRO_CC)
    template<class T, int size=sizeof(T)>
    struct cas_helper {
	T operator()(T volatile &x, T old_value, T new_value);
    };

    // this version handles *any* 32-bit type (including classes)
    template<class T>
    struct cas_helper<T, sizeof(uint32_t)> {
	T operator()(T volatile &x, T old_value, T new_value) {
	    T result;
	    union {
		T* _t;
		uint32_t* _i;
	    } ov={&old_value}, nv={&new_value}, dest={&result};
	    *dest._i = atomic_cas_32((uint32_t volatile*) &x, *ov._i, *nv._i);
	    return *dest._t;
	}
    };

    // this version handles *any* 64-bit type (including classes)
    template<class T>
    struct cas_helper<T, sizeof(uint64_t)> {
	T operator()(T volatile &x, T old_value, T new_value) {
	    T result;
	    union {
		T* _t;
		uint64_t* _i;
	    } ov={&old_value}, nv={&new_value}, dest={&result};
	    *dest._i = atomic_cas_64((uint64_t volatile*) &x, *ov._i, *nv._i);
	    return *dest._t;
	}
    };

    template<class T>
    T cas(T volatile &x, T old_value, T new_value) {
	return cas_helper<T>()(x, old_value, new_value);
    }
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
    template<class T>
    T cas(T volatile &x, T old_value, T new_value) {
	return __sync_val_compare_and_swap(&x, old_value, new_value);
    }
#endif

    /** ATOMIC SWAP **/
    /*
      Notes:
      - sparc/solaris provides a library call
      - x86(_64) has direct support through the xchg instruction

      If performance of SWAP turns out to be an issue on either arch,
      it might be worth customizing our implementation for those
      cases. So far, however, SWAP is not a bottleneck.
    */
#if defined( __SUNPRO_CC)
    template<class T, int size=sizeof(T)>
    struct swap_helper {
	T operator()(T volatile &x, T new_value);
    };

    // this version handles *any* 32-bit type (including classes)
    template<class T>
    struct swap_helper<T, sizeof(uint32_t)> {
	T operator()(T volatile &x, T new_value) {
	    T result;
	    union {
		T* _t;
		uint32_t* _i;
	    } nv={&new_value}, dest={&result};
	    *dest._i = atomic_swap_32((uint32_t volatile*) &x, *nv._i);
	    return *dest._t;
	}
    };

    // this version handles *any* 64-bit type (including classes)
    template<class T>
    struct swap_helper<T, sizeof(uint64_t)> {
	T operator()(T volatile &x, T new_value) {
	    T result;
	    union {
		T* _t;
		uint64_t* _i;
	    } nv={&new_value}, dest={&result};
	    *dest._i = atomic_swap_64((uint64_t volatile*) &x, *nv._i);
	    return *dest._t;
	}
    };

    template<class T>
    T swap(T volatile &x, T new_value) {
	return swap_helper<T>()(x, new_value);
    }
#else
    template<class T>
    T swap(T volatile &x, T nv) {
	T ov, cv;
	for(ov=x; (cv=cas(x, ov, nv)) != ov; ov=cv);
	return ov;
    }
#endif
}
#endif
