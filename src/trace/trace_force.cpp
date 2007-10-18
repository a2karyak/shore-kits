/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file trace_force.cpp
 *
 *  @brief trace_force_() is no longer used since we started using
 *  synchronization for our output streams. It should be removed from
 *  the trace module soon.
 *
 *  @brief Exports trace_force() function. trace_force() should only
 *  be invoked by code within the tracing module. All code outside the
 *  trace module should invoke TRACE().
 */

#include "trace/trace_force.h" /* for prototypes */

#ifdef __SUNPRO_CC
#include <stdarg.h>  
#include <stdio.h>   
#include <string.h>
#else
#include <cstdarg>      /* for va_list */
#include <cstdio>       /* for stdout, stderr */
#include <cstring>      /* for strlen() */
#endif



/* internal constants */

#define FORCE_BUFFER_SIZE 256



/* definitions of exported functions */


void trace_force_(const char* filename, int line_num, const char* function_name,
		  char* format, ...)
{

  int function_name_len = strlen(function_name);
  int size = FORCE_BUFFER_SIZE + function_name_len;
  char buf[size];

  
  snprintf(buf, size, "\nFORCE: %s:%d %s: ", filename, line_num, function_name);
  int   msg_offset = strlen(buf);
  int   msg_size   = size - msg_offset;
  char* msg        = &buf[ msg_offset ];
  

  va_list ap;
  va_start(ap, format);
  vsnprintf(msg, msg_size, format, ap);
  fprintf(stderr, buf);
  va_end(ap);
}
