/* -*- mode:C++; c-basic-offset:4 -*- */


#include <procfs.h>

#include <iostream>
#include <fstream>

#include "util/prcinfo.h"

using namespace std;

ifstream::pos_type size;

int main () 
{

    processinfo_t aprcinfo;

    sleep(1);

    for (int i=0; i<1200; i++) {
        printf(".");
    }
    
    aprcinfo.print();
    
    return 0;
}


// #include "tests/common/tester_common.h"
// int main(int argc, char* argv[]) 
// {
//     thread_init();

//     open("/proc/self");


//     TRACE_SET( TRACE_ALWAYS | TRACE_STATISTICS | TRACE_NETWORK | TRACE_CPU_BINDING
//                //              | TRACE_QUERY_RESULTS
//                //              | TRACE_PACKET_FLOW
//                //               | TRACE_RECORD_FLOW
//                //               | TRACE_TRX_FLOW
//                | TRACE_DEBUG
//                );


//     return (0);
// }



