/* -*- mode:C++ c-basic-offset:4 -*- */

/** @file    : shuffle.h
 *  @brief   : Shuffling utility
 *  @version : 0.1
 *  @history :
 6/11/2006: Initial version
*/ 

#include "engine/util/shuffle.h"

unsigned int srand_seed;

#define RANDOM    rand_r(&srand_seed)


/** @fn    : init_random(unsigned int)
 *  @brief : Sets the random seed
 *  @return: uint - the random seed
 */

unsigned int init_srand(unsigned int thid) {

  struct timeval init_time;

  if ( gettimeofday( &init_time, NULL ) < 0 ) {
    fprintf(stderr, "Error: gettimeofday failed\n");
    return (0);
  }

  srand_seed = (unsigned int)init_time.tv_usec * thid;

  srand(srand_seed);

  return (srand_seed);
}


/** @fn    : do_shuffle(int*, int)
 *  @brief : Shuffles the contents of the array of integers
 */

void do_shuffle( int* deck, int size ) {

  int i, swap_index, tmp;
  unsigned int r;


  for (i = 0; i < size-1; i++) {

    // swap entry i with random entry
    r = (unsigned) RANDOM % (size - i);

    swap_index = i + r;

    tmp = deck[i];
    deck[i] = deck[swap_index];
    deck[swap_index] = tmp;
  }
}


/** @fn    : print_array(int*, int)
 *  @brief : Prints the contents of the specified array of integers
 */

void print_array(int* deck, int size) {

  if ( size < 1 )
    return;

  printf("%d", deck[0]);

  int i;
  for (i = 1; i < size; i++) {
    printf(" %d", deck[i]);
  }

  printf("\n");
}



