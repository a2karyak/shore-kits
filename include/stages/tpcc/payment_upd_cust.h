/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _PAYMENT_UPD_CUST_H
#define _PAYMENT_UPD_CUST_H

#include <cstdio>

#include "stages/tpcc/trx_packet.h"
#include "core.h"
#include "util.h"


using namespace qpipe;



/* exported datatypes */

class payment_upd_cust_packet_t : public trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;

    /**
     *  @brief PAYMENT_UPD_WH transaction inputs:
     *  
     *  1) WH_ID int: warehouse id
     *  2) DISTR_ID int: district id
     *  3) CUST_ID int: customer id
     *  4) CUST_LAST char*: customer lastname
     *  5) H_AMOUNT long: payment amount
     */

    int _wh_id;
    int _distr_id;
    int _cust_id;
    char* _cust_last;
    double _amount;



    /**
     *  @brief payment_upd_cust_packet_t constructor.
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
     *  @param wh_id The warehouse id
     *  @param distr_id The district id
     *  @param cust_id The customer id
     *  @param cust_last The customer last name
     *  @param h_amount The payment amount
     */

    payment_upd_cust_packet_t(const c_str    &packet_id,
                              tuple_fifo*     output_buffer,
                              tuple_filter_t* output_filter,
                              const int a_trx_id,
                              const int a_wh_id,   
                              const int a_distr_id,
                              const int a_cust_id,
                              const char* a_cust_last,
                              const double a_amount)
      : trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                     create_plan(a_trx_id, a_wh_id, a_distr_id, a_cust_id, a_amount),
                     false, /* merging not allowed */
                     true,  /* unreserve worker on completion */
                     a_trx_id
                     ),
      _wh_id(a_wh_id),
      _distr_id(a_distr_id),
      _cust_id(a_cust_id),
      _amount(a_amount)
      {
        if (a_cust_last != NULL) {
            _cust_last = new char[strlen(a_cust_last) + 1];
            strncpy(_cust_last, a_cust_last, strlen(a_cust_last));
            _cust_last[strlen(a_cust_last)] = '\0';
        }
      }

    
    
    // Destructor
    ~payment_upd_cust_packet_t() { }


    void describe_trx() {

        TRACE( TRACE_ALWAYS,
               "\nUPD_CUST - TRX_ID=%d" \
               "\nWH_ID=%d\t\tDISTRICT=%d\tAMOYNT=%.2f\n" \
               "CUST_ID=%d\tCUST_LAST=%s\n",
               _trx_id, 
               _wh_id, _cust_id, _amount, _cust_id, _cust_last);
    }

    
    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_trx_id, 
                                    const int a_wh_id,
                                    const int a_distr_id,
                                    const int a_cust_id,
                                    const double a_amount)
      {
        c_str action("%s:%d:%d:%d:%d:%f", PACKET_TYPE.data(), 
		     a_trx_id, a_wh_id, a_distr_id, a_cust_id, a_amount);
        
        return new query_plan(action, "none", NULL, 0);
      }


    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        /* no inputs */
    }

}; // END OF CLASS: payment_upd_cust_packet_t



/**
 *  @brief PAYMENT_UPD_CUST stage. 
 *
 *  Case 1: The customer is selected based on customer number.
 *  The row in the CUSTOMER table with matching C_W_ID, C_D_ID and C_ID is selected.
 *  C_FIRST, C_MIDDLE, C_LAST, C_STREET_1, C_STREET_2, C_CITY, C_STATE, C_ZIP,
 *  C_PHONE, C_SINCE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, and C_BALANCE are retrieved.
 *  C_BALANCE is decreased by H_AMOYNT. C_YTD_PAYMENT is increased by H_AMOYNT.
 *  C_PAYMENT_CNT is incremented by 1.
 * 
 *  Case 2: The customer is selected based on customer last name.
 *  All rows in the CUSTOMER table with matching C_W_ID, C_D_ID and C_LAST are selected
 *  sorted by C_FIRST in ascending order. Let (n) be the number of rows selected. 
 *  C_FIRST, C_MIDDLE, C_LAST, C_STREET_1, C_STREET_2, C_CITY, C_STATE, C_ZIP,
 *  C_PHONE, C_SINCE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, and C_BALANCE are retrieved
 *  from the row at position (n/2 rounded up the next integer) in the sorted set of
 *  selected rows from the CUSTOMER table. C_BALANCE is decreased by H_AMOYNT. 
 *  C_YTD_PAYMENT is increased by H_AMOYNT. C_PAYMENT_CNT is incremented by 1.
 */

class payment_upd_cust_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef payment_upd_cust_packet_t stage_packet_t;

    payment_upd_cust_stage_t();
    
    virtual ~payment_upd_cust_stage_t() { 

	TRACE(TRACE_ALWAYS, "PAYMENT_UPD_CUST destructor\n");
    }
    
}; // END OF CLASS: payment_upd_cust_stage_t



#endif
