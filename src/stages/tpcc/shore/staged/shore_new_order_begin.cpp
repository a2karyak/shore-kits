/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_new_order_begin.cpp
 *
 *  @brief Implementation of the tpc-c shore_new_order_begin stage
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/staged/shore_new_order_begin.h"
#include "workload/common.h"



const c_str shore_new_order_begin_packet_t::PACKET_TYPE = "SHORE_NEW_ORDER_BEGIN";

const c_str shore_new_order_begin_stage_t::DEFAULT_STAGE_NAME = "SHORE_NEW_ORDER_BEGIN_STAGE";


int shore_new_order_begin_stage_t::_trx_counter = 0;
tatas_lock shore_new_order_begin_stage_t::_trx_counter_lock;


/**
 *  @brief Class for processing the results of the shore_new_order_begin
 */

class shore_new_order_begin_process_tuple_t : public process_tuple_t 
{
public:

    void begin() {
        TRACE( TRACE_DEBUG, "*** SHORE_NEW_ORDER_BEGIN RESULTS\n" );
    }

    void process(const tuple_t& output) {
        trx_result_tuple_t* r = aligned_cast<trx_result_tuple_t>(output.data);
        TRACE(TRACE_QUERY_RESULTS, "*** SHORE_NEW_ORDER_BEGIN TRX=%d RESULT=%s\n",
              r->get_id(),
              (r->get_state() == COMMITTED) ? "OK" : "ABORT");
        
    }        
};


/**
 *  @brief shore_new_order_begin_stage_t constructor
 */

shore_new_order_begin_stage_t::shore_new_order_begin_stage_t() 
{    
    //    TRACE(TRACE_DEBUG, "SHORE_NEW_ORDER_BEGIN constructor\n");
}


/**
 *  @brief Initialize transaction specified by shore_new_order_begin_packet_t p.
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void shore_new_order_begin_stage_t::process_packet() 
{
    adaptor_t* adaptor = _adaptor;

    shore_new_order_begin_packet_t* packet = 
	(shore_new_order_begin_packet_t*)adaptor->get_packet();

    /* STEP 1: Assigns a unique id */
    int my_trx_id;
    my_trx_id = get_next_counter();
    packet->set_trx_id(my_trx_id);

    /* Prints out the packet info */
    //    packet->describe_trx();

    /* STEP 2: Submits appropriate shore_new_order_* packets */
    
    /** FIXME: (ip) Scheduler policy?
     *  by default is set to OS  */
    scheduler::policy_t* dp = new scheduler::policy_os_t();

    // go!
    qpipe::query_state_t* qs = dp->query_state_create();

    /** @note The output buffers of all the non-finalize stages, which is
     *  the input to the finalize stage, are simple integers 
     */

    // 2a. SHORE_NEW_ORDER_OUTSIDE_LOOP
    tuple_fifo* buffer = new tuple_fifo(sizeof(int));
    tuple_filter_t* filter = new trivial_filter_t(sizeof(int));

    trx_packet_t* outside_loop_packet = 
        create_shore_new_order_outside_loop_packet( packet->_packet_id,
                                                    buffer,
                                                    filter,
                                                    packet->get_trx_id(),
                                                    &packet->_no_in);

    // 2b. SHORE_NEW_ORDER_ONE_OL
    // create ol_cnt packets
    typedef trx_packet_t* ptrx_packets;
    register int ol_cnt = packet->_no_in._ol_cnt;
    ptrx_packets* ol_packets = new ptrx_packets[packet->_no_in._ol_cnt];

    for (int i=0; i<ol_cnt; i++) {
    
        buffer = new tuple_fifo(sizeof(int));
        filter = new trivial_filter_t(sizeof(int));

        ol_packets[i] = 
            create_shore_new_order_one_ol_packet( packet->_packet_id,
                                                  buffer,
                                                  filter,
                                                  packet->get_trx_id(),
                                                  &packet->_no_in.items[i]);
        ol_packets[i]->assign_query_state(qs);
    } 

    // 2c. SHORE_NEW_ORDER_FINALIZE
    buffer = new tuple_fifo(sizeof(int));
    filter = new trivial_filter_t(sizeof(int));

    trx_packet_t* finalize_packet = 
        create_shore_new_order_finalize_packet( packet->_packet_id,
                                                buffer,
                                                filter,
                                                packet->get_trx_id(),
                                                outside_loop_packet,
                                                ol_packets,
                                                ol_cnt);

    finalize_packet->assign_query_state(qs);
    outside_loop_packet->assign_query_state(qs);


    // start processing packets
    trx_result_process_tuple_t rpt(finalize_packet);
    process_query(finalize_packet, rpt);

    dp->query_state_destroy(qs);
    
    // writing output 

    // create output tuple
    // "I" own tup, so allocate space for it in the stack
    size_t dest_size = packet->output_buffer()->tuple_size();
    
    array_guard_t<char> dest_data = new char[dest_size];
    tuple_t dest(dest_data, dest_size);
    
    
    trx_result_tuple_t aTrxResultTuple(COMMITTED, my_trx_id);
    
    trx_result_tuple_t* dest_result_tuple;
    dest_result_tuple = aligned_cast<trx_result_tuple_t>(dest.data);
    
    *dest_result_tuple = aTrxResultTuple;
    
    adaptor->output(dest);


} // process_packet




