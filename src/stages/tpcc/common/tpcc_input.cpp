/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_input.cpp
 *
 *  @brief Implementation of the (common) inputs for the TPC-C trxs
 */

#include "stages/tpcc/common/tpcc_input.h"
#include "stages/tpcc/common/tpcc_trx_input.h"

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
    TRACE( TRACE_TRX_FLOW,
           "\nPAYMENT: ID=%d\nWH_ID=%d\t\tD_ID=%d\n"  \
           "SEL_WH=%d\tSEL_IDENT=%d\n"  \
           "REM_WH_ID=%d\tREM_D_ID=%d\n"        \
           "C_ID=%d\tH_AMOUNT=%.2f\tH_DATE=%d\n",     \
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


void payment_input_t::gen_input(int sf)
{
    *this = create_payment_input(sf);
}



/* ----------------------- */
/* --- NEW_ORDER_INPUT --- */
/* ----------------------- */

new_order_input_t& new_order_input_t::operator= (const new_order_input_t& rhs) 
{
    assert (false); // (ip) Not implemented yet
    return (*this);
}


void new_order_input_t::describe(int id)
{
    TRACE( TRACE_TRX_FLOW,
           "\nNEW_ORDER: ID=%d\nWH_ID=%d\t\tD_ID=%d\nC_ID=%d\t\tOL_CNT=%d\n",
           id, _wh_id, _d_id, _c_id, _ol_cnt);

    for (int i=0; i < _ol_cnt; i++) {
    
        TRACE( TRACE_TRX_FLOW,
               "\nITEM_INFO: ID=%d\nWH_ID=%d\t\tQUANT=%d",
               id, items[i]._ol_supply_wh_id, items[i]._ol_quantity);
    }
}


void new_order_input_t::gen_input(int sf)
{
    *this = create_no_input(sf);
}



/* ----------------------- */
/* --- ORDERLINE_INPUT --- */
/* ----------------------- */


orderline_input_t& orderline_input_t::operator= (const orderline_input_t& rhs) 
{
    assert (false); // (ip) Not implemented yet
    return (*this);
}


void orderline_input_t::describe(int id)
{
    assert (false); // not implemented
}


void orderline_input_t::gen_input(int sf)
{
    assert (false); // not implemented
}



/* -------------------------- */
/* --- ORDER_STATUS_INPUT --- */
/* -------------------------- */


order_status_input_t& order_status_input_t::operator= (const order_status_input_t& rhs) 
{
    assert (false); // (ip) Not implemented yet
    return (*this);
}


void order_status_input_t::describe(int id)
{
    assert (false); // not implemented
}


void order_status_input_t::gen_input(int sf)
{
    assert (false); // not implemented
}

