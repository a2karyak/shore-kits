
#include "workload/dispatcher_globals.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "engine/dispatcher/dispatcher_policy_os.h"
#include "engine/dispatcher/dispatcher_policy_rr_cpu.h"
#include "engine/dispatcher/dispatcher_policy_query_cpu.h"
#include "engine/dispatcher/dispatcher_policy_rr_module.h"

using namespace qpipe;



static dispatcher_policy_t* global_policy = NULL;



void global_dispatcher_policy_set(const char* dpolicy) {

  assert( global_policy == NULL );
  
  if ( !strcmp(dpolicy, "OS") )
    global_policy = new dispatcher_policy_os_t();
  if ( !strcmp(dpolicy, "RR_CPU") )
    global_policy = new dispatcher_policy_rr_cpu_t();
  if ( !strcmp(dpolicy, "QUERY_CPU") )
    global_policy = new dispatcher_policy_query_cpu_t();
  if ( !strcmp(dpolicy, "RR_MODULE") )
    global_policy = new dispatcher_policy_rr_module_t();
  if ( global_policy == NULL ) {
    TRACE(TRACE_ALWAYS, "Unrecognized dispatcher policy: %s\n", dpolicy);
    QPIPE_PANIC();
  }
 
}


dispatcher_policy_t* global_dispatcher_policy_get(void) {

  assert( global_policy != NULL );
  return global_policy;
}

