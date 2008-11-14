/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "tests/common.h"

#include "sm/shore/shore_tools.h"

#include "workload/tpcc/shore_tpcc_env.h"
#include "workload/tpcc/shore_tpcc_load.h"


using namespace shore;
using namespace tpcc;


int main(int argc, char* argv[]) {

    // Instanciate the Shore Environment
    _g_shore_env = new ShoreTPCCEnv("shore.conf");
    
    // Load data to the Shore Database
    int* r = NULL;
    TRACE( TRACE_ALWAYS, "Loading... ");
    loading_smt_t* loader = new loading_smt_t(_g_shore_env, c_str("loader"));
    run_smthread<loading_smt_t,int>(loader, r);
    delete (loader);

    // Load data to the Shore Database
    TRACE( TRACE_ALWAYS, "Closing... ");
    closing_smt_t* closer = new closing_smt_t(_g_shore_env, c_str("closer"));
    run_smthread<closing_smt_t,int>(closer, r);
    delete (closer);

    return (0);
}
