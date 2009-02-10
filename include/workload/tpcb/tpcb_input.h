/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcb_input.h
 *
 *  @brief Declaration of the (common) inputs for the TPC-C trxs
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCB_INPUT_H
#define __TPCB_INPUT_H

#include "util.h"


ENTER_NAMESPACE(tpcb);


/** Exported data structures */



/*********************************************************************
 * 
 * @class bstract trx_input_t
 *
 * @brief Base class for the Input of any transaction
 *
 *********************************************************************/

struct acct_update_input_t {
    int b_id;
    int t_id;
    int a_id;
    double delta;

    acct_update_input_t(int branches_queried, int specific_b_id=-1);
};

struct populate_db_input_t {
    int _sf;
    int _first_a_id;

    populate_db_input_t(int sf, int a_id) : _sf(sf), _first_a_id(a_id) { }
};


EXIT_NAMESPACE(tpcb);


#endif

