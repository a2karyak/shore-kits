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


/** Helper macros */

// 'typeof' is a gnu extension
#define TYPEOF(TYPE, MEMBER) typeof(((TYPE *)0)->MEMBER)


#define SIZEOF(TYPE, MEMBER) sizeof(((TYPE *)0)->MEMBER)

// 'offsetof' is found in <stddef.h>
#define FIELD(SRC, TYPE, MEMBER) (((char*) (SRC)) + offsetof(TYPE, MEMBER))

#define GET_FIELD(DEST, SRC, TYPE, MEMBER) \
    memcpy(DEST, \
           FIELD(SRC, TYPE, MEMBER), \
           SIZEOF(TYPE, MEMBER))



/** Exported functions */

int bt_compare_fn_1_int(const Dbt* k1, 
                        const Dbt* k2);


int bt_compare_fn_2_int(const Dbt* k1, 
                        const Dbt* k2);


int bt_compare_fn_3_int(const Dbt* k1, 
                        const Dbt* k2);



int bt_compare_fn_4_int(const Dbt* k1, 
                        const Dbt* k2);


EXIT_NAMESPACE(workload);


#endif
