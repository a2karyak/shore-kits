
/** @file randgen_util.cpp
 *
 *  @brief Implements some useful functions using randgen_t objects.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug None known.
 */
#include "util/randgen_util.h"   /* for prototypes */
#include <cassert>



/* definitions of exported functions */


void randgen_util_t::random_in_range(int* results, int num_results,
                                     int min_inclusive, int max_inclusive,
                                     randgen_t* randgenp)
{
  /* Make sure we have a space to search in. */
  assert(min_inclusive <= max_inclusive);

  /* Make sure space is big enough to satisfy request. */
  int range_size = max_inclusive - min_inclusive + 1;
  assert(range_size >= num_results);

  int range[range_size];
  for (int i = 0; i < range_size; i++)
    range[i] = i + min_inclusive;

  randgen_util_t::shuffle(range, range_size, randgenp);
  for(int i = 0; i < num_results; i++)
    results[i] = range[i];
}
