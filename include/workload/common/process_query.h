/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _PROCESS_QUERY_H
#define _PROCESS_QUERY_H

#include "workload/common/process_tuple.h"

using namespace qpipe;


ENTER_NAMESPACE(workload);


extern bool RESERVE_WORKERS_BEFORE_DISPATCH;


void process_query(packet_t* root, process_tuple_t& pt);


EXIT_NAMESPACE(workload);


#endif
