/* -*- mode:C++; c-basic-offset:4 -*- */

#include "trace.h"
#include "tests/common/register_stage.h"

#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"


void register_stage_containers() {

    register_stage<tscan_stage_t>(1);
    register_stage<aggregate_stage_t>(1);
}
