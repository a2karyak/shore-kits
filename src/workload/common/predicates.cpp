/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/common/predicates.h"

using namespace qpipe;



ENTER_NAMESPACE(workload);



static bool use_deterministic_predicates(void) {
    return true;
}



predicate_randgen_t predicate_randgen_t::acquire(const char* caller_tag) {
    if (use_deterministic_predicates())
        /* deterministic */
        return predicate_randgen_t(caller_tag);
    else
        /* non-deterministic */
        return predicate_randgen_t(thread_get_self()->randgen());
}



EXIT_NAMESPACE(workload);
