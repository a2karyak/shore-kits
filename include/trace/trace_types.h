/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file trace_types.h
 *
 *  @brief Lists all TRACE types. Each of these should be a bit
 *  specified in a bit vector. We current support up to 32 tracing
 *  types.
 *
 *  @bug None known.
 */
#ifndef _TRACE_TYPES_H
#define _TRACE_TYPES_H



/* exported constants */

#define TRACE_COMPONENT_MASK_ALL       (~0)
#define TRACE_COMPONENT_MASK_NONE        0

#define TRACE_ALWAYS               (1 << 0)
#define TRACE_TUPLE_FLOW           (1 << 1)
#define TRACE_PACKET_FLOW          (1 << 2)
#define TRACE_SYNC_COND            (1 << 3)
#define TRACE_SYNC_LOCK            (1 << 4)
#define TRACE_THREAD_LIFE_CYCLE    (1 << 5)
#define TRACE_TEMP_FILE            (1 << 6)
#define TRACE_CPU_BINDING          (1 << 7)
#define TRACE_QUERY_RESULTS        (1 << 8)
#define TRACE_STATISTICS           (1 << 9)
#define TRACE_NETWORK              (1 << 10)
#define TRACE_RESPONSE_TIME        (1 << 11)
#define TRACE_WORK_SHARING         (1 << 12)
#define TRACE_DEBUG                (1 << 31)



#endif
