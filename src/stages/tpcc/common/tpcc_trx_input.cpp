/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_trx_input.cpp
 *
 *  @brief Generate inputs for the TPCC TRXs
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util.h"
#include "stages/tpcc/common/tpcc_random.h" 
#include "stages/tpcc/common/tpcc_trx_input.h"


ENTER_NAMESPACE(tpcc);


// uncomment line below to use produce the same input
#define USE_SAME_INPUT


/********************************************************************* 
 *
 *  @fn:    create_no_input
 *
 *  @brief: Creates the input for a new NEW_ORDER request, 
 *          given the scaling factor (sf) of the database
 *
 *********************************************************************/ 

new_order_input_t create_no_input(int sf) 
{
    // check scaling factor
    assert(sf > 0);

    // produce NEW_ORDER params according to tpcc spec v.5.9
    new_order_input_t noin;

#ifndef USE_SAME_INPUT    

    noin._wh_id  = URand(1, sf);
    noin._d_id   = URand(1, 10);
    noin._c_id   = NURand(1023, 1, 3000);
    noin._ol_cnt = URand(5, 15);
    noin._rbk    = URand(1, 100); /* if rbk == 1 - ROLLBACK */

    // generate the items order
    for (int i=0; i<noin._ol_cnt; i++) {
        noin.items[i]._ol_i_id = NURand(8191, 1, 100000);
        noin.items[i]._ol_supply_wh_select = URand(1, 100); // 1 - 99
        noin.items[i]._ol_quantity = URand(1, 10);

        if (noin.items[i]._ol_supply_wh_select == 1) {
            /* remote new_order */
            noin.items[i]._ol_supply_wh_id = URand(1, sf);
        }
        else {
            /* home new_order */ 
            noin.items[i]._ol_supply_wh_id = noin._wh_id;
        }
    }

#else
    // same input
    noin._wh_id  = 1;
    noin._d_id   = 2;
    noin._c_id   = 3;
    noin._ol_cnt = 10;
    noin._rbk    = 50;

    // generate the items order
    for (int i=0; i<noin._ol_cnt; i++) {
        noin.items[i]._ol_i_id = 1;
        noin.items[i]._ol_supply_wh_select = 50;      
        noin.items[i]._ol_supply_wh_id = noin._wh_id; /* home new_order */ 
        noin.items[i]._ol_quantity = 5;
    }
#endif        

    return (noin);

}; // EOF: create_new_order



/********************************************************************* 
 *
 *  @fn:    create_payment_input
 *
 *  @brief: Creates the input for a new PAYMENT request, 
 *          given the scaling factor (sf) of the database
 *
 *********************************************************************/ 

payment_input_t create_payment_input(int sf) 
{
    // check scaling factor
    assert(sf > 0);

    // produce PAYMENT params according to tpcc spec v.5.9
    payment_input_t pin;

#ifndef USE_SAME_INPUT

    pin._home_wh_id = URand(1, sf);
    pin._home_d_id = URand(1, 10);    
    pin._h_amount = (long)URand(100, 500000)/(long)100.00;
    pin._h_date = time(NULL);

    pin._v_cust_wh_selection = URand(1, 100); // 85 - 15        
    if (pin._v_cust_wh_selection <= 85) {
        // all local payment
        pin._remote_wh_id = pin._home_wh_id;
        pin._remote_d_id = pin._home_d_id;
    }
    else {
        // remote warehouse
        do {
            pin._remote_wh_id = URand(1, sf);
        } while (pin._home_wh_id != pin._remote_wh_id);
        pin._remote_d_id = URand(1, 10);
    }
    
    pin._v_cust_ident_selection = URand(1, 100); // 60 - 40
    if (pin._v_cust_ident_selection <= 60) {
        // Calls the function that returns the correct cust_last
        generate_cust_last(NURand(255,0,999), pin._c_last);    
    }
    else {
        pin._c_id = NURand(1023, 1, 3000);
    }

#else
    // Same input
    pin._home_wh_id = 1;
    pin._home_d_id =  2;
    pin._v_cust_wh_selection = 80;
    pin._remote_wh_id = 1;
    pin._remote_d_id =  3;
    pin._v_cust_ident_selection = 50;
    pin._c_id =  1500;        
    //pin._c_last = NULL;
    pin._h_amount = 1000.00;
    pin._h_date = time(NULL);
#endif        

    return (pin);

}; // EOF: create_payment



/********************************************************************* 
 *
 *  @fn:    create_order_status_input
 *
 *  @brief: Creates the input for a new ORDER_STATUS request, 
 *          given the scaling factor (sf) of the database
 *
 *********************************************************************/ 

order_status_input_t create_order_status_input(int sf)
{
    // check scaling factor
    assert(sf > 0);

    // produce PAYMENT params according to tpcc spec v.5.4
    order_status_input_t osin;

#ifndef USE_SAME_INPUT    

    osin._wh_id    = URand(1, sf);
    osin._d_id     = URand(1, 10);
    osin._c_select = URand(1, 100); /* 60% - 40% */

    if (osin._c_select <= 60) {
        // Calls the function that returns the correct cust_last
        generate_cust_last(NURand(255,0,999), osin._c_last);    
    }
    else {
        osin._c_id = NURand(1023, 1, 3000);
    }

#else
    // same input
    osin._wh_id    = 1;
    osin._d_id     = 2;
    osin._c_select = 80;
    osin._c_id     = 3;
    //osin._c_last   = NULL;
#endif

    return (osin);

}; // EOF: create_order_status



/********************************************************************* 
 *
 *  @fn:    create_delivery_input
 *
 *  @brief: Creates the input for a new DELIVERY request, 
 *          given the scaling factor (sf) of the database
 *
 *********************************************************************/ 

delivery_input_t create_delivery_input(int sf)
{
    // check scaling factor
    assert(sf > 0);

    // produce PAYMENT params according to tpcc spec v.5.9
    delivery_input_t din;

#ifndef USE_SAME_INPUT    

    din._wh_id      = URand(1, sf);
    din._carrier_id = URand(1, 10);

#else
    // same input
    din._wh_id      = 1;
    din._carrier_id = 1;
#endif        

    return (din);

}; // EOF: create_delivery



/********************************************************************* 
 *
 *  @fn:    create_stock_level_input
 *
 *  @brief: Creates the input for a new STOCK_LEVEL request, 
 *          given the scaling factor (sf) of the database
 *
 *********************************************************************/ 

stock_level_input_t create_stock_level_input(int sf)
{
    // check scaling factor
    assert(sf > 0);

    // produce PAYMENT params according to tpcc spec v.5.4
    stock_level_input_t slin;

#ifndef USE_SAME_INPUT    

    slin._wh_id     = URand(1, sf);
    slin._d_id      = URand(1, 10);
    slin._threshold = URand(10, 20);

#else
    // same input
    slin._wh_id     = 1;
    slin._d_id      = 2;
    slin._threshold = 15;
#endif        

    return (slin);

}; // EOF: create_stock_level




EXIT_NAMESPACE(tpcc);
