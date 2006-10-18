/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _WORKLOAD_COMMON_H
#define _WORKLOAD_COMMON_H


#include "workload/common/tuple_source_once.h"
#include "workload/common/count_aggregate.h"
#include "workload/common/register_stage.h"

#include "workload/common/predicates.h"
#include "workload/common/int_comparator.h"
#include "workload/common/single_int_join.h"

#include "workload/common/q6_tscan_filter.h"
#include "workload/common/q6_aggregate.h"
#include "workload/common/q6_packet.h"
#include "workload/common/q6_client.h"

// determines the size of a given field in a struct
#define SIZEOF(s, f) (sizeof(((s*)NULL)->f))

// shortcut to memcpy a same-named field between two structs
#define COPY(d, dtype, s, stype, f) \
    memcpy(d + offsetof(dtype, f), \
           s + offsetof(stype, f), \
           SIZEOF(stype, f))

#endif
