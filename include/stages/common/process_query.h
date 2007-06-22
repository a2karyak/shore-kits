/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __PROCESS_QUERY_H
#define __PROCESS_QUERY_H

#include "stages/common/process_tuple.h"


ENTER_NAMESPACE(qpipe);


void process_query(packet_t* root, process_tuple_t& pt);


EXIT_NAMESPACE(qpipe);


#endif
