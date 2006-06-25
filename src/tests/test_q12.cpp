// -*- mode:C++; c-basic-offset:4 -*-
#include "engine/stages/tscan.h"
#include "engine/stages/partial_aggregate.h"
#include "engine/stages/hash_join.h"
#include "engine/dispatcher.h"
#include "engine/util/stopwatch.h"
#include "engine/util/time_util.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common.h"
#include "tests/common/tpch_db.h"

// experimental!
#include "predicates.h"

using namespace qpipe;

/*
 * select
 *     l_shipmode,
 *     sum(case
 *         when o_orderpriority ='1-URGENT'
 *             or o_orderpriority ='2-HIGH'
 *         then 1
 *         else 0
 *     end) as high_line_count,
 *     sum(case
 *         when o_orderpriority <> '1-URGENT'
 *             and o_orderpriority <> '2-HIGH'
 *         then 1
 *         else 0
 *         end) as low_line_count
 * from
 *     tpcd.orders,
 *     tpcd.lineitem
 * where
 *     o_orderkey = l_orderkey
 *     and l_shipmode in ('MAIL', 'SHIP')
 *     and l_commitdate < l_receiptdate
 *     and l_shipdate < l_commitdate
 *     and l_receiptdate >= date('1994-01-01')
 *     and l_receiptdate < date('1994-01-01') + 1 year
 * group by
 *     l_shipmode
 * order by
 *     l_shipmode;
 */

struct lineitem_scan_tuple {
    int L_ORDERKEY;
    tpch_l_shipmode L_SHIPMODE;
};

struct order_scan_tuple {
    int O_ORDERKEY;
    tpch_o_orderpriority O_ORDERPRIORITY;
};

struct join_tuple {
    tpch_l_shipmode L_SHIPMODE;
    tpch_o_orderpriority O_ORDERPRIORITY;
};

struct q12_tuple {
    tpch_l_shipmode L_SHIPMODE;
    int HIGH_LINE_COUNT;
    int LOW_LINE_COUNT;
};

/**
 * @brief select O_ORDERKEY, O_ORDERPRIORITY from ORDERS
 */
struct order_tscan_filter_t : tuple_filter_t {
    order_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_orders_tuple))
    {
    }
    virtual void project(tuple_t &d, const tuple_t &s) {
        order_scan_tuple* dest = (order_scan_tuple*) d.data;
        tpch_orders_tuple* src = (tpch_orders_tuple*) s.data;
        dest->O_ORDERKEY = src->O_ORDERKEY;
        dest->O_ORDERPRIORITY = src->O_ORDERPRIORITY;
    }
    virtual order_tscan_filter_t* clone() {
        return new order_tscan_filter_t(*this);
    }
};

/**
 * @brief select L_ORDERKEY, L_SHIPMODE from LINEITEM where L_SHIPMODE
 * in ('MAIL', 'SHIP') and L_COMMITDATE < L_RECEIPTDATE and L_SHIPDATE
 * < L_COMMITDATE and L_RECEIPTDATE >= [date] and L_RECEIPTDATE <
 * [date] + 1 year
 */
