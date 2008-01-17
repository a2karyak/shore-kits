/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file bdb_trx_packet.h
 *
 *  @brief Add a  BerkeleyDB-specific transaction handle to the trx_packet
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __BDB_TRX_PACKET_H
#define __BDB_TRX_PACKET_H

#include "stages/tpcc/common/trx_packet.h"


ENTER_NAMESPACE(qpipe);


/** Exported classes and data structures */

/** @class bdb_trx_packet_t
 *  @brief A trx_packet with a BDB-specific transaction handle
 */

class bdb_trx_packet_t : public trx_packet_t 
{
public:


    /** @brief The corresponding transaction handle of this packet */
    DbTxn* _trx_txn;


    bdb_trx_packet_t(const c_str     &packet_id,
                     const c_str     &packet_type,
                     tuple_fifo*     output_buffer,
                     tuple_filter_t* output_filter,
                     query_plan*     trx_plan,
                     bool _merge,
                     bool _unreserve,
                     const int       trx_id = NO_VALID_TRX_ID)
        : trx_packet_t(packet_id, packet_type, output_buffer,
                       output_filter, trx_plan, 
                       _merge,     /* whether to merge */
                       _unreserve, /* whether to unreserve worker on completion */
                       trx_id)
    { }


    virtual ~bdb_trx_packet_t() { }

    /** Member access functions */

    inline DbTxn* get_trx_txn() {
        return (_trx_txn);
    }
    
    
    inline void set_trx_txn(DbTxn* a_trx_txn) {        
        _trx_txn = a_trx_txn;
    }


    /** Helper Functions */

    /** @brief Describes the requested transaction */

    virtual void describe_trx() {
        TRACE(TRACE_ALWAYS, "A BDB-TRX request with id=%d\n", _trx_id);
    }

    
}; // EOF bdb_trx_packet



EXIT_NAMESPACE(qpipe);

#endif 
