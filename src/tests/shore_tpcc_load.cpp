/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "tests/common.h"
#include "stages/tpcc/shore/shore_tools.h"
#include "workload/tpcc/shore_tpcc_env.h"
#include "workload/tpcc/shore_tpcc_load.h"

using namespace tpcc;


int main(int argc, char* argv[]) {

    // Instanciate the Shore Environment
    shore_env = new ShoreTPCCEnv("shore.conf");
    
    // Load data to the Shore Database
    TRACE( TRACE_ALWAYS, "Loading... ");
    loading_smthread_t* loader = new loading_smthread_t(shore_env);
    run_smthread(loader, c_str("loader"));

    // Load data to the Shore Database
    TRACE( TRACE_ALWAYS, "Closing... ");
    closing_smthread_t* closer = new closing_smthread_t(shore_env);
    run_smthread(closer, c_str("closer"));

    return (0);
}
