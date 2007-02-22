/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "workload/tpch/tpch_db.h"
#include "tests/common.h"
#include "workload/common/register_stage.h"
#include "workload/process_query.h"

using namespace qpipe;
using namespace workload;


class test_fdump_stage_process_tuple_t : public process_tuple_t {
private:
    bool _got_tuple;

public:

    test_fdump_stage_process_tuple_t()
        : _got_tuple(false)
    {
    }
     
    virtual void process(const tuple_t&) {
        _got_tuple = true;
    }
    
    virtual void end() {
        if (_got_tuple)
            TRACE(TRACE_ALWAYS, "Got a tuple!\n");
        else
            TRACE(TRACE_ALWAYS, "Done with no tuples\n");
        TRACE(TRACE_ALWAYS, "TEST DONE\n");
    }
};


int main(int argc, char* argv[]) {

    thread_init();
    db_open();
    
    
    // parse output filename
    if ( argc < 3 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <output file> <tuple count>\n", argv[0]);
	exit(-1);
    }
    const char* output_filename = argv[1];
    int num_tuples = atoi(argv[2]);
    if ( num_tuples == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid tuple count %s\n", argv[2]);
	exit(-1);
    }


    register_stage<fdump_stage_t>(1);
    
    
    // just need to pass one int at a time to the counter
    tuple_fifo* int_buffer = new tuple_fifo(sizeof(int));
    tuple_fifo* signal_buffer = new tuple_fifo(sizeof(int));


    struct int_tuple_writer_info_s info(int_buffer, num_tuples);
    tester_thread_t* writer_thread =
	new tester_thread_t(int_tuple_writer_main, &info, "WRITER_THREAD");
    
    thread_create(writer_thread);
    
    // aggregate single count result (single int)
    fdump_packet_t* packet = 
	new fdump_packet_t("FDUMP_PACKET_1",
			   signal_buffer, 
                           new trivial_filter_t(int_buffer->tuple_size()),
			   int_buffer, 
			   output_filename);
    
    test_fdump_stage_process_tuple_t pt;
    process_query(packet, pt);

    return 0;
}
