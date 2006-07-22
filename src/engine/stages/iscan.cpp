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

#include "engine/stages/iscan.h"
#include "engine/util/guard.h"
#include "engine/core/exception.h"
#include "qpipe_panic.h"
#include <unistd.h>


const c_str iscan_packet_t::PACKET_TYPE = "ISCAN";



const c_str iscan_stage_t::DEFAULT_STAGE_NAME = "ISCAN_STAGE";



#define KB 1024
#define MB 1024 * KB

/**
 *  @brief BerkeleyDB cannot read into page_t's. Allocate a large blob
 *  and do bulk reading. The blob must be aligned for int accesses and
 *  a multiple of 1024 bytes long.
 */
const size_t iscan_stage_t::ISCAN_READ_BUFFER_SIZE=256;



/**
 *  @brief Scan the specified index for a given range of keys.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

void iscan_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    iscan_packet_t* packet = (iscan_packet_t*)adaptor->get_packet();

    Db*  db = packet->_db;
    cursor_guard_t cursor(db);
    iscan_packet_t::bt_compare_func_t cmp = packet->_bt_compare;

    // BerkeleyDB cannot read into page_t's. Allocate a large blob and
    // do bulk reading.
    dbt_guard_t data(ISCAN_READ_BUFFER_SIZE);
    dbt_guard_t start_key = packet->_start_key;
    
    dbt_guard_t stop_key = packet->_stop_key;
    int err = cursor->get(start_key, data, DB_SET_RANGE);
    for (int bulk_read_index = 0; !err; bulk_read_index++) {
        if(cmp(db, start_key, stop_key) >= 0)
            break; // out of range!

        adaptor->output(*data);
        err = cursor->get(start_key, data, DB_NEXT);
    }

    // any other error should have been wrapped up and thrown already
    if(err)
        assert(err == DB_NOTFOUND);
}
