/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _PAYMENT_BEGIN_H
#define _PAYMENT_BEGIN_H

#include <cstdio>

#include "stages/tpcc/trx_packet.h"
#include "core.h"
#include "util.h"


using namespace qpipe;


/**
 *  @brief Possible states of a transaction
 */

enum TrxState { UNDEF, UNSUBMITTED, SUBMITTED, POISSONED, 
		COMMITTED, ROLLBACKED };


/* exported datatypes */

class payment_begin_packet_t : public trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;

    TrxState _trx_state;

    /**
     *  @brief PAYMENT transaction inputs:
     *  
     *  1) HOME_WH_ID int [1 .. SF] : home warehouse id
     *  2) HOME_D_ID int [1 .. 10]  : home district id
     *  3) V_CUST_WH_SELECTION int [1 .. 100] : customer warehouse selection ( 85% - 15%)
     *  4) REMOTE_WH_ID int [1 .. SF] : remote warehouse id (optional)
     *  5) REMOTE_D_ID int [1 .. 10] : remote district id (optional)
     *  6) V_CUST_IDENT_SELECTION int [1 .. 100] : customer identification selection ( 60% - 40%)
     *  7) C_ID int : customer id (C_ID = NURand(1023, 1, 3000) (optional)
     *  8) C_LAST char* : customer lastname (using NURand(255, 0, 999) (optional)
     *  9) H_AMOUNT long [1.00 .. 5,000.00] : the payment amount
     *  10) H_DATE char* : the payment time
     */

    int _home_wh_id;
    int _home_d_id;
    int _v_cust_wh_selection;
    int _remote_wh_id;
    int _remote_d_id;
    int _v_cust_ident_selection;
    int _c_id;
    char _c_last[16];
    double _h_amount;
    char* _h_date;



    /**
     *  @brief payment_begin_packet_t constructor.
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
     *  @param All the PAYMENT transaction input variables
     */

    payment_begin_packet_t(const c_str    &packet_id,
                           tuple_fifo*     output_buffer,
                           tuple_filter_t* output_filter,
                           const int a_home_wh_id,   
                           const int a_home_d_id,
                           const int a_v_cust_wh_selection,
                           const int a_remote_wh_id,
                           const int a_remote_d_id,
                           const int a_v_cust_ident_selection,
                           const int a_c_id,
                           const char a_c_last[16],
                           const double a_h_amount,
                           const char* a_h_date)
	: trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                       create_plan(a_c_id, a_h_amount, a_h_date),
                       true, /* merging allowed */
                       true  /* unreserve worker on completion */
                       ),
          _home_wh_id(a_home_wh_id),
          _home_d_id(a_home_d_id),
          _v_cust_wh_selection(a_v_cust_wh_selection),
          _remote_wh_id(a_remote_wh_id),
          _remote_d_id(a_remote_d_id),
          _v_cust_ident_selection(a_v_cust_ident_selection),
          _c_id(a_c_id),
          _h_amount(a_h_amount)
    {
	assert(a_h_date != NULL);

	if (a_c_last != NULL) {	    
	    strncpy(_c_last, a_c_last, 15);
	    _c_last[16] = '\0';
	}
	    
	_h_date = new char[strlen(a_h_date) + 1];
        strncpy(_h_date, a_h_date, strlen(a_h_date));
        _h_date[strlen(a_h_date)] = '\0';

        _trx_state = UNDEF;
    }

    
    // Destructor
    ~payment_begin_packet_t() {
	
 	if (_h_date)
 	    delete (_h_date);
    }


    void describe_trx() {

        TRACE( TRACE_ALWAYS, \
               "PAYMENT:\nWH_ID=%d\t\tD_ID=%d\n" \
               "SEL_WH=%d\tSEL_IDENT=%d\n" \
               "REM_WH_ID=%d\tREM_D_ID=%d\n" \
               "C_ID=%d\tC_LAST=%s\n" \
               "H_AMOUNT=%.2f\tH_DATE=%s\n", \
               _home_wh_id, _home_d_id, 
               _v_cust_wh_selection, _v_cust_ident_selection,
               _remote_wh_id, _remote_d_id,
               _c_id, _c_last,
               _h_amount, _h_date);
    }

    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_c_id,
                                    const double a_h_amount,
                                    const char* a_h_date) 
    {
        c_str action("%s:%d:%f:%d", PACKET_TYPE.data(), 
		     a_c_id, a_h_amount, a_h_date);

        return new query_plan(action, "none", NULL, 0);
    }


    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        /* no inputs */
    }

}; // END OF CLASS: payment_begin_packet_t



/**
 *  @brief PAYMENT_BEGIN stage. 
 *
 *  1) Assigns a unique id to the submitted PAYMENT transaction and submits the 
 *  appropriate packets to their stages (PAYMENT_CUST, PAYMENT_WH, and 
 *  PAYMENT_DIST).
 *
 *  2) Once all packets return: Commits if everyone successful. Rollbacks if any
 *  probkem.
 */

class payment_begin_stage_t : public stage_t {

protected:

    virtual void process_packet();

    static int _trx_counter;
    static pthread_mutex_t _trx_counter_mutex;
    

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef payment_begin_packet_t stage_packet_t;

    payment_begin_stage_t();
    
    virtual ~payment_begin_stage_t() { 

	TRACE(TRACE_ALWAYS, "PAYMENT_BEGIN destructor\n");	
	thread_mutex_destroy( _trx_counter_mutex );
    }

    int get_next_counter();
    
}; // END OF CLASS: payment_begin_stage_t



#endif
