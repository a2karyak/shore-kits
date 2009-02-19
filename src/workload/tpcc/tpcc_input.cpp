/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_input.cpp
 *
 *  @brief Implementation of the (common) inputs for the TPC-C trxs
 */

#include "workload/tpcc/tpcc_input.h"
#include "workload/tpcc/tpcc_trx_input.h"

using namespace tpcc;



/* ----------------------- */
/* --- NEW_ORDER_INPUT --- */
/* ----------------------- */

ol_item_info& 
ol_item_info::operator= (const ol_item_info& rhs)
{
    _ol_i_id = rhs._ol_i_id;
    _ol_supply_wh_select = rhs._ol_supply_wh_select;
    _ol_supply_wh_id = rhs._ol_supply_wh_id;
    _ol_quantity = rhs._ol_quantity;

    _item_amount = rhs._item_amount;
    _astock = rhs._astock;

    return (*this);
}

new_order_input_t& 
new_order_input_t::operator= (const new_order_input_t& rhs) 
{
    // copy input
    _wh_id = rhs._wh_id;
    _d_id = rhs._d_id;
    _c_id = rhs._c_id;
    _ol_cnt = rhs._ol_cnt;
    _rbk = rhs._rbk;

    _tstamp = rhs._tstamp;
    _all_local = rhs._all_local;
    _d_next_o_id = rhs._d_next_o_id;

    for (int i=0; i<rhs._ol_cnt; i++) {
        items[i] = rhs.items[i];
    }

    return (*this);
}



/* --------------------- */
/* --- PAYMENT_INPUT --- */
/* --------------------- */

payment_input_t& 
payment_input_t::operator= (const payment_input_t& rhs) 
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


/* -------------------------- */
/* --- ORDER_STATUS_INPUT --- */
/* -------------------------- */

order_status_input_t& 
order_status_input_t::operator= (const order_status_input_t& rhs) 
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


/* ---------------------- */
/* --- DELIVERY_INPUT --- */
/* ---------------------- */

delivery_input_t& 
delivery_input_t::operator= (const delivery_input_t& rhs) 
{
    _wh_id      = rhs._wh_id;
    _carrier_id = rhs._carrier_id;

    return (*this);
}



/* ------------------------- */
/* --- STOCK_LEVEL_INPUT --- */
/* ------------------------- */

stock_level_input_t& 
stock_level_input_t::operator= (const stock_level_input_t& rhs) 
{
    _wh_id     = rhs._wh_id;
    _d_id      = rhs._d_id;
    _threshold = rhs._threshold;

    return (*this);
}


/* ------------------------- */
/* --- MBENCH_WH_INPUT --- */
/* ------------------------- */

mbench_wh_input_t& 
mbench_wh_input_t::operator= (const mbench_wh_input_t& rhs) 
{
    _wh_id  = rhs._wh_id;
    _amount = rhs._amount;
    return (*this);
}



/* ------------------------- */
/* --- MBENCH_CUST_INPUT --- */
/* ------------------------- */

mbench_cust_input_t& 
mbench_cust_input_t::operator= (const mbench_cust_input_t& rhs) 
{
    _wh_id  = rhs._wh_id;
    _d_id   = rhs._d_id;
    _c_id   = rhs._c_id;
    _amount = rhs._amount;
    return (*this);
}


