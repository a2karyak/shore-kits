
#include "core.h"
#include "scheduler.h"
#include "workload/dispatcher_globals.h"

using namespace scheduler;



static policy_t* global_policy = NULL;



void global_scheduler_policy_set(const char* dpolicy) {

  assert( global_policy == NULL );
  
  if ( !strcmp(dpolicy, "OS") )
    global_policy = new policy_os_t();
  if ( !strcmp(dpolicy, "RR_CPU") )
    global_policy = new policy_rr_cpu_t();
  if ( !strcmp(dpolicy, "QUERY_CPU") )
    global_policy = new policy_query_cpu_t();
  if ( !strcmp(dpolicy, "RR_MODULE") )
    global_policy = new policy_rr_module_t();
  if ( global_policy == NULL ) 
      throw EXCEPTION(qpipe::DispatcherException,
                      "Unrecognized scheduler policy: %s",
                      dpolicy);
}


policy_t* global_scheduler_policy_get(void) {

  assert( global_policy != NULL );
  return global_policy;
}