struct lineitem_tscan_filter_t : tuple_filter_t {
    and_predicate_t _filter;
    lineitem_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_lineitem_tuple))
    {
        size_t offset;
        predicate_t* p;

        // where L_SHIPMODE in ('MAIL', 'SHIP') ...
        offset = FIELD_OFFSET(tpch_lineitem_tuple, L_SHIPMODE);
        or_predicate_t* orp = new or_predicate_t();
        p = new scalar_predicate_t<tpch_l_shipmode>(MAIL, offset);
        orp->add(p);
        p = new scalar_predicate_t<tpch_l_shipmode>(SHIP, offset);
        orp->add(p);
        _filter.add(orp);

        // ... and L_COMMITDATE < L_RECEIPTDATE ...
        size_t offset1 = FIELD_OFFSET(tpch_lineitem_tuple, L_COMMITDATE);
        size_t offset2 = FIELD_OFFSET(tpch_lineitem_tuple, L_RECEIPTDATE);
        p = new field_predicate_t<time_t, less>(offset1, offset2);
        _filter.add(p);

        // ... and L_SHIPDATE < L_COMMITDATE ...
        offset1 = FIELD_OFFSET(tpch_lineitem_tuple, L_SHIPDATE);
        offset2 = FIELD_OFFSET(tpch_lineitem_tuple, L_COMMITDATE);
        p = new field_predicate_t<time_t, less>(offset1, offset2);
        _filter.add(p);

        // ... and L_RECEIPTDATE >= [date] ...
        time_t date = datestr_to_timet("1994-01-01");
        offset = FIELD_OFFSET(tpch_lineitem_tuple, L_RECEIPTDATE);
        p = new scalar_predicate_t<time_t, greater_equal>(date, offset);
        _filter.add(p);

        // ... and L_RECEIPTDATE < [date] + 1 year
        date = time_add_year(date, 1);
        p = new scalar_predicate_t<time_t, less>(date, offset);
        _filter.add(p);
    }
    virtual void project(tuple_t &d, const tuple_t &s) {
        lineitem_scan_tuple *dest = (lineitem_scan_tuple*) d.data;
        tpch_lineitem_tuple *src = (tpch_lineitem_tuple*) s.data;
        dest->L_ORDERKEY = src->L_ORDERKEY;
        dest->L_SHIPMODE = src->L_SHIPMODE;
    }
    virtual bool select(const tuple_t &t) {
        return _filter.select(t);
    }
    virtual lineitem_tscan_filter_t* clone() {
        return new lineitem_tscan_filter_t(*this);
    }
};

/**
 * @brief ... from lineitem, orders where L_ORDERKEY = O_ORDERKEY ...
 */
struct q12_join_t : tuple_join_t {
    static key_extractor_t* create_left_extractor() {
        size_t offset = FIELD_OFFSET(order_scan_tuple, O_ORDERKEY);
        return new int_key_extractor_t(sizeof(int), offset);
    }
    static key_extractor_t* create_right_extractor() {
        size_t offset = FIELD_OFFSET(lineitem_scan_tuple, L_ORDERKEY);
        return new int_key_extractor_t(sizeof(int), offset);
    }
    q12_join_t()
        : tuple_join_t(sizeof(order_scan_tuple), create_left_extractor(),
                       sizeof(lineitem_scan_tuple), create_right_extractor(),
                       new int_key_compare_t(), sizeof(join_tuple))
    {
    }
    virtual void join(tuple_t &d, const tuple_t &l, const tuple_t &r) {
        join_tuple* dest = (join_tuple*) d.data;
        order_scan_tuple* left = (order_scan_tuple*) l.data;
        lineitem_scan_tuple* right = (lineitem_scan_tuple*) r.data;
        
        // cheat and filter out the join key here
        dest->L_SHIPMODE = right->L_SHIPMODE;
        dest->O_ORDERPRIORITY = left->O_ORDERPRIORITY;
    }
    virtual q12_join_t* clone() {
        return new q12_join_t(*this);
    }
};

/**
 * @brief sum(case
 *         when o_orderpriority ='1-URGENT'
 *             or o_orderpriority ='2-HIGH'
 *         then 1
 *         else 0
 *     end) as high_line_count,
 *     sum(case
 *         when o_orderpriority <> '1-URGENT'
 *             and o_orderpriority <> '2-HIGH'
 *         then 1
 *         else 0
 *         end) as low_line_count
 */
struct q12_aggregate_t : tuple_aggregate_t {
    default_key_extractor_t _extractor;
    or_predicate_t _filter;

