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
#include "util/compat.h"


/* exported constants */

#define TRACE_ALWAYS             (1 << 0)

#define TRACE_TUPLE_FLOW         (1 << 1)
#define TRACE_PACKET_FLOW        (1 << 2)

#define TRACE_SYNC_COND          (1 << 3)
#define TRACE_SYNC_LOCK          (1 << 4)

#define TRACE_THREAD_LIFE_CYCLE  (1 << 5)

#define TRACE_TEMP_FILE          (1 << 6)
#define TRACE_CPU_BINDING        (1 << 7)
#define TRACE_QUERY_RESULTS      (1 << 8)

#define TRACE_STATISTICS         (1 << 9)

#define TRACE_NETWORK            (1 << 10)

#define TRACE_DEBUG              (1u << 31)



/* exported functions */

struct tracer {
    char const* _file;
    int _line;
    char const* _function;
    tracer(char const* file, int line, char const* function)
	: _file(file), _line(line), _function(function)
    {
    }
    void operator()(unsigned int type, char const* format, ...) ATTRIBUTE(format(printf, 3, 4));
};

void trace_(unsigned int trace_type,
	    const char* filename, int line_num, const char* function_name,
	    char const* format, ...) ;
void trace_set(unsigned int trace_type_mask);
unsigned int trace_get();



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
#define TRACE tracer(__FILE__, __LINE__, __FUNCTION__)



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



/**
 *  @def TRACE_GET
 *
 * @brief Macro wrapper for trace_get()
 * 
 * @param types Passed through as trace_type_mask parameter of
 * trace_get().
 *
 * @return void
 */
#define TRACE_GET(types) trace_get(types)



#endif // _TRACE_H
