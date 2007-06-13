/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file table_compare.cpp
 *
 *  @brief Implementation of the common functionality for the B-trees 
 *  comparators
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "workload/common/table_compare.h"


ENTER_NAMESPACE(workload);


/** @bt_compare_fn_1_int
 *
 *  @brief Comparator when the key is 1 int field
 */

int bt_compare_fn_1_int(const Dbt* k1, 
                        const Dbt* k2)
{
    int u1;
    int u2;
    memcpy(&u1,((int*)k1->get_data()), sizeof(int));
    memcpy(&u2,((int*)k2->get_data()), sizeof(int));

    if (u1 < u2)
        return -1;
    else if (u1 == u2)
        return 0;
    else
        return 1;
}



/** @bt_compare_fn_2_int
 *
 *  @brief Comparator when the key consists of the first 2 int fields
 */

int bt_compare_fn_2_int(const Dbt* k1, 
                        const Dbt* k2)
{
    // key has 2 integers
    int u1[2];
    int u2[2];
    memcpy(u1, k1->get_data(), 2 * sizeof(int));
    memcpy(u2, k2->get_data(), 2 * sizeof(int));

    if ((u1[0] < u2[0]) || ((u1[0] == u2[0]) && (u1[1] < u2[1])))
        return -1;
    else if ((u1[0] == u2[0]) && (u1[1] == u2[1]))
        return 0;
    else
        return 1;
}




/** @bt_compare_fn_3_int
 *
 *  @brief Comparator when the key consists of the first 3 int fields
 */

int bt_compare_fn_3_int(const Dbt* k1, 
                        const Dbt* k2)
{
  // key has 3 integers
  int u1[3];
  int u2[3];
  memcpy(u1, k1->get_data(), 3 * sizeof(int));
  memcpy(u2, k2->get_data(), 3 * sizeof(int));
  
  if ((u1[0] < u2[0]) || 
      ((u1[0] == u2[0]) && (u1[1] < u2[1])) ||
      ((u1[0] == u2[0]) && (u1[1] == u2[1]) && (u1[2] < u2[2])))
    return -1;
  else if ((u1[0] == u2[0]) && (u1[1] == u2[1]) &&  (u1[2] == u2[2]) )
    return 0;
  else
    return 1;
}



/** @bt_compare_fn_4_int
 *
 *  @brief Comparator when the key consists of the first 4 int fields
 */

int bt_compare_fn_4_int(const Dbt* k1, 
                        const Dbt* k2)
{
  // key has 4 integers
  int u1[4];
  int u2[4];
  memcpy(u1, k1->get_data(), 4 * sizeof(int));
  memcpy(u2, k2->get_data(), 4 * sizeof(int));
  
  if ((u1[0] < u2[0]) || 
      ((u1[0] == u2[0]) && (u1[1] < u2[1])) ||
      ((u1[0] == u2[0]) && (u1[1] == u2[1]) && (u1[2] < u2[2])) ||
      ((u1[0] == u2[0]) && (u1[1] == u2[1]) && (u1[2] == u2[2]) && (u1[3] < u2[3])))
    return -1;
  else if ((u1[0] == u2[0]) && 
           (u1[1] == u2[1]) &&  
           (u1[2] == u2[2]) &&  
           (u1[3] == u2[3]))
    return 0;
  else
    return 1;
}



EXIT_NAMESPACE(workload);


