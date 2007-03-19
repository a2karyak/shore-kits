/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/common/randgen.h"


ENTER_NAMESPACE(workload);


static int use_deterministic = 1;


/**
 * @brief Thin wrapper about thread-local rand() that lets us control
 * whether predicates are randomly or deterministically selected.
 *
 * @return A number in [0,n) that is either generated randomly or
 * deterministically, depending on the use_deterministic flag.
 */
int randgen_next(int n)
{
    if (use_deterministic)
        return n-1;

    /* otherwise, do random */
    thread_t* self = thread_get_self();
    self->rand(n);
}



EXIT_NAMESPACE(workload);
