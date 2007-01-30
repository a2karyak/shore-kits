/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "workload/tpch/drivers/tpch_q12.h"

#include "workload/common.h"
#include "workload/tpch/tpch_db.h"
#include "workload/common/predicates.h"

using namespace qpipe;

ENTER_NAMESPACE(workload);

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
        order_scan_tuple* dest = aligned_cast<order_scan_tuple>(d.data);
        tpch_orders_tuple* src = aligned_cast<tpch_orders_tuple>(s.data);
        dest->O_ORDERKEY = src->O_ORDERKEY;
        dest->O_ORDERPRIORITY = src->O_ORDERPRIORITY;
    }
    virtual order_tscan_filter_t* clone() const {
        return new order_tscan_filter_t(*this);
    }
    virtual c_str to_string() const {
        return "select O_ORDERKEY, O_ORDERPRIORITY";
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
    int year;
    int mode1, mode2;
    
    lineitem_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_lineitem_tuple))
    {
        size_t offset;
        predicate_t* p;

        // where L_SHIPMODE in ('MAIL', 'SHIP') ...
        thread_t* self = thread_get_self();
        // pick a random ship mode
        mode1 = self->rand(END_SHIPMODE);
        // and another that must be different -- add a random number
        // that is neither 0 or END_SHIPMODE and take the modulus.
        mode2 = (mode1 + self->rand(END_SHIPMODE-1) + 1) % END_SHIPMODE;

        // 5 possible years to choose from
        year = self->rand(5);

        if(0) {
            // validation run
            mode1 = MAIL;
            mode2 = SHIP;
            year = 1; // 1994
        }
        
        offset = offsetof(tpch_lineitem_tuple, L_SHIPMODE);
        or_predicate_t* orp = new or_predicate_t();
        p = new scalar_predicate_t<int>(mode1, offset);
        orp->add(p);
        p = new scalar_predicate_t<int>(mode2, offset);
        orp->add(p);
        _filter.add(orp);

        // ... and L_COMMITDATE < L_RECEIPTDATE ...
        size_t offset1 = offsetof(tpch_lineitem_tuple, L_COMMITDATE);
        size_t offset2 = offsetof(tpch_lineitem_tuple, L_RECEIPTDATE);
        p = new field_predicate_t<time_t, less>(offset1, offset2);
        _filter.add(p);

        // ... and L_SHIPDATE < L_COMMITDATE ...
        offset1 = offsetof(tpch_lineitem_tuple, L_SHIPDATE);
        offset2 = offsetof(tpch_lineitem_tuple, L_COMMITDATE);
        p = new field_predicate_t<time_t, less>(offset1, offset2);
        _filter.add(p);

        // ... and L_RECEIPTDATE >= [date] ...
        time_t date1 = datestr_to_timet("1993-01-01");
        date1 = time_add_year(date1, year);
        time_t date2 = time_add_year(date1, 1);
        offset = offsetof(tpch_lineitem_tuple, L_RECEIPTDATE);
        p = new scalar_predicate_t<time_t, greater_equal>(date1, offset);
        _filter.add(p);

        // ... and L_RECEIPTDATE < [date] + 1 year
        p = new scalar_predicate_t<time_t, less>(date2, offset);
        _filter.add(p);
    }
    virtual void project(tuple_t &d, const tuple_t &s) {
        lineitem_scan_tuple *dest = aligned_cast<lineitem_scan_tuple>(d.data);
        tpch_lineitem_tuple *src = aligned_cast<tpch_lineitem_tuple>(s.data);
        dest->L_ORDERKEY = src->L_ORDERKEY;
        dest->L_SHIPMODE = src->L_SHIPMODE;
    }
    virtual bool select(const tuple_t &t) {
        return _filter.select(t);
    }
    virtual lineitem_tscan_filter_t* clone() const {
        return new lineitem_tscan_filter_t(*this);
    }
    virtual c_str to_string() const {
        return c_str("SHIPMODE1 = %d, SHIPMODE2 = %d, YEAR = %d",
                     mode1, mode2, year);
    }
};

/**
 * @brief ... from lineitem, orders where L_ORDERKEY = O_ORDERKEY ...
 */
struct q12_join_t : tuple_join_t {

    q12_join_t()
        : tuple_join_t(sizeof(order_scan_tuple),
                       offsetof(order_scan_tuple, O_ORDERKEY),
                       sizeof(lineitem_scan_tuple),
                       offsetof(lineitem_scan_tuple, L_ORDERKEY),
                       sizeof(int),
                       sizeof(join_tuple))
    {
    }

