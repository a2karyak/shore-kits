/* -*- mode:C++; c-basic-offset:4 -*-
   Shore-kits -- Benchmark implementations for Shore-MT
   
   Copyright (c) 2007-2009
   Data Intensive Applications and Systems Labaratory (DIAS)
   Ecole Polytechnique Federale de Lausanne
   
   All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpch_input.cpp
 *
 *  @brief Implementation of the (common) inputs for the TPC-H trxs
 *  @brief Generate inputs for the TPCH TRXs
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "util.h"
#include "workload/tpch/tpch_random.h" 
#include "workload/tpch/tpch_input.h"

#include "workload/tpch/dbgen/dss.h"
#include "workload/tpch/dbgen/dsstypes.h"

using namespace dbgentpch; // for random MODES etc..

ENTER_NAMESPACE(tpch);



/******************************************************************** 
 *
 *  Q1
 *
 ********************************************************************/


q1_input_t& 
q1_input_t::operator=(const q1_input_t& rhs)
{
    l_shipdate = rhs.l_shipdate;
    return (*this);
};


q1_input_t  
create_q1_input(const double sf, 
                const int specificWH)
{
    q1_input_t q1_input;

    struct tm shipdate;

    shipdate.tm_year = 1998;
    shipdate.tm_mon = 12;
    shipdate.tm_mday = URand(60, 120);
    shipdate.tm_hour = 0;
    shipdate.tm_min = 0;
    shipdate.tm_sec = 0;

    // The format: YYYY-MM-DD
    
    q1_input.l_shipdate = mktime(&shipdate);

    return (q1_input);
};



/******************************************************************** 
 *
 *  Q6
 *
 ********************************************************************/


q6_input_t& 
q6_input_t::operator=(const q6_input_t& rhs)
{
    l_shipdate = rhs.l_shipdate;
    l_quantity = rhs.l_quantity;
    l_discount = rhs.l_discount;
    return (*this);
};


q6_input_t    
create_q6_input(const double sf, 
                const int specificWH)
{
    q6_input_t q6_input;

    struct tm shipdate;

    //random() % 5 + 1993;
    shipdate.tm_year = URand(1993, 1997);

    shipdate.tm_mon = 1;
    shipdate.tm_mday = 1;
    shipdate.tm_hour = 0;
    shipdate.tm_min = 0;
    shipdate.tm_sec = 0;
    q6_input.l_shipdate = mktime(&shipdate);

    //random() % 2 + 24;
    q6_input.l_quantity = URand(24, 25);

    //random() % 8 + 2
    q6_input.l_discount = ((double)(URand(2,9))) / (double)100.0;
	
    return (q6_input);
};



/******************************************************************** 
 *
 *  Q12
 *
 *  l_shipmode1:   Random within the list of MODES defined in 
 *                 Clause 5.2.2.13 (tpc-h-2.8.0 pp.91)
 *  l_shipmode2:   Random within the list of MODES defined in
 *                 Clause 5.2.2.13 (tpc-h-2.8.0 pp.91)
 *                 and different from l_shipmode1
 *  l_receiptdate: First of January of a random year within [1993 .. 1997]
 *
 ********************************************************************/


q12_input_t& 
q12_input_t::operator=(const q12_input_t& rhs)
{    
    strncpy(l_shipmode1,rhs.l_shipmode1,11);
    strncpy(l_shipmode2,rhs.l_shipmode2,11);
    l_receiptdate = rhs.l_receiptdate;
    return (*this);
};


q12_input_t    
create_q12_input(const double sf, 
                 const int specificWH)
{
    q12_input_t q12_input;

    pick_str(&l_smode_set, L_SMODE_SD, q12_input.l_shipmode1);
    pick_str(&l_smode_set, L_SMODE_SD, q12_input.l_shipmode2);

    struct tm receiptdate;

    // Random year [1993 .. 1997]
    receiptdate.tm_year = URand(1993, 1997);

    // First of January
    receiptdate.tm_mon = 1;
    receiptdate.tm_mday = 1;
    receiptdate.tm_hour = 0;
    receiptdate.tm_min = 0;
    receiptdate.tm_sec = 0;

    q12_input.l_receiptdate = mktime(&receiptdate);
	
    return (q12_input);
};



/******************************************************************** 
 *
 *  Q14
 *
 *  l_shipdate: The first day of a month randomly selected from a
 *              random year within [1993 .. 1997]
 *
 ********************************************************************/


q14_input_t& 
q14_input_t::operator=(const q14_input_t& rhs)
{
    l_shipdate = rhs.l_shipdate;
    return (*this);
};


q14_input_t    
create_q14_input(const double sf, 
                 const int specificWH)
{
    q14_input_t q14_input;

    struct tm shipdate;

    // Random year within [1993 .. 1997]
    shipdate.tm_year = URand(1993, 1997);

    // Random month ([1 .. 12])
    shipdate.tm_mon = URand(1,12);

    // First day
    shipdate.tm_mday = 1;
    shipdate.tm_hour = 0;
    shipdate.tm_min = 0;
    shipdate.tm_sec = 0;

    q14_input.l_shipdate = mktime(&shipdate);
	
    return (q14_input);
};



EXIT_NAMESPACE(tpch);
