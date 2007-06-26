/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file payment_baseline.h
 *
 *  @brief Implementation for the Baseline TPC-C Payment transaction
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/payment_baseline.h"
#include "stages/tpcc/common/payment_functions.h"

#include "workload/tpcc/tpcc_env.h"

using namespace qpipe;
using namespace tpcc_payment;


const c_str payment_baseline_packet_t::PACKET_TYPE = "PAYMENT_BASELINE";

const c_str payment_baseline_stage_t::DEFAULT_STAGE_NAME = "PAYMENT_BASELINE_STAGE";


/**
 *  @brief payment_baseline_stage_t constructor
 */

payment_baseline_stage_t::payment_baseline_stage_t() {
    
    TRACE(TRACE_DEBUG, "PAYMENT_BASELINE constructor\n");
}


/**
 *  @brief Execute TPC-C Payment transaction is a conventional way
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void payment_baseline_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    payment_baseline_packet_t* packet = 
	(payment_baseline_packet_t*)adaptor->get_packet();

    // Prints out the packet info
    packet->describe_trx();

    trx_result_tuple_t aTrxResultTuple = executePaymentBaseline(&packet->_p_in, 
                                                                packet->_trx_txn,
                                                                packet->get_trx_id(),
                                                                packet->_p_dbts);
    

    TRACE( TRACE_TRX_FLOW, "DONE. NOTIFYING CLIENT\n" );
    
    // writing output 

    // create output tuple
    // "I" own tup, so allocate space for it in the stack
    size_t dest_size = packet->output_buffer()->tuple_size();
    
    array_guard_t<char> dest_data = new char[dest_size];
    tuple_t dest(dest_data, dest_size);
    
    trx_result_tuple_t* dest_result_tuple;
    dest_result_tuple = aligned_cast<trx_result_tuple_t>(dest.data);
    
    *dest_result_tuple = aTrxResultTuple;
    
    adaptor->output(dest);


} // process_packet




/** @fn executePaymentBaseline
 *
 *  @brief Executes the PAYMENT transaction serially. Wrapper function.
 *  
 *  @return A trx_result_tuple_t with the status of the transaction.
 */
    
trx_result_tuple_t payment_baseline_stage_t::executePaymentBaseline(payment_input_t* pin, 
                                                                    DbTxn* txn, 
                                                                    const int id,
                                                                    s_payment_dbt_t* a_p_dbts) 
{
    // initialize the result structure
    trx_result_tuple_t aTrxResultTuple(UNDEF, id);
    
    try {
        
       TRACE( TRACE_TRX_FLOW, "*** EXECUTING TRX CONVENTIONALLY ***\n");
    
       /** @note PAYMENT TRX According to TPC-C benchmark, Revision 5.8.0 pp. 32-35 */

    
       /** Step 1: The database transaction is started */
       TRACE( TRACE_TRX_FLOW, "Step 1: The database transaction is started\n" );
       dbenv->txn_begin(NULL, &txn, 0);

       if (updateWarehouse(pin, txn, a_p_dbts)) {
           
           // Failed. Throw exception
           THROW1( BdbException, 
                   "WAREHOUSE update failed...\n");
       }


       if (updateDistrict(pin, txn, a_p_dbts)) {
           
           // Failed. Throw exception
           THROW1( BdbException, 
                   "DISTRICT update failed...\n");
       }


       if (updateCustomer(pin, txn, a_p_dbts)) {           
           // Failed. Throw exception
           THROW1( BdbException,
                   "CUSTOMER update failed...\n");
       }

       if (insertHistory(pin, txn, a_p_dbts)) {

           // Failed. Throw exception
           THROW1( BdbException, 
                   "HISTORY instertion failed...\n");
       }


    
       /** Step 6: The database transaction is committed */
       TRACE( TRACE_TRX_FLOW, "Step 6: The database transaction is committed\n" );
       txn->commit(0);
    }
    catch(DbException err) {
        dbenv->err(err.get_errno(), "%d: Caught exception\n");
        TRACE( TRACE_ALWAYS, "DbException - Aborting PAYMENT trx...\n");
        
        txn->abort();
	    
        aTrxResultTuple.set_state(ROLLBACKED);
        return (aTrxResultTuple);
        
        // FIXME (ip): We may want to have retries
        //if (++failed_tries > MAX_TRIES) {
        //   packet->_trx_txn->abort();
        //   TRACE( TRACE_ALWAYS, "MAX_FAILS - Aborting...\n");        
    }
    catch(...) {
        TRACE( TRACE_ALWAYS, "Unknown Exception - Aborting PAYMENT trx...\n");
        
        txn->abort();
	    
        aTrxResultTuple.set_state(ROLLBACKED);
        return (aTrxResultTuple);
    }

    // if reached this point transaction succeeded
    aTrxResultTuple.set_state(COMMITTED);
    return (aTrxResultTuple);

} // EOF: executePaymentBaseline



    
