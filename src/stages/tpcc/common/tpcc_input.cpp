/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_input.cpp
 *
 *  @brief Implementation of the (common) inputs for the TPC-C trxs
 */

#include "stages/tpcc/common/tpcc_input.h"

using namespace tpcc;


/* --------------------- */
/* --- PAYMENT_INPUT --- */
/* --------------------- */


// Assignment operator
payment_input_t& payment_input_t::operator= (const payment_input_t& rhs) 
{
    // copy input
    _home_wh_id = rhs._home_wh_id;
    _home_d_id = rhs._home_d_id;
    _v_cust_wh_selection = rhs._v_cust_wh_selection;
    _remote_wh_id = rhs._remote_wh_id;
    _remote_d_id = rhs._remote_d_id;
    _v_cust_ident_selection = rhs._v_cust_ident_selection;
    _c_id = rhs._c_id;
    _h_amount = rhs._h_amount;

    if (rhs._c_last) {
        store_string(_c_last, rhs._c_last);
    }        

    return (*this);
}


void payment_input_t::describe(int id)
{
    if (_c_last) {

        TRACE( TRACE_TRX_FLOW,
               "\nPAYMENT: ID=%d\nWH_ID=%d\t\tD_ID=%d\n"  \
               "SEL_WH=%d\tSEL_IDENT=%d\n"  \
               "REM_WH_ID=%d\tREM_D_ID=%d\n"        \
               "C_ID=%d\tC_LAST=%s\n"               \
               "H_AMOUNT=%.2f\tH_DATE=%d\n",        \
               id,
               _home_wh_id, 
               _home_d_id, 
               _v_cust_wh_selection, 
               _v_cust_ident_selection,
               _remote_wh_id, 
               _remote_d_id,
               _c_id, 
               _c_last,
               _h_amount, 
               _h_date);
    }
    else {
            
        // If using the standard input, _c_last is NULL
        TRACE( TRACE_TRX_FLOW,                   
               "\nPAYMENT: ID=%d\nWH_ID=%d\t\tD_ID=%d\n"      \
               "SEL_WH=%d\tSEL_IDENT=%d\n"  \
               "REM_WH_ID=%d\tREM_D_ID=%d\n"      \
               "C_ID=%d\tH_AMOUNT=%.2f\tH_DATE=%d\n",       \
               id,
               _home_wh_id, 
               _home_d_id, 
               _v_cust_wh_selection, 
               _v_cust_ident_selection,
               _remote_wh_id, 
               _remote_d_id,
               _c_id, 
               _h_amount, 
               _h_date);
    }
}




/* ----------------------- */
/* --- NEW_ORDER_INPUT --- */
/* ----------------------- */


void new_order_input_t::describe(int id) 
{
    assert (false); // (ip) Not implemented yet
}



/* ----------------------- */
/* --- ORDERLINE_INPUT --- */
/* ----------------------- */


void orderline_input_t::describe(int id) 
{
    assert (false); // (ip) Not implemented yet
}

