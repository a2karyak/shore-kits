
#ifndef _STAGES_H
#define _STAGES_H

#include "stages/aggregate.h"
#include "stages/bnl_in.h"
#include "stages/bnl_join.h"
#include "stages/fdump.h"
#include "stages/fscan.h"
#include "stages/func_call.h"
#include "stages/hash_join.h"
#include "stages/pipe_hash_join.h"
#include "stages/merge.h"
#include "stages/partial_aggregate.h"
#include "stages/hash_aggregate.h"
#include "stages/sort.h"
#include "stages/sorted_in.h"
#include "stages/tscan.h"
#include "stages/echo.h"
#include "stages/sieve.h"
#include "stages/tuple_source.h"

#include "stages/tpcc/payment_begin.h"
#include "stages/tpcc/payment_upd_wh.h"
#include "stages/tpcc/payment_upd_distr.h"
#include "stages/tpcc/payment_upd_cust.h"
#include "stages/tpcc/payment_ins_hist.h"
#include "stages/tpcc/payment_finalize.h"


#endif

