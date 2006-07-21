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
const size_t iscan_stage_t::ISCAN_BULK_READ_BUFFER_SIZE=256*KB;



/**
 *  @brief Read the specified table.
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
    dbt_guard_t bulk_data(ISCAN_BULK_READ_BUFFER_SIZE);
    dbt_guard_t start_key = packet->_start_key;
    
    dbt_guard_t stop_key = packet->_stop_key;
    bool first_time = true;
    for (int bulk_read_index = 0; ; bulk_read_index++) {

	int err;
        if(first_time)
            err = cursor->get(start_key, bulk_data, DB_MULTIPLE_KEY | DB_SET_RANGE);
        else
            err = cursor->get(start_key, bulk_data, DB_MULTIPLE_KEY | DB_NEXT);
        
	if (err) {
	    if (err != DB_NOTFOUND) {
		db->err(err, "dbcp->get() failed: ");
                throw EXCEPTION(Berkeley_DB_Exception, "Unable to read rows from DB cursor");
	    }
	    
	    // done reading table
	    return;
	}

        first_time = false;

        // iterate over the blob we read and output the individual
        // tuples
        Dbt key, data;
        DbMultipleKeyDataIterator it = *bulk_data;
	while(it.next(key, data)) {
            if(cmp(db, &key, stop_key) >= 0)
                return; // out of range!
            
	    adaptor->output(data);
        }
    }

    // control never reaches here
    assert(false);
}
