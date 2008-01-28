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
#include "stages/tpcc/common/tpcc_struct.h"

#include "stages/tpcc/shore/shore_env.h"
#include "stages/tpcc/shore/shore_tpcc_const.h"
#include "stages/tpcc/shore/shore_tpcc_schema.h"

#include <map>



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

class ShoreTPCCEnv : public shore::ShoreEnv
{
private:       
    // TPC-C tables

    /* all the tables */
    warehouse_t   _warehouse;
    district_t    _district;
    customer_t    _customer;
    history_t     _history;
    new_order_t   _new_order;
    order_t       _order;
    order_line_t  _order_line;
    item_t        _item;
    stock_t       _stock;

    
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

}; // EOF ShoreTPCCEnv


EXIT_NAMESPACE(tpcc);


#endif /* __SHORE_TPCC_ENV_H */