    q12_aggregate_t()
        : tuple_aggregate_t(sizeof(q12_tuple)),
          _extractor(sizeof(tpch_l_shipmode),
                     FIELD_OFFSET(q12_tuple, L_SHIPMODE))
    {
        size_t offset;
        predicate_t* p;

        // case when O_ORDERPRIORITY = '1-URGENT' or O_ORDERPRIORITY =
        // '2-HIGH' then 1 else 0 end
        offset = FIELD_OFFSET(join_tuple, O_ORDERPRIORITY);
        p = new scalar_predicate_t<tpch_o_orderpriority>(URGENT_1, offset);
        _filter.add(p);
        p = new scalar_predicate_t<tpch_o_orderpriority>(HIGH_2, offset);
        _filter.add(p);
    }

    virtual key_extractor_t* key_extractor() {
        return &_extractor;
    }
    virtual void aggregate(char* agg_data, const tuple_t &t) {
        q12_tuple* agg = (q12_tuple*) agg_data;

        if(_filter.select(t))
            agg->HIGH_LINE_COUNT++;
        else
            agg->LOW_LINE_COUNT++;
    }
    virtual void finish(tuple_t &dest, const char* agg_data) {
        memcpy(dest.data, agg_data, dest.size);
    }
    virtual q12_aggregate_t* clone() {
        return new q12_aggregate_t(*this);
    }
};

int main() {
    thread_init();
    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        

    trace_current_setting = TRACE_ALWAYS;


    // line up the stages...
    register_stage<tscan_stage_t>();
    register_stage<partial_aggregate_stage_t>();
    register_stage<hash_join_stage_t>();


    for(int i=0; i < 5; i++) {

        stopwatch_t timer;

        // lineitem scan
        char* packet_id = copy_string("lineitem TSCAN");
        tuple_filter_t* filter = new lineitem_tscan_filter_t();
        tuple_buffer_t* buffer = new tuple_buffer_t(sizeof(lineitem_scan_tuple));
        packet_t* lineitem_packet;
        lineitem_packet = new tscan_packet_t(packet_id,
                                             buffer,
                                             filter,
                                             tpch_lineitem);
        // order scan
        packet_id = copy_string("order TSCAN");
        filter = new order_tscan_filter_t();
        buffer = new tuple_buffer_t(sizeof(order_scan_tuple));
        packet_t* order_packet;
        order_packet = new tscan_packet_t(packet_id,
                                          buffer, filter,
                                          tpch_orders);

        // join
        packet_id = copy_string("orders-lineitem HJOIN");
        filter = new trivial_filter_t(sizeof(join_tuple));
        buffer = new tuple_buffer_t(sizeof(join_tuple));
        packet_t* join_packet;
        join_packet = new hash_join_packet_t(packet_id,
                                             buffer, filter,
                                             order_packet,
                                             lineitem_packet,
                                             new q12_join_t());

        // partial aggregation
        packet_id = copy_string("sum AGG");
        filter = new trivial_filter_t(sizeof(q12_tuple));
        buffer = new tuple_buffer_t(sizeof(q12_tuple));
        size_t offset = FIELD_OFFSET(join_tuple, L_SHIPMODE);
        key_extractor_t* extractor = new default_key_extractor_t(sizeof(int), offset);
        key_compare_t* compare = new int_key_compare_t();
        tuple_aggregate_t* aggregate = new q12_aggregate_t();
        packet_t* agg_packet;
        agg_packet = new partial_aggregate_packet_t(packet_id,
                                                    buffer, filter,
                                                    join_packet,
                                                    aggregate,
                                                    extractor,
                                                    compare);

        // go!
        dispatcher_t::dispatch_packet(agg_packet);
        
        tuple_t output;
        TRACE(TRACE_ALWAYS, "*** Q12 %10s %10s %10s\n",
              "Shipmode", "High_Count", "Low_Count");
        while(!buffer->get_tuple(output)) {
            q12_tuple* r = (q12_tuple*) output.data;
        TRACE(TRACE_ALWAYS, "*** Q12 %10s %10d %10d\n",
              (r->L_SHIPMODE == MAIL)? "MAIL" : "SHIP",
              r->HIGH_LINE_COUNT, r->LOW_LINE_COUNT);
        }

        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }
    
    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;

    
}
