#ifndef __COMPAT_H
#define __COMPAT_H

#ifdef __GCC

#define ATTRIBUTE(x) __attribute__((x))

#else

#define ATTRIBUTE(x)
#define __PRETTY_FUNCTION__ __func__
#define __FUNCTION__ __func__

#endif


#endif
