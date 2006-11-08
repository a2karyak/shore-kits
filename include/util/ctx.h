// -*- mode:C++; c-basic-offset:4 -*-
#ifndef __CTX_H
#define __CTX_H

#include <cstdlib>

/**
 * An opaque, architecture-dependent context handle
 */
struct ctx_handle;

/**
 * Creates a new context that will begin with a call to func(arg).
 *
 * @param sched context to return to for scheduling or at exit.
 * @param stack_size size of stack to allocate. Zero requests a
 * "reasonable" default.
 */
ctx_handle* ctx_create(void* (*func)(void*), void* arg,
                      ctx_handle** sched, size_t stack_size=0);

/**
 * Swaps to a different context.
 *
 * This function will return when this context is re-scheduled.
 *
 * @param from The handle to store the current context into. This
 * handle must be available to at least one other context and/or a
 * scheduler, or the current context will never run again.
 *
 * @param to The handle to restore context from. 
 */
void ctx_swap(ctx_handle** from, ctx_handle* to);


/**
 * @brief Yields this context and requests scheduling.
 *
 * There is no guarantee as to which context will run next, or how
 * long this one will remain descheduled.
 *
 * @param cur the handle to store the current context into.
 *
 * @see ctx_swap()
 */
void ctx_yield(ctx_handle** cur);


/**
 * Exits this context and returns to the scheduler.
 *
 * The context's return value can be retrieved by calling ctx_yield()
 * (not yet implemented) with a valid handle to it.
 */
void ctx_exit(void* rval) __attribute__((noreturn));

#endif