    virtual void join(tuple_t &d, const tuple_t &l, const tuple_t &r) {

        join_tuple* dest = aligned_cast<join_tuple>(d.data);
        order_scan_tuple* left = aligned_cast<order_scan_tuple>(l.data);
        lineitem_scan_tuple* right = aligned_cast<lineitem_scan_tuple>(r.data);
        
        // cheat and filter out the join key here
        dest->L_SHIPMODE = right->L_SHIPMODE;
        dest->O_ORDERPRIORITY = left->O_ORDERPRIORITY;
    }

    virtual q12_join_t* clone() const {
        return new q12_join_t(*this);
    }

    virtual c_str to_string() const {
        return "join LINEITEM, ORDERS, select L_SHIPMODE, O_ORDERPRIORITY";
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
                     offsetof(q12_tuple, L_SHIPMODE))
    {
        size_t offset;
        predicate_t* p;

        // case when O_ORDERPRIORITY = '1-URGENT' or O_ORDERPRIORITY =
        // '2-HIGH' then 1 else 0 end
        offset = offsetof(join_tuple, O_ORDERPRIORITY);
        p = new scalar_predicate_t<tpch_o_orderpriority>(URGENT_1, offset);
        _filter.add(p);
        p = new scalar_predicate_t<tpch_o_orderpriority>(HIGH_2, offset);
        _filter.add(p);
    }

    virtual key_extractor_t* key_extractor() {
        return &_extractor;
    }
    virtual void aggregate(char* agg_data, const tuple_t &t) {
        q12_tuple* agg = aligned_cast<q12_tuple>(agg_data);

        if(_filter.select(t))
            agg->HIGH_LINE_COUNT++;
        else
            agg->LOW_LINE_COUNT++;
    }
    virtual void finish(tuple_t &dest, const char* agg_data) {
        memcpy(dest.data, agg_data, dest.size);
    }
    virtual q12_aggregate_t* clone() const {
        return new q12_aggregate_t(*this);
    }
    virtual c_str to_string() const {
        return "q12_aggregate_t";
    }
};


    
void tpch_q12_driver::submit(void* disp) {

    scheduler::policy_t* dp = (scheduler::policy_t*) disp;
  

    // lineitem scan
    tuple_filter_t* filter = new lineitem_tscan_filter_t();
    tuple_fifo* buffer = new tuple_fifo(sizeof(lineitem_scan_tuple));
    packet_t* lineitem_packet =
        new tscan_packet_t("lineitem TSCAN",
                           buffer,
                           filter,
                           tpch_lineitem);
    // order scan
    filter = new order_tscan_filter_t();
    buffer = new tuple_fifo(sizeof(order_scan_tuple));
    packet_t* order_packet =
        new tscan_packet_t("order TSCAN",
                           buffer, filter,
                           tpch_orders);
    
    // join
    filter = new trivial_filter_t(sizeof(join_tuple));
    buffer = new tuple_fifo(sizeof(join_tuple));
    packet_t* join_packet;
    join_packet =
        new hash_join_packet_t("orders-lineitem HJOIN",
                               buffer, filter,
                               order_packet,
                               lineitem_packet,
                               new q12_join_t());

    // partial aggregation
    filter = new trivial_filter_t(sizeof(q12_tuple));
    guard<tuple_fifo> result = new tuple_fifo(sizeof(q12_tuple));
    size_t offset = offsetof(join_tuple, L_SHIPMODE);
    key_extractor_t* extractor = new default_key_extractor_t(sizeof(int), offset);
    key_compare_t* compare = new int_key_compare_t();
    tuple_aggregate_t* aggregate = new q12_aggregate_t();
    packet_t* agg_packet =
        new partial_aggregate_packet_t("sum AGG",
                                       result, filter,
                                       join_packet,
                                       aggregate,
                                       extractor,
                                       compare);

    // go!
    qpipe::query_state_t* qs = dp->query_state_create();
    agg_packet->assign_query_state(qs);
    join_packet->assign_query_state(qs);
    order_packet->assign_query_state(qs);
    lineitem_packet->assign_query_state(qs);

    dispatcher_t::dispatch_packet(agg_packet);

    tuple_t output;
    TRACE(TRACE_QUERY_RESULTS, "*** Q12 %10s %10s %10s\n",
          "Shipmode", "High_Count", "Low_Count");
    while(result->get_tuple(output)) {
        q12_tuple* r = aligned_cast<q12_tuple>(output.data);
        TRACE(TRACE_QUERY_RESULTS, "*** Q12 %10s %10d %10d\n",
              (r->L_SHIPMODE == MAIL) ? "MAIL" : "SHIP",
              r->HIGH_LINE_COUNT,
              r->LOW_LINE_COUNT);
    }

    dp->query_state_destroy(qs);
}

EXIT_NAMESPACE(workload);
