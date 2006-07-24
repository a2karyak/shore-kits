/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _Q1_COMMON_H
#define _Q1_COMMON_H

#include "workload/tpch/drivers/tpch_q1.h"

#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "workload/tpch/tpch_env.h"

#include "engine/util/time_util.h"

#include "engine/stages/partial_aggregate.h"
#include "engine/stages/tscan.h"

#include "engine/dispatcher/dispatcher_policy.h"
#include "engine/dispatcher.h"



class TPCH_Q1 {

public:

    static void run(dispatcher_policy_t* dp) {
        tpch_q1_driver driver(c_str("TPC-H Q1"));
        driver.submit(dp);
    }        
};



#endif
