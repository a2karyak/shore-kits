// -*- mode:c++; c-basic-offset:4 -*-

/**
 * Reference material for using (secondary) indices in BDB:
 *      http://www.sleepycat.com/docs/ref/am/second.html
 *
 *
 * TPCH queries that can use indices:
 * 12: L_RECEIPTDATE
 * 14: L_SHIPDATE
 * 16: PS_PARTKEY, PS_SUPP_KEY
 *  4: O_ORDERDATE
 *  8: N_NATIONKEY, R_REGIONKEY, N_REGIONKEY
 * 13: C_CUSTKEY
 *  6: L_SHIPDATE
 */

#include "engine/stages/iprobe.h"
#include "engine/util/guard.h"
#include "engine/dispatcher.h"
#include "engine/core/exception.h"
#include "qpipe_panic.h"
#include <unistd.h>


const c_str iprobe_packet_t::PACKET_TYPE = "IPROBE";



const c_str iprobe_stage_t::DEFAULT_STAGE_NAME = "IPROBE_STAGE";



/**
 *  @brief Probes an index for each incoming key.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

void iprobe_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    iprobe_packet_t* packet = (iprobe_packet_t*)adaptor->get_packet();

    Db*  db = packet->_db;
    tuple_buffer_t* input_buffer = packet->_input_buffer;
    dispatcher_t::dispatch_packet(packet->_input);


    // "I" own dest, so allocate space for it on the stack
    size_t dest_size = packet->_output_buffer->tuple_size;
    char dest_data[dest_size];
    tuple_t dest(dest_data, dest_size);
    
    size_t tuple_size = packet->_output_filter->input_tuple_size();
    char tuple_data[tuple_size];
    Dbt tuple(tuple_data, tuple_size);
    tuple.set_flags(DB_DBT_USERMEM);
    tuple.set_ulen(tuple_size);

    while(1) {
        // no more tuples?
        tuple_t src;
        if(!input_buffer->get_tuple(src))
            return;

        Dbt key(src.data, src.size);
        int err = db->get(NULL, &key, &tuple, 0);
        if(err) 
            assert(err == DB_NOTFOUND);
        else
            adaptor->output(tuple);
    }
}
