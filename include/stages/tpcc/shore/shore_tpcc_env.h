/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_env.h
 *
 *  @brief Definition of the Shore TPC-C environment
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_ENV_H
#define __SHORE_TPCC_ENV_H

#include "sm_vas.h"
#include "util.h"

#include "stages/tpcc/common/tpcc_scaling_factor.h"
#include "stages/tpcc/common/tpcc_const.h"
#include "stages/tpcc/common/tpcc_struct.h"
#include "stages/tpcc/common/tpcc_input.h"

#include "sm/shore/shore_env.h"

#include "stages/tpcc/shore/shore_tpcc_schema.h"

#include <map>


using namespace shore;


ENTER_NAMESPACE(tpcc);


using std::map;


/******** Exported variables ********/

class ShoreTPCCEnv;
extern ShoreTPCCEnv* shore_env;



/******************************************************************** 
 * 
 *  ShoreTPCCEnv
 *  
 *  Shore TPC-C Database.
 *
 ********************************************************************/

class ShoreTPCCEnv : public ShoreEnv
{
private:       
    // TPC-C tables

    /** all the tables */
    warehouse_t   _warehouse;
    district_t    _district;
    customer_t    _customer;
    history_t     _history;
    new_order_t   _new_order;
    order_t       _order;
    order_line_t  _order_line;
    item_t        _item;
    stock_t       _stock;

    /** some stats */
    long _no_cnt;        // new order count
    
public:

    /** Construction  */
    ShoreTPCCEnv(string confname) 
        : ShoreEnv(confname)
    {
    }

    ~ShoreTPCCEnv() {         
    }

    /** Public methods */    
    int loaddata();  

    /* --- access to the tables --- */
    warehouse_t*  warehouse() { return (&_warehouse); }
    district_t*   district()  { return (&_district); }
    customer_t*   customer()  { return (&_customer); }
    history_t*    history()   { return (&_history); }
    new_order_t*  new_order() { return (&_new_order); }
    order_t*      order()     { return (&_order); }
    order_line_t* orderline() { return (&_order_line); }
    item_t*       item()      { return (&_item); }
    stock_t*      stock()     { return (&_stock); }

    /* --- kit baseline trxs --- */
    w_rc_t xct_new_order(new_order_input_t* no_input, const int xct_id);
    w_rc_t xct_payment(payment_input_t * pay_input, const int xct_id);
    w_rc_t xct_order_status(order_status_input_t* status_input, const int xct_id);
    w_rc_t xct_delivery(delivery_input_t* deliv_input, const int xct_id);
    w_rc_t xct_stock_level(stock_level_input_t* level_input, const int xct_id);

    /* --- various helper and stats --- */
    inline long no_cnt() const { return (_no_cnt); }
    inline long inc_no_cnt() { return (++_no_cnt); }



}; // EOF ShoreTPCCEnv


EXIT_NAMESPACE(tpcc);


#endif /* __SHORE_TPCC_ENV_H */

