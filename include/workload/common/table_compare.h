/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file table_compare.h
 *
 *  @brief Interface of the common functionality for the B-trees comparators
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TABLE_COMPARE_H
#define __TABLE_COMPARE_H

#include <cstdlib>
#include "workload/common/bdb_env.h"


ENTER_NAMESPACE(workload);


/** Exported functions */

int bt_compare_fn_1_int(const Dbt* k1, 
                        const Dbt* k2);


int bt_compare_fn_2_int(const Dbt* k1, 
                        const Dbt* k2);


int bt_compare_fn_3_int(const Dbt* k1, 
                        const Dbt* k2);



int bt_compare_fn_4_int(const Dbt* k1, 
                        const Dbt* k2);


int bt_compare_fn_6_int(const Dbt* k1, 
                        const Dbt* k2);


EXIT_NAMESPACE(workload);


#endif
