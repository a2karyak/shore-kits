/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __BDB_TPCC_PAYMENT_FINALIZE_H
#define __BDB_TPCC_PAYMENT_FINALIZE_H

#include <cstdio>

#include "core.h"
#include "util.h"
#include "scheduler.h"

#include "stages/tpcc/common/bdb_trx_packet.h"

#include "stages/tpcc/payment_upd_wh.h"
#include "stages/tpcc/payment_upd_distr.h"
#include "stages/tpcc/payment_upd_cust.h"
#include "stages/tpcc/payment_ins_hist.h"


using namespace qpipe;



/* exported datatypes */

class payment_finalize_packet_t : public bdb_trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;


    // Removed the guards

    // The input packets
    payment_upd_wh_packet_t* _upd_wh;
    payment_upd_distr_packet_t* _upd_distr;
    payment_upd_cust_packet_t* _upd_cust;
    payment_ins_hist_packet_t* _ins_hist;

    // The input buffers
    tuple_fifo* _upd_wh_buffer;
    tuple_fifo* _upd_distr_buffer;
    tuple_fifo* _upd_cust_buffer;
    tuple_fifo* _ins_hist_buffer;
    

    /**
     *  @brief payment_finalize_packet_t constructor.
     *
     *  @param packet_id The ID of this packet. This should point to a
     *  block of bytes allocated with malloc(). This packet will take
     *  ownership of this block and invoke free() when it is
     *  destroyed.
     *
     *  @param output_buffer The buffer where this packet should send
     *  its data. A packet DOES NOT own its output buffer (we will not
     *  invoke delete or free() on this field in our packet
     *  destructor).
     *
     *  @param output_filter The filter that will be applied to any
     *  tuple sent to output_buffer. The packet OWNS this filter. It
     *  will be deleted in the packet destructor.
     *
     *  @param trx_id The transaction id
     *
     *  @param The four packets that make the payment transaction
     */

    payment_finalize_packet_t(const c_str    &packet_id,
                              tuple_fifo*     output_buffer,
                              tuple_filter_t* output_filter,
                              const int a_trx_id,
                              bdb_trx_packet_t* upd_wh,
                              bdb_trx_packet_t* upd_distr,
                              bdb_trx_packet_t* upd_cust,
                              bdb_trx_packet_t* ins_hist)
	: bdb_trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                           create_plan(a_trx_id),
                           true, /* merging allowed */
                           true,  /* unreserve worker on completion */
                           a_trx_id
                           ),
          _upd_wh((payment_upd_wh_packet_t*)upd_wh),
          _upd_distr((payment_upd_distr_packet_t*)upd_distr),
          _upd_cust((payment_upd_cust_packet_t*)upd_cust),
          _ins_hist((payment_ins_hist_packet_t*)ins_hist),
          _upd_wh_buffer(upd_wh->output_buffer()),
          _upd_distr_buffer(upd_distr->output_buffer()),
          _upd_cust_buffer(upd_cust->output_buffer()),
          _ins_hist_buffer(ins_hist->output_buffer())
    {
        /* asserts on uninitiliazed input packets */
        assert(_upd_wh != NULL);
        assert(_upd_distr != NULL);
        assert(_upd_cust != NULL);
        assert(_ins_hist != NULL);       

        /* asserts on NULL input buffers */
        assert(_upd_wh_buffer != NULL);
        assert(_upd_distr_buffer != NULL);
        assert(_upd_cust_buffer != NULL);
        assert(_ins_hist_buffer != NULL);       
    }


    void describe_trx() {

        TRACE( TRACE_ALWAYS,
               "\nFINALIZE - TRX_ID=%d\n",
               _trx_id);
    }

    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_trx_id) 
    {
        c_str action("%s:%d", PACKET_TYPE.data(), a_trx_id);

        return new query_plan(action, "none", NULL, 0);
    }


    virtual void declare_worker_needs(resource_declare_t* declare) {
        
        /* declares inputs */
        _upd_wh->declare_worker_needs(declare);
        _upd_distr->declare_worker_needs(declare);
        _upd_cust->declare_worker_needs(declare);
        _ins_hist->declare_worker_needs(declare);

        /* declare own needs */
        declare->declare(_packet_type, 1);
    }


    /** @brief Calls for this transaction to rollback */
    
    void rollback() {
        
        TRACE( TRACE_ALWAYS, "~~~ Should Rollback TRX=%d ~~~\n", _trx_id);
    }

    /** @brief Calls for this transaction to commit */

    void commit() {

        TRACE( TRACE_ALWAYS, "~~~ Should Commit: TRX=%d ~~~~\n", _trx_id);
    }

}; // END OF CLASS: payment_finalize_packet_t



/**
 *  @brief PAYMENT_FINALIZE stage. 
 *
 *  The stage at the end of each PAYMENT transaction. It is the one which
 *  coordinates the commit/rollback between the parallel executing parts.
 */

class payment_finalize_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef payment_finalize_packet_t stage_packet_t;
    payment_finalize_stage_t();
    
    virtual ~payment_finalize_stage_t() { 

	TRACE(TRACE_ALWAYS, "PAYMENT_FINALIZE destructor\n");	
    }
    
}; // END OF CLASS: payment_finalize_stage_t



#endif
