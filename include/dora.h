/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __DORA_H
#define __DORA_H


// define the ONLYDORA if want to test the DORA mechanism without Shore-mt
#undef ONLYDORA
//#define ONLYDORA



#include "dora/dora_error.h"
#include "dora/common.h"

#include "dora/key.h"
#include "dora/rvp.h"
#include "dora/action.h"

#include "dora/lockman.h"

#include "dora/worker.h"
#include "dora/partition.h"
#include "dora/part_table.h"

#include "dora/range_action.h"
#include "dora/range_partition.h"
#include "dora/range_part_table.h"

#ifdef CFG_DORA_LOGGER
#include "dora/dora_logger.h"
#endif


#endif /** __DORA_H */
