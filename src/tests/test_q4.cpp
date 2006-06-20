// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : test_q4.cpp
 *  @brief   : Unittest for Q4
 *  @version : 0.1
 *  @history :
 6/16/2006: Initial version
*/ 

#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"
#include "engine/stages/sort.h"
#include "engine/stages/hash_join.h"
#include "engine/stages/fdump.h"
#include "engine/stages/fscan.h"
#include "engine/stages/hash_join.h"
#include "engine/dispatcher.h"
#include "engine/util/stopwatch.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common.h"

using namespace qpipe;






/**
 * @brief select distinct l_orderkey from lineitem where l_commitdate < l_receiptdate
 */
packet_t *line_item_scan(Db* tpch_lineitem) {
    struct lineitem_tscan_filter_t : public tuple_filter_t {
        /* Initialize the predicates */
        lineitem_tscan_filter_t()
            : tuple_filter_t(sizeof(tpch_lineitem_tuple))
        {
        }

        /* Predication */
        virtual bool select(const tuple_t &input) {
            tpch_lineitem_tuple *tuple = (tpch_lineitem_tuple*)input.data;
            return tuple->L_COMMITDATE < tuple->L_RECEIPTDATE;
        }
    
        /* Projection */
        virtual void project(tuple_t &dest, const tuple_t &src) {
            /* Should project  L_ORDERKEY */
            tpch_lineitem_tuple *at = (tpch_lineitem_tuple*)(src.data);
            memcpy(dest.data, &at->L_ORDERKEY, sizeof(int));
        }
        virtual lineitem_tscan_filter_t* clone() {
            return new lineitem_tscan_filter_t(*this);
        }
    };

    struct int_distinct_t : public tuple_aggregate_t {
        bool _first;
        int _last_value;
        int_distinct_t()
            : _first(true), _last_value(-1)
        {
        }
        virtual bool aggregate(tuple_t &d, const tuple_t &s) {
            int value = *(int*)s.data;
            bool broken = _first || value != _last_value;
            return broken? break_group(d, value) : false;
        }
        virtual bool eof(tuple_t  &d) {
            return break_group(d, -1);
        }
        bool break_group(tuple_t &d, int value) {
            // no tuples read previously?
            if(!_first) {
                int* dest = (int*) d.data;
                *dest = _last_value;
            }
            bool result = !_first;
            _last_value = value;
            _first = false;
            return result;
        }
    };
    
    // LINEITEM TSCAN PACKET
    // the output consists of 1 int (the
    char* packet_id = copy_string("Lineitem TSCAN");
    tuple_filter_t* filter = new lineitem_tscan_filter_t(); 
    tuple_buffer_t* buffer = new tuple_buffer_t(sizeof(int));
    packet_t *tscan_packet = new tscan_packet_t(packet_id,
                                                buffer,
                                                filter,
                                                tpch_lineitem);
#if 0    
    // sort as a precursor to the distinct aggregate
    packet_id = copy_string("Lineitem SORT");
    filter = new tuple_filter_t(sizeof(int));
    buffer = new tuple_buffer_t(sizeof(int));
    tuple_comparator_t *compare = new int_comparator_t();
    packet_t* sort_packet = new sort_packet_t(packet_id,
                                              buffer,
                                              filter,
                                              compare,
                                              tscan_packet);

    // now select distinct
    packet_id = copy_string("Lineitem DISTINCT");
    filter = new tuple_filter_t(sizeof(int));
    buffer = new tuple_buffer_t(sizeof(int));
    tuple_aggregate_t *aggregate = new int_distinct_t();
    packet_t* agg_packet = new aggregate_packet_t(packet_id,
                                                  buffer,
                                                  filter,
                                                  aggregate,
                                                  sort_packet);
#endif
    return tscan_packet;
}

/**
 * @brief holds the results of an order scan after projection
 */
struct order_scan_tuple_t {
    int O_ORDERKEY;
    int O_ORDERPRIORITY;
};

