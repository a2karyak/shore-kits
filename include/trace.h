/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file trace.h
 *
 *  @brief Tracing module.
 *
 *  @bug See trace.cpp.
 */
#ifndef _TRACE_H
#define _TRACE_H

#include <cstdarg>             /* for varargs */
#include <stdint.h>            /* for uint32_t */
#include "trace/trace_types.h" /* export trace types */



/* exported functions */

void trace_(unsigned int trace_type,
	    const char* filename, int line_num, const char* function_name,
	    char* format, ...);

void trace_set(unsigned int trace_type_mask);



/* exported macros */


/**
 *  @def TRACE
 *
 * @brief Other modules in our program use this macro for
 * reporting. We can use preprocessor macros like __FILE__ and
 * __LINE__ to provide more information in the output messages. We can
 * also remove all messages at compile time by changing the
 * definition.
 *
 * @param type The type of the message. This is really a bit vector of
 * all times when this message should be printed. If the current debug
 * setting contains any one of these bits, we print the message.
 *
 * @param format The format string for the printing. Format follows
 * that of printf(3).
 *
 * @param rest Optional arguments that can printed (see printf(3)
 * definition for more details).
 *
 * @return void
 */
#define TRACE(type, format, rest...) trace_(type, __FILE__, __LINE__, __FUNCTION__, format, ##rest)



/**
 *  @def TRACE_SET
 *
 * @brief Macro wrapper for trace_set()
 * 
 * @param types Passed through as trace_type_mask parameter of
 * trace_set().
 *
 * @return void
 */
#define TRACE_SET(types) trace_set(types)



#endif // _TRACE_H
