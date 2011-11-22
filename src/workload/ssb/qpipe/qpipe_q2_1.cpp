/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/** @file:   qpipe_q2_1.cpp
 *
 *  @brief:  Implementation of QPIPE SSB Q2_1 over Shore-MT
 *
 *  @author: Iraklis Psaroudakis
 *  @date:   October 2011
 */

#include "workload/ssb/shore_ssb_env.h"
#include "qpipe.h"

using namespace shore;
using namespace qpipe;



ENTER_NAMESPACE(ssb);


/******************************************************************** 
 *
 * QPIPE Q2_1 - Structures needed by operators 
 *
 ********************************************************************/

struct qsupplier_tuple {
    int S_SUPPKEY;
    char S_NAME[STRSIZE(25)];
};

class q2_1_tscan_filter_t : public tuple_filter_t 
{
private:
    ShoreSSBEnv* _ssbdb;
    table_row_t* _prsupp;
    rep_row_t _rr;

    ssb_supplier_tuple _supplier;

public:

    q2_1_tscan_filter_t(ShoreSSBEnv* ssbdb, q2_1_input_t &in) 
        : tuple_filter_t(ssbdb->supplier_desc()->maxsize()), _ssbdb(ssbdb)
    {

    	// Get a supplier tupple from the tuple cache and allocate space
        _prsupp = _ssbdb->supplier_man()->get_tuple();
        _rr.set_ts(_ssbdb->supplier_man()->ts(),
                   _ssbdb->supplier_desc()->maxsize());
        _prsupp->_rep = &_rr;

    }

    ~q2_1_tscan_filter_t()
    {
        // Give back the supplier tuple 
        _ssbdb->supplier_man()->give_tuple(_prsupp);
    }


    // Predication
    bool select(const tuple_t &input) {

        // Get next supplier and read its shipdate
        if (!_ssbdb->supplier_man()->load(_prsupp, input.data)) {
            assert(false); // RC(se_WRONG_DISK_DATA)
        }

        return (true);
    }

    
    // Projection
    void project(tuple_t &d, const tuple_t &s) {        

        qsupplier_tuple *dest;
        dest = aligned_cast<qsupplier_tuple>(d.data);

        _prsupp->get_value(0, _supplier.S_SUPPKEY);
        _prsupp->get_value(1, _supplier.S_NAME,25);

        TRACE( TRACE_RECORD_FLOW, "%d|%s --d\n",
               _supplier.S_SUPPKEY,
               _supplier.S_NAME);

        dest->S_SUPPKEY = _supplier.S_SUPPKEY;
        strcpy(dest->S_NAME,_supplier.S_NAME);
    }

    q2_1_tscan_filter_t* clone() const {
        return new q2_1_tscan_filter_t(*this);
    }

    c_str to_string() const {
        return c_str("q2_1_tscan_filter_t()");
    }
};

class ssb_q2_1_process_tuple_t : public process_tuple_t 
{    
public:
        
    void begin() {
        TRACE(TRACE_QUERY_RESULTS, "*** q2_1 ANSWER ...\n");
        TRACE(TRACE_QUERY_RESULTS, "*** ...\n");
    }
    
    virtual void process(const tuple_t& output) {
        qsupplier_tuple *tuple;
        tuple = aligned_cast<qsupplier_tuple>(output.data);
        TRACE( TRACE_QUERY_RESULTS, "%d|%s --d\n",
               tuple->S_SUPPKEY,
               tuple->S_NAME);
    }
};



/******************************************************************** 
 *
 * QPIPE q2_1 - Packet creation and submission
 *
 ********************************************************************/

w_rc_t ShoreSSBEnv::xct_qpipe_q2_1(const int xct_id, 
                                  q2_1_input_t& in)
{
    TRACE( TRACE_ALWAYS, "********** q2_1 *********\n");

   
    policy_t* dp = this->get_sched_policy();
    xct_t* pxct = smthread_t::me()->xct();
    

    // TSCAN PACKET
    tuple_fifo* tscan_out_buffer =
        new tuple_fifo(sizeof(qsupplier_tuple));
        tscan_packet_t* supplier_tscan_packet =
        new tscan_packet_t("TSCAN SUPPLIER",
                           tscan_out_buffer,
                           new q2_1_tscan_filter_t(this,in),
                           this->db(),
                           _psupplier_desc.get(),
                           pxct
                           //, SH 
                           );

    qpipe::query_state_t* qs = dp->query_state_create();
    supplier_tscan_packet->assign_query_state(qs);
        
    // Dispatch packet
    ssb_q2_1_process_tuple_t pt;
    process_query(supplier_tscan_packet, pt);
    dp->query_state_destroy(qs);

    return (RCOK); 
}


EXIT_NAMESPACE(ssb);

