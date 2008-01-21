/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "tests/common.h"
#include "stages/tpcc/shore/shore_tools.h"
#include "workload/tpcc/shore_tpcc_load.h"

using namespace tpcc;


int main(int argc, char* argv[]) {

    // Instanciate the Shore Environment
    cout << "Initializing ShoreEnv..." << endl;
    shore_env = new ShoreTPCCEnv();
    thread_init();
    

    sl_thread_t* smtu = new sl_thread_t(shore_env);
    if (!smtu)
	W_FATAL(fcOUTOFMEMORY);

    w_rc_t e = smtu->fork();
    if(e) {
	cerr << "error forking main thread: " << e <<endl;
	return 1;
    }

    e = smtu->join();
    if(e) {
	cerr << "error joining main thread: " << e <<endl;
	return 1;
    }

    int	rv = smtu->retval;
    delete smtu;

    cout << "Closing..." << endl;
    return rv;
}
