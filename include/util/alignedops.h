/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file alignedops.h
 *
 *  @brief Exported aligned memory operations.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug See alignedops.cpp.
 */
#ifndef _ALIGNEDOPS_H
#define _ALIGNEDOPS_H

#include <stdlib.h>


/* exported functions */

void* aligned_alloc(void*  buf,
                    size_t buf_size, size_t align_size,
                    void** aligned_base);

#endif
