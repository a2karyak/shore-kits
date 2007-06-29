/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/drivers/tpch_q6.h"
#include "workload/tpch/q6_packet.h"

#include "workload/common.h"
#include "core.h"


using namespace qpipe;
using namespace workload;
using namespace tpch;

ENTER_NAMESPACE(workload);


void tpch_q6_driver::submit(void* disp, memObject_t*) {
 
    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
    thread_t* this_thread = thread_get_self();
    c_str qtag("Q6CL_%s_", this_thread->thread_name().data());
    packet_t* q6 = create_q6_packet( qtag.data(), dp );
    
    q6_process_tuple_t pt;
    process_query(q6, pt);
}


EXIT_NAMESPACE(workload);

