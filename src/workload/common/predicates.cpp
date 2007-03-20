/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/common/predicates.h"

using namespace qpipe;



ENTER_NAMESPACE(workload);



static bool _audp = true;

bool always_use_deterministic_predicates(void) {
    return _audp;
}



EXIT_NAMESPACE(workload);
