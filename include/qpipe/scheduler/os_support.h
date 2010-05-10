/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _OS_SUPPORT_H
#define _OS_SUPPORT_H

#ifdef  UNSUPPORTED_OS
#define UNSUPPORTED_OS 1
#endif

/* Sun Solaris */
/* detected Sun Solaris */
#undef  UNSUPPORTED_OS
#define UNSUPPORTED_OS 0

/* detected Sun Solaris */
#include <sys/types.h>
#include <sys/processor.h>

#if UNSUPPORTED_OS
#error "Unsupported operating system\n"
#endif



#endif
