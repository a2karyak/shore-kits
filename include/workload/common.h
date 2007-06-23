/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __WORKLOAD_COMMON_H
#define __WORKLOAD_COMMON_H

#include "workload/common/tuple_source_once.h"
#include "workload/common/count_aggregate.h"
#include "workload/common/register_stage.h"

#include "workload/common/int_comparator.h"
#include "workload/common/single_int_join.h"

// FIXME (ip) To reduce the need for code-changes, after moving process_* and
// predicates to stages/common/, I added the following inclusion.
// Should be removed 
#include "stages/common.h"
//#include "stages/tpcc/common/tpcc_struct.h"

#endif
