/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file trace_stream.cpp
 *
 *  @brief Implements trace_stream().
 *
 *  @bug None known.
 */
#include "engine/thread.h"             /* for pthread_mutex_t */
#include "trace/trace_stream.h"        /* for prototypes */
#include "trace/trace_force.h"         /* for TRACE_FORCE() */
#include "trace/trace_print_pthread.h" /* for trace_print_pthread() */



/* internal data structures */

static pthread_mutex_t stream_mutex = PTHREAD_MUTEX_INITIALIZER;



/* definitions of exported functions */


/**
 *  @brief Prints the entire trace message to the specified stream.
 *
 *  @param out_stream The stream to print to. We flush after the
 *  print. We also make one call to fprintf(), so the message should
 *  be printed atomically.
 *
 *  @param filename The filename that the message is coming from. Used
 *  in message prefix.
 *
 *  @param line_num The line number that the message is coming
 *  from. Used in message prefix.
 *
 *  @param function_name The name of the function where the message is
 *  coming from. Used in message prefix.
 *
 *  @param format The format of the output message (not counting the
 *  prefix).
 *
 *  @param ap The parameters for format.
 *
 *  @return void
 */

void trace_stream(FILE* out_stream,
		  const char* filename, int line_num, const char* function_name,
		  char* format, va_list ap)
{

    /* Any message we print should be prefixed by:
       .
       "<thread ID>: <filename>:<line num>:<function name>: "

       Even though individual fprintf() calls can be expected to execute
       atomically, there is no such guarantee across multiple calls. We
       use a mutex to synchronize access to the console.
    */
    thread_t* this_thread = thread_get_self();
    const c_str &thread_name = (this_thread == NULL) ? NULL : this_thread->get_thread_name();
  

    if ( pthread_mutex_lock( &stream_mutex ) )
    {
        TRACE_FORCE("pthread_mutex_lock() failed\n");
        return;
    }


    /* Try to print a meaningful (string) thread ID. If no ID is
       registered, just use pthread_t returned by pthread_self(). */
    if ( thread_name != NULL )
        fprintf(out_stream, "%s", thread_name.data());
    else
        trace_print_pthread(out_stream, pthread_self());


    fprintf(out_stream, ": %s:%d:%s: ", filename, line_num, function_name);
    vfprintf(out_stream, format, ap);
  
    if ( pthread_mutex_unlock( &stream_mutex ) )
    {
        TRACE_FORCE("pthread_mutex_unlock() failed\n");
        return;
    }


    /* No need to flush in a critical section. Worst-case, someone else
       prints between out print and our flush. Since fflush() is atomic
       with respect to fprintf(), we end up simply flushing someone
       else's data along with our own. */
    fflush(out_stream);
}
