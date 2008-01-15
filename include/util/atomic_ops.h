// -*- mode:c++; c-basic-offset:4 -*-

/** @file  atomic_ops.h 
 *
 *  @brief Provides typesafe atomic operations in a compiler- and machine-agnostic way
 */

// TODO: atomic xchg might be useful on x86(_64) arches


#ifndef __ATOMIC_OPS_H
#define __ATOMIC_OPS_H


#if defined(__sparcv9)
#include <atomic.h>
#else
// use compiler intrinsics
#endif

/** ATOMIC COMPARE-AND-SWAP **/
template<class T, int size=sizeof(T)>
struct atomic_cas_helper {
    T operator()(T volatile* target, T old_value, T new_value);
};

// this version handles *any* 32-bit type (including classes)
template<class T>
struct atomic_cas_helper<T, sizeof(uint32_t)> {
    T operator()(T volatile* target, T old_value, T new_value) {
	T result;
	union {
	    T* _t;
	    uint32_t* _i;
	} ov={&old_value}, nv={&new_value}, dest={&result};
	union { T volatile* _t; uint32_t volatile* _i; } x={target};
#if defined(__sparcv9)
	*dest._i = atomic_cas_32(x._i, *ov._i, *nv._i);
#else
	*dest._i = __sync_val_compare_and_swap(x._i, *ov._i, *nv._i);
#endif
	return *dest._t;
    }
};

// this version handles *any* 64-bit type (including classes)
template<class T>
struct atomic_cas_helper<T, sizeof(uint64_t)> {
    T operator()(T volatile* target, T old_value, T new_value) {
	T result;
	union {
	    T* _t;
	    uint64_t* _i;
	} ov={&old_value}, nv={&new_value}, dest={&result};
	union { T volatile* _t; uint64_t volatile* _i; } x={target};
#ifdef __sparcv9
	*dest._i = atomic_cas_64(x._i, *ov._i, *nv._i);
#else
	*dest._i = __sync_val_compare_and_swap(x._i, *ov._i, *nv._i);
#endif
	return *dest._t;
    }
};

template<class T>
T atomic_cas(T volatile* target, T old_value, T new_value) {
    return atomic_cas_helper<T>()(target, old_value, new_value);
}

/** ATOMIC SWAP **/
/*
  Notes:
  - sparc/solaris provides a library call
  - x86(_64) has direct support through the xchg instruction

  If performance of SWAP turns out to be an issue on either arch,
  it might be worth customizing our implementation for those
  cases. So far, however, SWAP is not a bottleneck.
*/
template<class T, int size=sizeof(T)>
struct atomic_swap_helper {
    T operator()(T volatile* target, T new_value);
};

// this version handles *any* 32-bit type (including classes)
template<class T>
struct atomic_swap_helper<T, sizeof(uint32_t)> {
    T operator()(T volatile* target, T new_value) {
	T result;
	union {
	    T* _t;
	    uint32_t* _i;
	} nv={&new_value}, dest={&result};
	union { T volatile* _t; uint32_t volatile* _i; } x={target};
#ifdef __sparcv9
	*dest._i = atomic_swap_32(x._i, *nv._i);
#else
	*dest._i = __sync_lock_test_and_set(x._i, *nv._i);
#endif
	return *dest._t;
    }
};

// this version handles *any* 64-bit type (including classes)
template<class T>
struct atomic_swap_helper<T, sizeof(uint64_t)> {
    T operator()(T volatile* target, T new_value) {
	T result;
	union {
	    T* _t;
	    uint64_t* _i;
	} nv={&new_value}, dest={&result};
	union { T volatile* _t; uint64_t volatile* _i; } x={target};
#ifdef __sparcv9
	*dest._i = atomic_swap_64(x._i, *nv._i);
#else
	*dest._i = __sync_lock_test_and_set(x._i, *nv._i);
#endif
	return *dest._t;
    }
};

template<class T>
T atomic_swap(T volatile* target, T new_value) {
    return atomic_swap_helper<T>()(target, new_value);
};

#endif
