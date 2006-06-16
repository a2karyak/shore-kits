// -*- mode:C++ c-basic-offset:4 -*-

/** @file    : shuffle.h
 *  @brief   : Shuffling utility
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

# ifndef _SHUFFLE_UTIL_H
# define _SHUFFLE_UTIL_H

# include <stdlib.h>
# include <stdio.h>
# include <sys/time.h>
# include <unistd.h>

/* Exported Functions */

unsigned int init_srand(unsigned int thid);

void do_shuffle( int* deck, int size );

void print_array(int* deck, int size);


# endif // _SHUFFLE_UTIL_H
