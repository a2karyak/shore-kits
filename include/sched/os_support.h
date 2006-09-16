/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _OS_SUPPORT_H
#define _OS_SUPPORT_H

#ifdef  UNSUPPORTED_OS
#define UNSUPPORTED_OS 1
#endif



/* GNU Linux */
#if defined(linux) || defined(__linux)
#define FOUND_LINUX
/* detected GNU Linux */
#undef  UNSUPPORTED_OS
#define UNSUPPORTED_OS 0

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <sched.h>

#endif


/* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)
#define FOUND_SOLARIS

/* detected Sun Solaris */
#undef  UNSUPPORTED_OS
#define UNSUPPORTED_OS 0

/* detected Sun Solaris */
#include <sys/types.h>
#include <sys/processor.h>

#endif
#endif



#if UNSUPPORTED_OS
#error "Unsupported operating system\n"
#endif

typedef cpu_set_t os_cpu_set_t;


#endif