packet_t* orders_scan(Db* tpch_orders) {
    struct orders_tscan_filter_t : public tuple_filter_t {
        time_t t1, t2;
        
        /* Initialize the predicates */
        orders_tscan_filter_t()
            : tuple_filter_t(sizeof(tpch_orders_tuple))
        {
            // TODO: random predicates per the TPCH spec...
            t1 = datestr_to_timet("1993-07-01");
            t2 = datestr_to_timet("1993-10-01");
        }

        /* Predication */
        virtual bool select(const tuple_t &input) {
            tpch_orders_tuple *tuple = (tpch_orders_tuple*)input.data;
            return tuple->O_ORDERDATE >= t1 && tuple->O_ORDERDATE < t2;
        }
    
        /* Projection */
        virtual void project(tuple_t &d, const tuple_t &s) {
            /* Should project  O_ORDERKEY and O_ORDERPRIORITY*/
            tpch_orders_tuple* src = (tpch_orders_tuple*) s.data;
            order_scan_tuple_t* dest = (order_scan_tuple_t*) d.data;
            dest->O_ORDERKEY = src->O_ORDERKEY;
            dest->O_ORDERPRIORITY = src->O_ORDERPRIORITY;
        }
        virtual orders_tscan_filter_t* clone() {
            return new orders_tscan_filter_t(*this);
        }
    };

    char* packet_id = copy_string("Orders TSCAN");
    tuple_filter_t* filter = new orders_tscan_filter_t(); 
    tuple_buffer_t* buffer = new tuple_buffer_t(sizeof(order_scan_tuple_t));
    packet_t *tscan_packet = new tscan_packet_t(packet_id,
                                                buffer,
                                                filter,
                                                tpch_orders);
    
    return tscan_packet;
}

// left is lineitem, right is orders
struct q4_join_t : public tuple_join_t {
    struct left_key_extractor_t : public key_extractor_t {
        virtual void extract_key(void* key, const void* tuple_data) {
            order_scan_tuple_t* tuple = (order_scan_tuple_t*) tuple_data;
            memcpy(key, &tuple->O_ORDERKEY, key_size());
        }
        virtual left_key_extractor_t* clone() {
            return new left_key_extractor_t(*this);
        }
    };
    
    struct right_key_extractor_t : public key_extractor_t {
        virtual void extract_key(void* key, const void* tuple_data) {
            memcpy(key, tuple_data, key_size());
        }
        virtual right_key_extractor_t* clone() {
            return new right_key_extractor_t(*this);
        }
    };
    
    q4_join_t()
        : tuple_join_t(sizeof(order_scan_tuple_t), new left_key_extractor_t(),
                       sizeof(int), new right_key_extractor_t(),
                       new int_key_compare_t(),
                       sizeof(int))
    {
    }
    virtual void join(tuple_t &dest,
                      const tuple_t &left,
                      const tuple_t &)
    {
        // KLUDGE: this projection should go in a separate filter class
        order_scan_tuple_t* tuple = (order_scan_tuple_t*) left.data;
        memcpy(dest.data, &tuple->O_ORDERPRIORITY, sizeof(int));
#if 0
        int* ltup = (int*)left.data;
        TRACE(TRACE_ALWAYS, "Joined L_ORDERKEY %d with O_ORDERKEY = %d, O_ORDERPRIORITY = %d\n",
              *ltup,
              tuple->O_ORDERKEY,
              tuple->O_ORDERPRIORITY);
#endif
    }
};

struct q4_tuple_t {
    int O_ORDERPRIORITY;
    int ORDER_COUNT;
};

struct q4_count_aggregate_t : public tuple_aggregate_t {
    bool _first;
    int  _last_value;
    int  _count;
    q4_count_aggregate_t()
        : _first(true), _last_value(-1), _count(0)
    {
    }
    virtual bool aggregate(tuple_t &d, const tuple_t &s) {

        int value = *(int*)s.data;

        if ( _first ) {
            // no tuple yet
            _first = false;
            _last_value = value;
            _count = 1;
            return false;
        }
        
        if ( value == _last_value ) {
            // keep counting
            _count++;
            return false;
        }

        // Otherwise, we are seeing a group break. Produce the count
        // and the value.
        q4_tuple_t* dest = (q4_tuple_t*) d.data;
        dest->O_ORDERPRIORITY = _last_value;
        dest->ORDER_COUNT = _count;
        _last_value = value;
        _count = 1;
        return true;
    }

