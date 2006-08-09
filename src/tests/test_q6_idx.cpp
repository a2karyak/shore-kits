/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : test_q6.cpp
 *  @brief   : Unittest for Q6
 *  @version : 0.1
 *  @history :
 6/8/2006 : Updated to work with the new class definitions
 5/25/2006: Initial version
*/ 

#include "engine/thread.h"
#include "engine/dispatcher.h"
#include "engine/stages/iscan.h"
#include "engine/stages/sort.h"
#include "engine/stages/iprobe.h"
#include "engine/stages/fscan.h"
#include "engine/stages/merge.h"
#include "engine/stages/fdump.h"
#include "engine/stages/aggregate.h"
#include "engine/dispatcher/dispatcher_policy_os.h"
#include "engine/util/stopwatch.h"
#include "engine/util/time_util.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "workload/common.h"
#include "tests/common.h"
#include "workload/tpch/tpch_db.h"
#include "workload/tpch/tpch_compare.h"

#include "db_cxx.h"
#include "workload/common/predicates.h"

using namespace qpipe;

struct iscan_tuple {
    double L_EXTENDEDPRICE;
    double L_DISCOUNT;
};

// no need for the date range here because the index scan takes care of it
struct q6_iscan_filter_t : tuple_filter_t {
    and_predicate_t _filter;

    q6_iscan_filter_t(double discount, double qty)
        : tuple_filter_t(sizeof(tpch_lineitem_tuple))
    {
        predicate_t* p;
        size_t offset;

        // L_DISCOUNT between discount - .01 and discount + .01
        offset = offsetof(tpch_lineitem_tuple, L_DISCOUNT);
        p = new scalar_predicate_t<double, greater_equal>(discount - .01, offset);
        _filter.add(p);
        p = new scalar_predicate_t<double, less_equal>(discount + .01, offset);
        _filter.add(p);

        // L_QUANTITY < qty
        offset = offsetof(tpch_lineitem_tuple, L_QUANTITY);
        p = new scalar_predicate_t<double, less>(qty, offset);
        _filter.add(p);
    }
    virtual void project(tuple_t &d, const tuple_t &s) {
        iscan_tuple *dest = (iscan_tuple*) d.data;
        tpch_lineitem_tuple *src = (tpch_lineitem_tuple*) s.data;
        dest->L_EXTENDEDPRICE = src->L_EXTENDEDPRICE;
        dest->L_DISCOUNT = src->L_DISCOUNT;
    }
    virtual bool select(const tuple_t &t) {
        return _filter.select(t);
    }
    virtual q6_iscan_filter_t* clone() const {
        return new q6_iscan_filter_t(*this);
    }
};

struct lineitem_key_compare_t : key_compare_t {
    int operator()(const void* key1, const void* key2) {
        Dbt k1((void*) key1, 2*sizeof(int));
        Dbt k2((void*) key2, 2*sizeof(int));
        return tpch_lineitem_bt_compare_fcn(NULL, &k1, &k2);
    }
    lineitem_key_compare_t* clone() const {
        return new lineitem_key_compare_t(*this);
    }
};

struct lineitem_key_extractor_t : key_extractor_t {
    lineitem_key_extractor_t()
        : key_extractor_t(2*sizeof(int), 0)
    {
    }
    int extract_hint(const char* key) {
        int hint;
        memcpy(&hint, key, sizeof(int));
        return hint;
    }
    lineitem_key_extractor_t* clone() const {
        return new lineitem_key_extractor_t(*this);
    }
};





// no need for the date range here because the index scan takes care of it
struct print_filter_t : tuple_filter_t {

    print_filter_t()
        : tuple_filter_t(2*sizeof(int))
    {
    }

    // use default project...
    
    virtual bool select(const tuple_t &t) {
        if(0) {
            int* data = (int*)t.data;
            printf("%d\n", data[0]);
        }
        return true;
    }

    virtual print_filter_t* clone() const {
        return new print_filter_t();
    }
};





