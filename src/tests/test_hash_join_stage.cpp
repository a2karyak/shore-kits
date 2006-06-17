// -*- mode:C++; c-basic-offset:4 -*-

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/func_call.h"
#include "engine/stages/hash_join.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common.h"

#include <vector>
#include <algorithm>

using std::vector;

using namespace qpipe;



int num_tuples = -1;


stage_t::result_t write_ints(void* arg)
{
    tuple_buffer_t* int_buffer = (tuple_buffer_t*)arg;
    

    vector<int> tuples;

    for (int i = 0; i < num_tuples; i++) {
        for(int j=0; j < i; j++)
            tuples.push_back(i);
    }
    std::random_shuffle(tuples.begin(), tuples.end());
    for (unsigned i=0; i < tuples.size(); i++) {
	tuple_t in_tuple((char*)&tuples[i], sizeof(int));
	if( int_buffer->put_tuple(in_tuple) ) {
	    TRACE(TRACE_ALWAYS, "tuple_page->append_init() returned non-zero!\n");
	    TRACE(TRACE_ALWAYS, "Terminating loop...\n");
	    break;
	}
    }
    
    return stage_t::RESULT_STOP;
}



class count_aggregate_t : public tuple_aggregate_t {

private:
    int count;
    
public:
  
    count_aggregate_t() {
        count = 0;
    }
  
    bool aggregate(tuple_t &, const tuple_t &) {
        count++;
        return false;
    }

    bool eof(tuple_t &dest) {
        *(int*)dest.data = count;
        return true;
    }
};

struct simple_join_t : public tuple_join_t {
    simple_join_t()
        : tuple_join_t(sizeof(int), sizeof(int), sizeof(int), sizeof(int))
    {
    }

    virtual void get_left_key(char *key, const tuple_t &tuple) {
        memcpy(key, tuple.data, left_tuple_size());
    }

    virtual void get_right_key(char *key, const tuple_t &tuple) {
        memcpy(key, tuple.data, right_tuple_size());
    }

    virtual void join(tuple_t &dest,
                      const tuple_t &left,
                      const tuple_t &)
    {
        dest.assign(left);
    }
};      


int main(int argc, char* argv[]) {

    thread_init();


    // parse output filename
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <tuple count>\n", argv[0]);
	exit(-1);
    }
    num_tuples = atoi(argv[1]);
    if ( num_tuples == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid tuple count %s\n", argv[1]);
	exit(-1);
    }


    // create two FUNC_CALL stages to feed the HASH_JOIN stage
    stage_container_t* sc = new stage_container_t("FUNC_CALL_CONTAINER", new stage_factory<func_call_stage_t>);
    dispatcher_t::register_stage_container(func_call_packet_t::PACKET_TYPE, sc);
    
    tester_thread_t* func_call_thread_left = 
	new tester_thread_t(drive_stage, sc, "FUNC_CALL_THREAD_LEFT");
    
    if ( thread_create( NULL, func_call_thread_left ) ) {
	TRACE(TRACE_ALWAYS, "thread_create() failed\n");
	QPIPE_PANIC();
    }

    tester_thread_t* func_call_thread_right = 
	new tester_thread_t(drive_stage, sc, "FUNC_CALL_THREAD_RIGHT");
    
    if ( thread_create( NULL, func_call_thread_right ) ) {
	TRACE(TRACE_ALWAYS, "thread_create() failed\n");
	QPIPE_PANIC();
    }


    stage_container_t* sc2 = new stage_container_t("HASH_JOIN_CONTAINER", new stage_factory<hash_join_stage_t>);
    dispatcher_t::register_stage_container(hash_join_packet_t::PACKET_TYPE, sc2);
    tester_thread_t* hash_join_thread = new tester_thread_t(drive_stage, sc2, "HASH JOIN THREAD");
    if ( thread_create( NULL, hash_join_thread ) ) {
        TRACE(TRACE_ALWAYS, "thread_create failed\n");
        QPIPE_PANIC();
    }


    simple_join_t *join = new simple_join_t();
    
    tuple_buffer_t* left_int_buffer = new tuple_buffer_t(join->left_tuple_size());
    char* left_packet_id;
    int left_packet_id_ret =
        asprintf( &left_packet_id, "LEFT_PACKET" );
    assert( left_packet_id_ret != -1 );

    func_call_packet_t* left_packet = 
	new func_call_packet_t(left_packet_id,
                               left_int_buffer, 
                               new tuple_filter_t(sizeof(int)), // unused, cannot be NULL
                               write_ints,
                               left_int_buffer);
    
    tuple_buffer_t* right_int_buffer = new tuple_buffer_t(join->right_tuple_size());
    char* right_packet_id;
    int right_packet_id_ret =
        asprintf( &right_packet_id, "RIGHT_PACKET" );
    assert( right_packet_id_ret != -1 );
    

    func_call_packet_t* right_packet = 
	new func_call_packet_t(right_packet_id,
                               right_int_buffer, 
                               new tuple_filter_t(sizeof(int)), // unused, cannot be NULL
                               write_ints,
                               right_int_buffer);
    
    
    tuple_buffer_t* join_buffer = new tuple_buffer_t(join->out_tuple_size());


    char* hash_join_packet_id;
    int hash_join_packet_id_ret =
        asprintf( &hash_join_packet_id, "HASH_JOIN_PACKET_1" );
    assert( hash_join_packet_id_ret != -1 );
 
    hash_join_packet_t* join_packet =
        new hash_join_packet_t( hash_join_packet_id,
                                join_buffer,
                                new tuple_filter_t(sizeof(int)),
                                left_packet,
                                right_packet,
                                join
                                );
    dispatcher_t::dispatch_packet(join_packet);
    
    tuple_t output;
    while(!join_buffer->get_tuple(output))
        TRACE(TRACE_ALWAYS, "Value: %d\n", *(int*)output.data);
    TRACE(TRACE_ALWAYS, "TEST DONE\n");

    return 0;
}