    virtual bool eof(tuple_t  &d) {
        if ( _first )
            // we've seen no tuples!
            return false;

        q4_tuple_t* dest = (q4_tuple_t*) d.data;
        dest->O_ORDERPRIORITY = _last_value;
        dest->ORDER_COUNT = _count;
        return true;
    }
};

/** @fn    : main
 *  @brief : TPC-H Q6
 */

int main() {

    thread_init();
    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        

    trace_current_setting = TRACE_ALWAYS;


    // line up the stages...
    register_stage<tscan_stage_t>(2);
    register_stage<aggregate_stage_t>(2);
    register_stage<sort_stage_t>(2);
    register_stage<merge_stage_t>(10);
    register_stage<fdump_stage_t>(10);
    register_stage<fscan_stage_t>(20);
    register_stage<hash_join_stage_t>(1);


    for(int i=0; i < 5; i++) {

        stopwatch_t timer;

        /*
         * Query 4 original:
         *
         * select o_orderpriority, count(*) as order_count
         * from orders
         * where o_order_data <in range> and exists
         *      (select * from lineitem
         *       where l_orderkey = o_orderkey and l_commitdate < l_receiptdate)
         * group by o_orderpriority
         * order by o_orderpriority
         *
         *
         * Query 4 modified to make the nested query cleaner:
         *
         * select o_orderpriority, count(*) as order_count
         * from orders natural join (
         *      select distinct l_orderkey
         *      from lineitem
         *      where l_commitdate < l_receiptdate)
         */
        
        tuple_buffer_t* buffer;

        // First deal with the lineitem half

        packet_t *line_item_packet = line_item_scan(tpch_lineitem);
        
        // Now, the orders half
        packet_t* orders_packet = orders_scan(tpch_orders);

        // join them...
        char *packet_id = copy_string("Orders - Lineitem JOIN");
        tuple_filter_t* filter = new trivial_filter_t(sizeof(int));
        buffer = new tuple_buffer_t(sizeof(int));
        tuple_join_t* join = new q4_join_t();
        packet_t* join_packet = new hash_join_packet_t(packet_id,
                                                       buffer,
                                                       filter,
                                                       orders_packet,
                                                       line_item_packet,
                                                       join,
                                                       false,
                                                       true);

#if 0
        // sort as a precursor to the count aggregate
        packet_id = copy_string("Orders.O_ORDERPRIORITY SORT");
        filter = new tuple_filter_t(sizeof(int));
        buffer = new tuple_buffer_t(sizeof(int));
        tuple_comparator_t *compare = new int_comparator_t();
        packet_t* sort_packet = new sort_packet_t(packet_id,
                                                  buffer,
                                                  filter,
                                                  compare,
                                                  join_packet);
        
        // count aggregate
        packet_id = copy_string("O_ORDERPRIORITY COUNT");
        filter = new tuple_filter_t(sizeof(q4_tuple_t));
        buffer = new tuple_buffer_t(sizeof(q4_tuple_t));
        tuple_aggregate_t *aggregate = new q4_count_aggregate_t();
        packet_t* agg_packet = new aggregate_packet_t(packet_id,
                                                      buffer,
                                                      filter,
                                                      aggregate,
                                                      sort_packet);
#endif   
        // Dispatch packet
        dispatcher_t::dispatch_packet(join_packet);
        
        tuple_t output;
        while(!buffer->get_tuple(output)) {
            q4_tuple_t* r = (q4_tuple_t*) output.data;
            //            TRACE(TRACE_ALWAYS, "*** Q4 Priority: %d. Count: %d.  ***\n",
            //    r->O_ORDERPRIORITY, r->ORDER_COUNT);
        }
        


        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }

    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
