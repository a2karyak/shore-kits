/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/drivers/tpch_q6.h"

#include "workload/common.h"
#include "core.h"

#include "vldb07_model/model.h"

#include <stdio.h>


using namespace qpipe;
using namespace VLDB07Model;


ENTER_NAMESPACE(workload);


void tpch_q6_driver::submit(void* disp) {
 
    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
    thread_t* this_thread = thread_get_self();
    c_str qtag("Q6CL_%s_", this_thread->thread_name().data());
    packet_t* q6 = create_q6_packet( qtag.data(), dp );
    
    // VLDB07 share/unshared execution predictive model.
    if (VLDB07_model_t::is_model_enabled()) {
        bool osp_switch = VLDB07_model_t::is_shared_faster_scan(   VLDB07_model_t::get_M()     // m
                                                                 , 9.297  // p
                                                                 , 3.5144 // w
                                                                 , 5.2716 // s
                                                                 , 188.1   // k
                                                                 , 0.6    // fs
                                                                 , 0.85   // fa
                                                               );
        fprintf(stderr, "************* MODEL = %d\n", osp_switch);
        qpipe::set_osp_for_type(c_str("TSCAN"), osp_switch);
    }

    // Dispatch packet
    q6_process_tuple_t pt;
    process_query(q6, pt);
}


EXIT_NAMESPACE(workload);
