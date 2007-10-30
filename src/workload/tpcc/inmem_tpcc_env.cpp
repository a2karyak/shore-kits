/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_env.cpp
 *
 *  @brief Declaration of the InMemory TPC-C environment
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "workload/tpcc/inmem_tpcc_env.h"

using namespace tpcc;

/** Exported functions */

int InMemTPCCEnv::loaddata(c_str loadDir) {

    TRACE( TRACE_DEBUG, "Getting data from (%s)\n", loadDir.data());

    assert (1==0); // (ip) Not implemented yet!!!

    return (0);
}

