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

    // produce PAYMENT params according to tpcc spec v.5.4
    payment_input_t pin;

#ifndef USE_SAME_INPUT

    pin._home_wh_id = URand(1, sf);
    pin._home_d_id = URand(1, 10);
    pin._v_cust_wh_selection = URand(1, 100); // 85 - 15
    pin._remote_wh_id = URand(1, sf);
    pin._remote_d_id = URand(1, 10);
    pin._v_cust_ident_selection = URand(1, 100); // 60 - 40
    pin._c_id = NURand(1023, 1, 3000);
    
    // Calls the function that returns the correct cust_last
    generate_cust_last(NURand(255, 0, 999),pin._c_last);
    
    pin._h_amount = (long)URand(100, 500000)/(long)100.00;
    pin._h_date = time(NULL);

#else
    // Same input
    pin._home_wh_id = 1;
    pin._home_d_id =  1;
    pin._v_cust_wh_selection = 50;
    pin._remote_wh_id = 1;
    pin._remote_d_id =  1;
    pin._v_cust_ident_selection = 50;
    pin._c_id =  1500;        
    pin._c_last = NULL;
    pin._h_amount = 1000.00;
    pin._h_date = time(NULL);
#endif        

    return (pin);
}


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

    // produce PAYMENT params according to tpcc spec v.5.4
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
    noin._d_id   = 1;
    noin._c_id   = 1;
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
}



EXIT_NAMESPACE(tpcc);
