/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/stages/tscan.h"
#include "engine/util/guard.h"
#include "qpipe_panic.h"
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
 * @brief A cursor guard that automatically closes the cursor when it
 * goes out of scope.
 */
struct cursor_guard_t {
    Db*  _db;
    Dbc* _cursor;

    cursor_guard_t(Db* db, Dbc* cursor)
        : _db(db), _cursor(cursor)
    {
    }
    
    ~cursor_guard_t() {
        int ret = _cursor->close();
        if(ret)
            _db->err(ret, "_cursor->close() failed: ");
    }
};



/**
 *  @brief Read the specified table.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

void tscan_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    tscan_packet_t* packet = (tscan_packet_t*)adaptor->get_packet();

    Dbc* dbcp;
    Db*  db = packet->_db;
    

    // open a cursor to the table
    int ret = db->cursor(NULL, &dbcp, 0);
    if (ret) {
        db->err(ret, "db->cursor() failed: ");
        throw qpipe_exception("Unable to open DB cursor");
    }
    cursor_guard_t cursor_guard(db, dbcp);
    

    // BerkeleyDB cannot read into page_t's. Allocate a large blob and
    // do bulk reading. The blob must be aligned for int accesses and a
    // multiple of 1024 bytes long.
    size_t bufsize = TSCAN_BULK_READ_BUFFER_SIZE / sizeof(int);
    array_guard_t<int> buffer = new int[bufsize];

    
    // initiates and sets the key and data variables for the bulk reading
    Dbt bulk_key, bulk_data;
    memset(&bulk_key,  0, sizeof(bulk_key));
    memset(&bulk_data, 0, sizeof(bulk_data));
    bulk_data.set_data(buffer);
    bulk_data.set_ulen(bufsize);
    bulk_data.set_flags(DB_DBT_USERMEM);


    for (int bulk_read_index = 0; ; bulk_read_index++) {

	int err = dbcp->get(&bulk_key, &bulk_data, DB_MULTIPLE_KEY | DB_NEXT);
	if (err) {
	    if (err != DB_NOTFOUND) {
		db->err(err, "dbcp->get() failed: ");
                throw qpipe_exception("Unable to read rows from DB cursor");
	    }
	    
	    // done reading table
	    return;
	}


        // iterate over the blob we read and output the individual
        // tuples
        Dbt key, data;
        DbMultipleKeyDataIterator it = bulk_data;
	for (int tuple_index = 0; it.next(key, data); tuple_index++) 
	    adaptor->output(data);
    }

    // control never reaches here
    assert(false);
}
