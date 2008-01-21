/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "workload/tpcc/shore_tpcc_load.h"

using namespace tpcc;



/// **** Need to initialize volume_mutex

// pthread_mutex_t vol_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {

    sl_thread_t*smtu = new sl_thread_t(argc, argv);
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

    return rv;
}
