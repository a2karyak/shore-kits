/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file payment_baseline.h
 *
 *  @brief Interface for the Baseline TPC-C Payment transaction.
 *  The Baseline implementation uses a single thread for the entire
 *  transaction. We wrap the code to a stage in order to use the
 *  same subsystem
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __TPCC_PAYMENT_BASELINE_H
#define __TPCC_PAYMENT_BASELINE_H


#include <cstdio>

#include "core.h"

#include "stages/tpcc/payment_begin.h"


using namespace qpipe;


/* exported datatypes */

/** @note The payment_begin_packet is adequate. Since Baseline and Staged
 *  implementation of the transaction have the same interface.
 */


/** 
 *  @brief PAYMENT_BASELINE stage. 
 *
 *  @desc Executes the entire tpcc payment transaction in a conventional,
 *  single-threaded fashion.
 */

class payment_baseline_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef payment_begin_packet_t stage_packet_t;

    payment_baseline_stage_t();
 
    ~payment_baseline_stage_t() { 
	TRACE(TRACE_ALWAYS, "PAYMENT_BASELINE destructor\n");	
    }        
    
}; // EOF: payment_baseline_stage_t



#endif
