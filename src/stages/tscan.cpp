/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/tscan.h"
#include <unistd.h>



const c_str tscan_packet_t::PACKET_TYPE = "TSCAN";



const c_str tscan_stage_t::DEFAULT_STAGE_NAME = "TSCAN_STAGE";



#define KB 1024
#define MB 1024 * KB

/**
 *  @brief BerkeleyDB cannot read into page_t's. Allocate a large blob
 *  and do bulk reading. The blob must be aligned for int accesses and
 *  a multiple of 1024 bytes long.
 */
const size_t tscan_stage_t::TSCAN_BULK_READ_BUFFER_SIZE=256*KB;

/**
 *  @brief Read the specified table.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

void tscan_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    tscan_packet_t* packet = (tscan_packet_t*)adaptor->get_packet();

    Db*  db = packet->_db;
    cursor_guard_t cursor(db);
    

    // BerkeleyDB cannot read into page_t's. Allocate a large blob and
    // do bulk reading.
    dbt_guard_t bulk_data(TSCAN_BULK_READ_BUFFER_SIZE);
    Dbt bulk_key;
    for (int bulk_read_index = 0; ; bulk_read_index++) {

        // any return code besides DB_NOTFOUND would throw an exception
        int err = cursor->get(&bulk_key, bulk_data, DB_MULTIPLE_KEY | DB_NEXT);
        if(err)
            THROW2(BdbException, "cursor->get() returned %d\n", err);
        
        // iterate over the blob we read and output the individual
        // tuples
#if 0
        Dbt key, data;
        DbMultipleKeyDataIterator it = bulk_data.get();
	for (int tuple_index = 0; it.next(key, data); tuple_index++) {
            tuple_t tup((char*)data.get_data(), (size_t)data.get_size());
	    adaptor->output(tup);
        }
#else
        Dbt data;
        DbMultipleDataIterator it = bulk_data.get();
	for (int tuple_index = 0; it.next(data); tuple_index++) {
            tuple_t tup((char*)data.get_data(), (size_t)data.get_size());
	    adaptor->output(tup);
        }
#endif
    }

    // control never reaches here
    assert(false);
}
