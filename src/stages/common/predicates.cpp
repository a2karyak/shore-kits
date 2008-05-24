/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/common/predicates.h"


ENTER_NAMESPACE(qpipe);



static bool _audp = true;

bool always_use_deterministic_predicates(void) {
    return _audp;
}



EXIT_NAMESPACE(qpipe);
