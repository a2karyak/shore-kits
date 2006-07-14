/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file trace_force.h
 *
 *  @brief Exports TRACE_FORCE(). TRACE_FORCE() should only be invoked
 *  by code within the tracing module. All code outside the trace
 *  module should invoke TRACE().
 *
 *  @bug See trace_force.cpp.
 */
#ifndef _TRACE_FORCE_H
#define _TRACE_FORCE_H

#include <cstdarg> /* for varargs */



/* exported functions */

void trace_force_(const char* filename, int line_num, const char* function_name,
		  char* format, ...) __attribute__((format(printf, 4, 5)));;



/* exported macros */


/**
 *  @def TRACE_FORCE
 *
 * @brief Used by TRACE() to report an error.
 *
 * @param format The format string for the printing. Format follows
 * that of printf(3).
 *
 * @param rest Optional arguments that can printed (see printf(3)
 * definition for more details).
 *
 * @return void
 */

#define TRACE_FORCE(format, rest...) trace_force_(__FILE__, __LINE__, __FUNCTION__, format, ##rest)



#endif // _TRACE_FORCE_H
