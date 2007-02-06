
/** @file alloc_printf.h
 *
 *  @brief Exports alloc_printf(), a function that uses malloc(3) to
 *  allocate a string initialized with the specified format.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug See alloc_printf.c.
 */
#ifndef _ALLOC_PRINTF_H
#define _ALLOC_PRINTF_H

#include <stdlib.h> /* for size_t */
#include <stdarg.h> /* for variable number of arguments (...) */


/* exported functions */


char* alloc_printf(void* (*alloc) (size_t), void (*dealloc) (void*), const char* format, ...);


#endif


