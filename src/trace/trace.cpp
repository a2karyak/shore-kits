/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file trace.cpp
 *
 *  @brief Tracing module. To add a new tracing type, please create a
 *  constant for the type in trace_types.h. Then register it in the
 *  switch() statement below. You may also want to add a macro for
 *  that tracing type in trace.h.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug None known.
 */
#include "trace.h"              /* for prototypes */
#include <cstdio>               /* for stdout, stderr */
#include <cstring>              /* for strlen() */
#include "trace/trace_stream.h" /* for trace_stream() */
#include "trace/trace_force.h"  /* for TRACE_FORCE() */



/* internal data structures */


/**
 *  @brief Bit vector representing the current set of messages to be
 *  printed. Should be set at runtime to turn different message types
 *  on and off. We initialize it here to enable all messages. That
 *  way, any messages we print during QPIPE startup will be printed.
 */
static unsigned int trace_current_setting = ~0u;



/* definitions of exported functions */


/**
 *  @brief Covert the specified message into a single string and
 *  process it.
 *
 *  @param trace_type The message type. This determines how we will
 *  process it. For example, TRACE_TYPE_DEBUG could mean that we print
 *  the message to the console.
 *
 *  @param filename The name of the file where the message is coming
 *  from.
 *
 *  @param line_num The line number in the file where the message is
 *  coming from.
 *
 *  @param function_name The name of the function where the message is
 *  coming from.
 *
 *  @param format The format string for this message. printf()
 *  syntax.
 *
 *  @param ... Optional arguments referenced by the format string.
 */
void trace_(unsigned int trace_type,
	    const char* filename, int line_num, const char* function_name,
	    char* format, ...)
{

    /* Print if any trace_type bits match bits in the current trace
       setting. */
    unsigned int do_trace = trace_current_setting & trace_type;
    if ( do_trace == 0 )
        return;

    
    va_list ap;
    va_start(ap, format);
  
    /* currently, we only support printing to streams */
    trace_stream(stdout, filename, line_num, function_name, format, ap);

    va_end(ap);
    return;
}



/**
 *  @brief Specify the set of trace types that are currently enabled.
 *
 *  @param trace_type_mask Bitmask used to specify the current set of
 *  trace types.
 */
void trace_set(unsigned int trace_type_mask) {

    /* avoid unnecessary synchronization here */
    /* should really only be called once at the beginning of the
       program */
    trace_current_setting = trace_type_mask;        
}
