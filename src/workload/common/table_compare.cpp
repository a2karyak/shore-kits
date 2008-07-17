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


/** Helper functions */


/** Exported functions */

struct s_one_int {
    int s1;
};

struct s_two_ints {
    int s1;
    int s2;
};

struct s_three_ints {
    int s1;
    int s2;
    int s3;
};

struct s_four_ints {
    int s1;
    int s2;
    int s3;
    int s4;
};

struct s_six_ints {
    int s1;
    int s2;
    int s3;
    int s4;
    int s5;
    int s6;
};
    


/** @bt_compare_fn_1_int
 *
 *  @brief Comparator when the key is 1 int field
 */

int bt_compare_fn_1_int(const Dbt* k1, 
                        const Dbt* k2)
{
    // key has 1 integer
    s_one_int u1;
    s_one_int u2;
    memcpy(&u1, k1->get_data(), sizeof(u1));
    memcpy(&u2, k2->get_data(), sizeof(u2));

    TRACE ( TRACE_KEY_COMP, 
            "\nk1=(%d)\tk2=(%d)\n", 
            u1.s1, u2.s1);

    if (u1.s1 < u2.s1)
        return -1;
    else if (u1.s1 == u2.s1)
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
    s_two_ints u1;
    s_two_ints u2;
    memcpy(&u1, k1->get_data(), sizeof(u1));
    memcpy(&u2, k2->get_data(), sizeof(u2));

    TRACE ( TRACE_KEY_COMP, 
            "\nk1=(%d,%d)\tk2=(%d,%d)\n", 
            u1.s1, u1.s2, 
            u2.s1, u2.s2);

    if ((u1.s1 < u2.s1) || ((u1.s1 == u2.s1) && (u1.s2 < u2.s2)))
        return -1;
    else if ((u1.s1 == u2.s1) && (u1.s2 == u2.s2))
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
    s_three_ints u1;
    s_three_ints u2;

    memcpy(&u1, k1->get_data(), sizeof(u1));
    memcpy(&u2, k2->get_data(), sizeof(u2));

    TRACE ( TRACE_KEY_COMP, 
            "\nk1=(%d,%d,%d)\tk2=(%d,%d,%d)\n", 
            u1.s1, u1.s2, u1.s3, 
            u2.s1, u2.s2, u2.s3);
    
    if ((u1.s1 < u2.s1) || 
        ((u1.s1 == u2.s1) && (u1.s2 < u2.s2)) ||
        ((u1.s1 == u2.s1) && (u1.s2 == u2.s2) && (u1.s3 < u2.s3)))
        return -1;
    else if ((u1.s1 == u2.s1) && (u1.s2 == u2.s2) &&  (u1.s3 == u2.s3) )
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
    s_four_ints u1;
    s_four_ints u2;
    memcpy(&u1, k1->get_data(), sizeof(s_four_ints));
    memcpy(&u2, k2->get_data(), sizeof(s_four_ints));


    TRACE ( TRACE_KEY_COMP, 
            "\nk1=(%d,%d,%d,%d)\tk2=(%d,%d,%d,%d)\n", 
            u1.s1, u1.s2, u1.s3, u1.s4,
            u2.s1, u2.s2, u2.s3, u1.s4);
    
  
    if ((u1.s1 < u2.s1) || 
        ((u1.s1 == u2.s1) && (u1.s2 < u2.s2)) ||
        ((u1.s1 == u2.s1) && (u1.s2 == u2.s2) && (u1.s3 < u2.s3)) ||
        ((u1.s1 == u2.s1) && (u1.s2 == u2.s2) && (u1.s3 == u2.s3) && (u1.s4 < u2.s4)))
        return -1;
    else if ((u1.s1 == u2.s1) && 
             (u1.s2 == u2.s2) &&  
             (u1.s3 == u2.s3) &&  
             (u1.s4 == u2.s4))
        return 0;
    else
        return 1;
}



/** @bt_compare_fn_6_int
 *
 *  @brief Comparator when the key consists of the first 6 int fields
 */

int bt_compare_fn_6_int(const Dbt* k1, 
                        const Dbt* k2)
{
  // key has 6 integers
  s_six_ints u1;
  s_six_ints u2;
  memcpy(&u1, k1->get_data(), sizeof(s_six_ints));
  memcpy(&u2, k2->get_data(), sizeof(s_six_ints));
  
  if ((u1.s1 < u2.s1) || 
      ((u1.s1 == u2.s1) && (u1.s2 < u2.s2)) ||
      ((u1.s1 == u2.s1) && (u1.s2 == u2.s2) && (u1.s3 < u2.s3)) ||
      ((u1.s1 == u2.s1) && (u1.s2 == u2.s2) && 
       (u1.s3 == u2.s3) && (u1.s4 < u2.s4)) ||
      ((u1.s1 == u2.s1) && (u1.s2 == u2.s2) && 
       (u1.s3 == u2.s3) && (u1.s4 == u2.s4) && (u1.s5 < u2.s5)) ||
      ((u1.s1 == u2.s1) && (u1.s2 == u2.s2) && (u1.s3 == u2.s3) && 
       (u1.s4 == u2.s4) && (u1.s5 == u2.s5) && (u1.s6 < u2.s6)))
    return -1;
  else if ((u1.s1 == u2.s1) && 
           (u1.s2 == u2.s2) &&  
           (u1.s3 == u2.s3) &&  
           (u1.s4 == u2.s4) &&  
           (u1.s5 == u2.s5) &&  
           (u1.s6 == u2.s6))
    return 0;
  else
    return 1;
}



EXIT_NAMESPACE(workload);

