
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

#define TRACE_ALWAYS      (1 << 0)
#define TRACE_TUPLE_FLOW  (1 << 1)
#define TRACE_PACKET_FLOW (1 << 2)

#define TRACE_SYNC_COND   (1 << 3)
#define TRACE_SYNC_LOCK   (1 << 4)
#define TRACE_DEBUG       (1 << 31)



#endif
