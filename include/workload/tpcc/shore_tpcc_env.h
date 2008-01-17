/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_env.h
 *
 *  @brief Definition of the Shore TPC-C environment
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_ENV_H
#define __SHORE_TPCC_ENV_H

#include "sm_vas.h"

#include "util/guard.h"
#include "util/c_str.h"
#include "util/namespace.h"

#include "stages/tpcc/common/tpcc_scaling_factor.h"
#include "stages/tpcc/common/tpcc_struct.h"

//#include "stages/tpcc/common/tpcc_struct.h"



ENTER_NAMESPACE(tpcc);

/* constants */

#define SHORE_TPCC_DATA_DIR "tpcc_sf"
#define SHORE_TPCC_DATA_WAREHOUSE "WAREHOUSE.dat"
#define SHORE_TPCC_DATA_DISTRICT  "DISTRICT.dat"
#define SHORE_TPCC_DATA_CUSTOMER  "CUSTOMER.dat"
#define SHORE_TPCC_DATA_HISTORY   "HISTORY.dat"

#define SHORE_TPCC_DATA_ITEM      "ITEM.dat"
#define SHORE_TPCC_DATA_NE_ORDER  "NEW_ORDER.dat"
#define SHORE_TPCC_DATA_ORDER     "ORDER.dat"
#define SHORE_TPCC_DATA_ORDERLINE "ORDERLINE.dat"
#define SHORE_TPCC_DATA_STOCK     "STOCK.dat"


////////////////////////////////////////////////////////////////////////

/** @class ShoreTPCCEnv
 *  
 *  @brief Class that contains the various data structures used in the
 *  Shore TPC-C environment.
 */

class ShoreTPCCEnv 
{
private:    

    static const int WAREHOUSE = 0;
    static const int DISTRICT = 1;
    static const int CUSTOMER = 2;
    static const int HISTORY = 3;

    static const int SHORE_PAYMENT_TABLES = 4;


    
    /** Private variables */
    bool _initialized;

    ss_m* _ssm; /// database handle
    

public:
    
    int loaddata(c_str loadDir);

    /** Construction  */

    ShoreTPCCEnv() {
	_initialized = false;

        /*
        im_customers.set_name(c_str("customer"));
        im_histories.set_name(c_str("history"));
	im_warehouses.set_name(c_str("warehouse"));
	im_districts.set_name(c_str("district"));
        */
    }


    ~ShoreTPCCEnv() { }

    bool is_initialized() const { return _initialized; }
    void dump();


}; // EOF ShoreTPCCEnv


EXIT_NAMESPACE(tpcc);


#endif