/** @fn get_next_counter
 *  @brief Using the mutex, increases the internal counter by 1 and returns it.
 */

int shore_new_order_begin_stage_t::get_next_counter() {

    CRITICAL_SECTION( ccs, _trx_counter_lock);
    return (++_trx_counter);
}


/** SHORE_NEW_ORDER_BEGIN packet creation functions */


/** @fn  create_shore_new_order_upd_wh_packet_t
 *  @brief Retuns a shore_new_order_upd_wh_packet_t 
 */

trx_packet_t* 
shore_new_order_begin_stage_t::create_shore_new_order_outside_loop_packet(const c_str& client_prefix,
                                                                          tuple_fifo* iout_buffer,
                                                                          tuple_filter_t* iout_filter,
                                                                          int a_trx_id,
                                                                          new_order_input_t* p_noin)
{
    c_str packet_name("%s_shore_new_order_upd_wh", client_prefix.data());

    shore_new_order_outside_loop_packet_t* shore_new_order_outside_loop_packet =
        new shore_new_order_outside_loop_packet_t(packet_name,
                                                  iout_buffer,
                                                  iout_filter,
                                                  a_trx_id,
                                                  p_noin);

    return (shore_new_order_outside_loop_packet);
}


/** @fn  create_shore_new_order_upd_distr_packet_t
 *  @brief Returns a shore_new_order_upd_distr_packet_t 
 */
 
trx_packet_t* 
shore_new_order_begin_stage_t::create_shore_new_order_one_ol_packet(const c_str& client_prefix,
                                                                    tuple_fifo* iol_buffer,
                                                                    tuple_filter_t* iol_filter,
                                                                    int a_trx_id,
                                                                    ol_item_info* p_nolin)
{
    c_str packet_name("%s_shore_new_order_one_ol", client_prefix.data());
    
    shore_new_order_one_ol_packet_t* shore_new_order_one_ol_packet =
        new shore_new_order_one_ol_packet_t(packet_name,
                                            iol_buffer,
                                            iol_filter,
                                            a_trx_id,
                                            p_nolin);
    
    return (shore_new_order_one_ol_packet);
}


/** @fn  create_shore_new_order_finalize_packet_t
 *  @brief Returns a shore_new_order_finalize_packet_t
 */

trx_packet_t* 
shore_new_order_begin_stage_t::create_shore_new_order_finalize_packet(const c_str& client_prefix,
                                                                      tuple_fifo* fin_buffer,
                                                                      tuple_filter_t* fin_filter,
                                                                      int a_trx_id,
                                                                      trx_packet_t* outside_loop,
                                                                      trx_packet_t** ol_packets,
                                                                      int ol_cnt)
{
    c_str packet_name("%s_shore_new_order_finalize", client_prefix.data());

    shore_new_order_finalize_packet_t* shore_new_order_finalize_packet =
        new shore_new_order_finalize_packet_t(packet_name,
                                              fin_buffer,
                                              fin_filter,
                                              a_trx_id,
                                              outside_loop,
                                              ol_packets,
                                              ol_cnt);
    
    return (shore_new_order_finalize_packet);
}

