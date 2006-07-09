/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _DISPATCHER_OS_SUPPORT_H
#define _DISPATCHER_OS_SUPPORT_H

#ifdef  DISPATCHER_UNSUPPORTED_OS
#define DISPATCHER_UNSUPPORTED_OS 1
#endif



/* GNU Linux */
#if defined(linux) || defined(__linux)

/* detected GNU Linux */
#undef  DISPATCHER_UNSUPPORTED_OS
#define DISPATCHER_UNSUPPORTED_OS 0

#endif


/* Sun Solaris */
#if defined(sun) || defined(__sun)
#if defined(__SVR4) || defined(__svr4__)

/* detected Sun Solaris */
#undef  DISPATCHER_UNSUPPORTED_OS
#define DISPATCHER_UNSUPPORTED_OS 0

#endif
#endif



#if DISPATCHER_UNSUPPORTED_OS
#error "Unsupported operating system\n"
#endif



#endif
