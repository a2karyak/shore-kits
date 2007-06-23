/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file trx_packet.cpp
 *
 *  @brief A trx_packet is a normal packet with a transaction id, status identifier,
 *  and a corresponding db-specific (currently BerkeleyDB) transaction handle.
 *
 *  @author Ippokratis Pandis (ipandis)
 */
 
#include "stages/tpcc/common/trx_packet.h"



ENTER_NAMESPACE(qpipe);


/** Helper functions */

/**
 *  @brief Displays in a friendly way a TrxState
 */

c_str translate_state(TrxState aState) {

    switch (aState) {
    case UNDEF:
        return ("Undef");
        break;
    case UNSUBMITTED:
        return ("Unsubmitted");
        break;
    case SUBMITTED:
        return ("Submitted");
        break;
    case POISSONED:
        return ("Poissoned");
        break;
    case COMMITTED:
        return ("Commited");
        break;
    case ROLLBACKED:
        return ("Rollbacked");
        break;
    }

    return ("Known");
}


////////////////////////
// class trx_packet_t //

/**
 *  @brief trx_packet_t constructor
 */

trx_packet_t::trx_packet_t(const c_str       &packet_id,
                           const c_str       &packet_type,
                           tuple_fifo*        output_buffer,
                           tuple_filter_t*    output_filter,
                           query_plan*        trx_plan,
                           bool               _merge,
                           bool               _unreserve,
                           const int          trx_id)
    : packet_t(packet_id, packet_type, output_buffer, 
               output_filter, trx_plan,
               _merge,    /* whether to merge */
               _unreserve /* whether to unreserve worker on completion */
               )
{
    _trx_id = trx_id;
    
    TRACE(TRACE_PACKET_FLOW, "Created %s TRX packet with ID %s\n",
          _packet_type.data(),
          _packet_id.data());
}



/**
 *  @brief trx_packet_t destructor.
 */

trx_packet_t::~trx_packet_t(void) {
    
    TRACE(TRACE_PACKET_FLOW, "Destroying %s TRX packet with ID %s\n",
	  _packet_type.data(),
	  _packet_id.data());
}



////////////////////////////
// class trx_result_tuple //

/** @fn reset
 *  @brief trx_result_tuple initializer 
 */

void trx_result_tuple::reset(TrxState aTrxState, int anID) {

    // check for validity of inputs
    assert ((aTrxState >= UNDEF) && (aTrxState <= ROLLBACKED));
    assert (anID >= NO_VALID_TRX_ID);

    R_STATE = aTrxState;
    R_ID = anID;
}


/** @fn copy constructor */
trx_result_tuple::trx_result_tuple(const trx_result_tuple& t) {

    R_STATE = t.R_STATE;
    R_ID = t.R_ID;
}
    
    
/** @fn copy assingment */
trx_result_tuple& trx_result_tuple::operator=(const trx_result_tuple& t) {

    R_STATE = t.R_STATE;
    R_ID = t.R_ID;
    
    return (*this);
}



EXIT_NAMESPACE(qpipe);