packet_t* create_q6_idx_packet(const c_str &client_prefix, dispatcher_policy_t* dp) {

    // choose a random year from 1993 to 1997 (inclusive)
    time_t date = datestr_to_timet("1993-01-01");

    // thread_t* self = thread_get_self();
    // int year = abs((int)(5*(float)(self->rand())/(float)(RAND_MAX+1)));

    date = time_add_year(date, 1); // 1994

    // now for a qty and discount
    double qty = 24;
    double discount = .06;

    dbt_guard_t start_key(sizeof(time_t));
    dbt_guard_t stop_key(sizeof(time_t));
    memcpy(start_key->get_data(), &date, sizeof(time_t));
    date = time_add_year(date, 1); // DATE + 1 year
    memcpy(stop_key->get_data(), &date, sizeof(time_t));
    
    
    // ISCAN
    tuple_fifo* iscan_output = new tuple_fifo(2*sizeof(int), dbenv);
    iscan_packet_t *q6_iscan_packet;
    c_str id("%s_ISCAN_PACKET", client_prefix.data());
    q6_iscan_packet = new iscan_packet_t(id,
                                         iscan_output,
                                         //new q6_iscan_filter_t(discount, qty),
                                         new trivial_filter_t(iscan_output->tuple_size()),
                                         //new print_filter_t(),
                                         tpch_lineitem_shipdate_idx,
                                         start_key, stop_key,
                                         tpch_lineitem_shipdate_compare_fcn
                                         );

    // SORT IDS
    sort_packet_t* q6_sort_packet;
    id = c_str("%s_SORT_PACKET", client_prefix.data());
    q6_sort_packet = new sort_packet_t(id, new tuple_fifo(iscan_output->tuple_size(), dbenv),
                                       //new trivial_filter_t(iscan_output->tuple_size),
                                       new print_filter_t(),
                                       new lineitem_key_extractor_t(),
                                       new lineitem_key_compare_t(),
                                       q6_iscan_packet);

    // PROBE IDS
    iprobe_packet_t* q6_probe_packet;
    id = c_str("%s_IPROBE_PACKET", client_prefix.data());
    q6_probe_packet = new iprobe_packet_t(id, new tuple_fifo(sizeof(iscan_tuple), dbenv),
                                          new q6_iscan_filter_t(discount, qty),
                                          q6_sort_packet, tpch_lineitem);
                                       
    // AGGREGATE
    id = c_str("%s_AGGREGATE_PACKET", client_prefix.data());
    aggregate_packet_t* q6_agg_packet;
    q6_agg_packet = new aggregate_packet_t(id,
                                           new tuple_fifo(2*sizeof(double), dbenv),
                                           new trivial_filter_t(2*sizeof(double)),
                                           new q6_count_aggregate_t(),
                                           new default_key_extractor_t(0),
                                           q6_probe_packet);
    
    dispatcher_policy_t::query_state_t* qs = dp->query_state_create();
    dp->assign_packet_to_cpu(q6_agg_packet, qs);
    dp->assign_packet_to_cpu(q6_iscan_packet, qs);
    dp->assign_packet_to_cpu(q6_sort_packet, qs);
    dp->assign_packet_to_cpu(q6_probe_packet, qs);
    dp->query_state_destroy(qs);

    if(0)
        return q6_iscan_packet;
    else
        return q6_agg_packet;
}


/** @fn    : main
 *  @brief : TPC-H Q6
 */

int main(int argc, char* argv[]) {

    thread_init();
    db_open(DB_RDONLY|DB_THREAD, 1, 600*1024*1024);
    dispatcher_policy_t* dp = new dispatcher_policy_os_t();
    TRACE_SET(TRACE_ALWAYS);

    
    // parse command line args
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <num iterations>\n", argv[0]);
	exit(-1);
    }
    int num_iterations = atoi(argv[1]);
    if ( num_iterations == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid num iterations %s\n", argv[1]);
	exit(-1);
    }


    register_stage<iscan_stage_t>(1);
    register_stage<aggregate_stage_t>(1);
    register_stage<sort_stage_t>();
    register_stage<iprobe_stage_t>();
    register_stage<fscan_stage_t>(10);
    register_stage<merge_stage_t>(10);
    register_stage<fdump_stage_t>(10);
    

    for(int i=0; i < num_iterations; i++) {
        stopwatch_t timer;
        
        
        packet_t* q6_packet = create_q6_idx_packet("Q6", dp);

        tuple_fifo* output_buffer = q6_packet->output_buffer();
        dispatcher_t::dispatch_packet(q6_packet);
    
        tuple_t output;
        int count;
        for(count=0; output_buffer->get_tuple(output); count++);
        printf("Count: %d\n", count);
        if(0)
        while(output_buffer->get_tuple(output)) {
            double* r = (double*)output.data;
            TRACE(TRACE_ALWAYS, "*** Q6 Count: %u. Sum: %lf.  ***\n", (unsigned)r[0], r[1]);
        }
        
        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }
    
 
    db_close();
    return 0;
}
