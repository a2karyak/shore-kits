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


struct tpcc_customer_tuple {
    int C_C_ID;
    int C_D_ID;
    int C_W_ID;
    char C_FIRST    [STRSIZE(17)];
    char C_MIDDLE   [STRSIZE(3)];
    char C_LAST     [STRSIZE(17)];
    char C_STREET_1 [STRSIZE(21)];
    char C_STREET_2 [STRSIZE(21)];
    char C_CITY     [STRSIZE(21)];
    char C_STATE    [STRSIZE(3)];
    char C_ZIP      [STRSIZE(10)];
    char C_PHONE    [STRSIZE(17)];
    int C_SINCE;
    char C_CREDIT   [STRSIZE(3)];
    decimal C_CREDIT_LIM;
    decimal C_DISCOUNT;
    decimal C_BALANCE;
    decimal C_YTD_PAYMENT;
    decimal C_LAST_PAYMENT;
    int C_PAYMENT_CNT;
    char C_DATA_1   [STRSIZE(251)];
    char C_DATA_2   [STRSIZE(251)];    
};




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
*/


EXIT_NAMESPACE(tpcc);

#endif
