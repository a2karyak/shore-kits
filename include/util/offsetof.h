/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef ___OFFSET_OF_H
#define ___OFFSET_OF_H


/**
 * -ngm-
 *
 * @brief This code seems to work when the macro is define in the .cpp
 * file, but not in and included .h file. For the moment, let's not
 * include it in include/util.h.
 */


/* The linker should find a nice home for this */
unsigned long offsetof_base;

#ifdef offsetof
#undef offsetof
#endif

#define offsetof(struct_name,field) \
  ( ((size_t)&(((struct_name*)(&offsetof_base))->field)) - (size_t)(&offsetof_base) )


#endif
