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

void* aligned_alloc(size_t min_buf_size, size_t align_size,
                    void** aligned_base);

#endif
