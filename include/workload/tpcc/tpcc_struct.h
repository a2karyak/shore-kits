/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_struct.h
 *
 *  @brief Exports structures that we store/load from BerkeleyDB
 *         for the TPC-C database
 *
 *  @author Ippokratis Pandis (ipandis)
 *
 *  Comment field:
 *
 */

#ifndef __TPCC_STRUCT_H
#define __TPCC_STRUCT_H

#include <cstdlib>
#include <unistd.h>
#include <sys/time.h>
#include "util.h"


ENTER_NAMESPACE(tpcc);


/* use this for allocation of NUL-terminated strings */

#define STRSIZE(x)(x+1)

// helper macro to calculate field offsets
// DEPRECATED: use the equivalent standard lib offsetof() macro instead
#define FIELD_OFFSET(type, field) offsetof(type, field)



/* exported structures */

/*
enum tpch_l_shipmode {
    REG_AIR,
    AIR,
    RAIL,
    TRUCK,
    MAIL,
    FOB,
    SHIP,
    END_SHIPMODE
};

struct tpch_customer_tuple {
    decimal C_ACCTBAL;
    int C_CUSTKEY;
    int C_NATIONKEY;
    char C_NAME      [STRSIZE(25)];
    char C_ADDRESS   [STRSIZE(40)];
    char C_PHONE     [STRSIZE(15)];
    char C_MKTSEGMENT[STRSIZE(10)];
    char C_COMMENT   [STRSIZE(117)];
};
*/


EXIT_NAMESPACE(tpcc);

#endif
