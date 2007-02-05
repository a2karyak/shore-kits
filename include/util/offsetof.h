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


#define ASSIGN_OFFSET_OF(dest,type,field) { \
   type t;                                  \
   size_t field_base  = (size_t)(&t.field); \
   size_t struct_base = (size_t)(&t);       \
   dest = field_base - struct_base;         \
}


/*
  #define offsetof(struct_name,field) { \
  ( ((size_t)&(((struct_name*)(&offsetof_base))->field)) - (size_t)(&offsetof_base) )
*/


#endif
