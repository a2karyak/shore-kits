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
    TRACE( TRACE_DEBUG, // TRACE_TRX_FLOW,
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

ol_item_info& ol_item_info::operator= (const ol_item_info& rhs)
{
    _ol_i_id = rhs._ol_i_id;
    _ol_supply_wh_select = rhs._ol_supply_wh_select;
    _ol_supply_wh_id = rhs._ol_supply_wh_id;
    _ol_quantity = rhs._ol_quantity;

    return (*this);
}

new_order_input_t& new_order_input_t::operator= (const new_order_input_t& rhs) 
{
    // copy input
    _wh_id = rhs._wh_id;
    _d_id = rhs._d_id;
    _c_id = rhs._c_id;
    _ol_cnt = rhs._ol_cnt;
    _rbk = rhs._rbk;

    for (int i=0; i<rhs._ol_cnt; i++) {
        items[i] = rhs.items[i];
    }

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



/* -------------------------- */
/* --- ORDER_STATUS_INPUT --- */
/* -------------------------- */


order_status_input_t& order_status_input_t::operator= (const order_status_input_t& rhs) 
{
    _wh_id    = rhs._wh_id;
    _d_id     = rhs._d_id;
    _c_select = rhs._c_select;
    _c_id     = rhs._c_id;
    
    if (rhs._c_last) {
        store_string(_c_last, rhs._c_last);
    }

    return (*this);
}


void order_status_input_t::describe(int id)
{
    if (_c_select < 60) {
        // using C_LAST
        TRACE( TRACE_TRX_FLOW,
               "\nORDER_STATUS: ID=%d\n" \
               "WH=%d\t\tD=%d\tC_SEL=%d\tC_LAST=%s\n",
               id, _wh_id, _d_id, _c_select, _c_last);
    }
    else {
        // using C_ID
        TRACE( TRACE_TRX_FLOW,
               "\nORDER_STATUS: ID=%d\n" \
               "WH=%d\t\tD=%d\tC_SEL=%d\tC_ID=%d\n",
               id, _wh_id, _d_id, _c_select, _c_id);
    }
}


void order_status_input_t::gen_input(int sf)
{
    *this = create_order_status_input(sf);
}



/* ---------------------- */
/* --- DELIVERY_INPUT --- */
/* ---------------------- */


delivery_input_t& delivery_input_t::operator= (const delivery_input_t& rhs) 
{
    _wh_id      = rhs._wh_id;
    _carrier_id = rhs._carrier_id;

    return (*this);
}


void delivery_input_t::describe(int id)
{
    TRACE( TRACE_TRX_FLOW,
           "\nDELIVERY: ID=%d\nWH=%d\t\tCARRIER=%d\n",
           id, _wh_id, _carrier_id);
}


void delivery_input_t::gen_input(int sf)
{
    *this = create_delivery_input(sf);
}



/* ------------------------- */
/* --- STOCK_LEVEL_INPUT --- */
/* ------------------------- */


stock_level_input_t& stock_level_input_t::operator= (const stock_level_input_t& rhs) 
{
    _wh_id     = rhs._wh_id;
    _d_id      = rhs._d_id;
    _threshold = rhs._threshold;

    return (*this);
}


void stock_level_input_t::describe(int id)
{
    TRACE( TRACE_TRX_FLOW,
           "\nSTOCK_LEVEL: ID=%d\nWH=%d\t\tD=%d\tTHRESHOLD=%d\n",
           id, _wh_id, _d_id, _threshold);
}


void stock_level_input_t::gen_input(int sf)
{
    *this = create_stock_level_input(sf);
}

