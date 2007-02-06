
/** @file alloc_printf.c
 *
 *  @brief Implements alloc_printf(), a function that uses a specified
 *  allocator to create a string initialized with the specified
 *  format.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug None known.
 */
#include "util/alloc_printf.h" /* for prototypes */
#include <stdio.h>        /* for vsnprintf() */
#include <stdlib.h>       /* for NULL, malloc, free() */
#include <string.h>       /* for memcpy() */



/* internal constants */


/** @def INITIAL_MESSAGE_LENGTH_GUESS
 *
 *  @brief We initially allocate a buffer of this many characters and
 *  grow it as necessary.
 */
#define INITIAL_MESSAGE_LENGTH_GUESS 64



/* definitions of exported functions */


/**
 *  @brief Allocate a string using the specified allocator, initialize
 *  it using the specified format and optional parameters, and return
 *  it. The returned string should be deallocated when it is no longer
 *  needed.
 *
 *  @param alloc The allocator. If NULL, malloc() will be used.
 *
 *  @param dealloc The de-allocator. If NULL, free() will be used.
 *
 *  @param format The format (printf() syntax).
 *
 *  @param ... Optional parameters for the format.
 *
 *  @return NULL on error. The specified string on success. This
 *  string should be released with free() when it is no longer
 *  needed.
 */
char* alloc_printf(void* (*alloc) (size_t),
		   void  (*dealloc) (void*),
		   const char* format, ...)
{
  
  if (alloc == NULL)
    alloc = malloc;
  if (dealloc == NULL)
    dealloc = free;

  
  /* This code is adapted from sample code in the man page for
     vsnprintf(). */
  int n, size = INITIAL_MESSAGE_LENGTH_GUESS;
  char *p, *np;
  int old_size = size;

  va_list ap;

  if ((p = (char*)alloc(size)) == NULL)
  {
    /* alloc() failed! */
    return NULL;
  }
  while (1)
  {

    /* try to print in the allocated space */
    va_start(ap, format);
    n = vsnprintf(p, size, format, ap);
    va_end(ap);


    /* if that worked, return the string. */
    if (n > -1 && n < size)
    {
      break; /* the string fits! */
    }
    /* otherwise, try again with more space. */
    old_size = size;
    if (n > -1)    /* glibc 2.1 */
      size = n+1;  /* precisely what is needed */
    else           /* glibc 2.0 */
      size *= 2;   /* twice the old size */
    
    
    np = (char*)alloc(size);
    if (np == NULL)
    {
      dealloc(p);
      return NULL;
    }
    else
    {
      memcpy(np, p, old_size);
      dealloc(p);
      p = np;
      /* continue with larger space */
    }
  }

  
  /* done! */
  return p;
}
