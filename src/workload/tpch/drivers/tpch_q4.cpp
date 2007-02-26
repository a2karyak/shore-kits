/* -*- mode:C++; c-basic-offset:4 -*- */
#include "stages.h"
#include "scheduler.h"

#include "workload/tpch/drivers/tpch_q4.h"

#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "workload/tpch/tpch_env.h"
#include "workload/common/int_comparator.h"
#include "workload/process_query.h"


ENTER_NAMESPACE(q4);



/**
 * @brief select distinct l_orderkey from lineitem where l_commitdate
 * < l_receiptdate
 */
packet_t* line_item_scan(page_list* tpch_lineitem) {
    struct lineitem_tscan_filter_t : public tuple_filter_t {
        /* Initialize the predicates */
        lineitem_tscan_filter_t()
            : tuple_filter_t(sizeof(tpch_lineitem_tuple))
        {
        }

        /* Predication */
        virtual bool select(const tuple_t &input) {
            tpch_lineitem_tuple *tuple = aligned_cast<tpch_lineitem_tuple>(input.data);
            return tuple->L_COMMITDATE < tuple->L_RECEIPTDATE;
        }
    
        /* Projection */
        virtual void project(tuple_t &dest, const tuple_t &src) {
            /* Should project  L_ORDERKEY */
            tpch_lineitem_tuple *at = aligned_cast<tpch_lineitem_tuple>((src.data));
	    *aligned_cast<int>(dest.data) = at->L_ORDERKEY;
        }
        virtual lineitem_tscan_filter_t* clone() const {
            return new lineitem_tscan_filter_t(*this);
        }
        virtual c_str to_string() const {
            return "select L_ORDERKEY where L_COMMITDATE < L_RECEIPTDATE";
        }
    };

    // LINEITEM TSCAN PACKET
    // the output consists of 1 int (the
    tuple_filter_t* filter = new lineitem_tscan_filter_t(); 
    tuple_fifo* buffer = new tuple_fifo(sizeof(int));
    packet_t *tscan_packet = new tscan_packet_t("lineitem TSCAN",
                                                buffer,
                                                filter,
                                                tpch_lineitem);
    return tscan_packet;
}

/**
 * @brief holds the results of an order scan after projection
 */
struct order_scan_tuple_t {
    int O_ORDERKEY;
    int O_ORDERPRIORITY;
};

packet_t* orders_scan(page_list* tpch_orders) {
    struct orders_tscan_filter_t : public tuple_filter_t {
        time_t t1, t2;
        
        /* Initialize the predicates */
        orders_tscan_filter_t()
            : tuple_filter_t(sizeof(tpch_orders_tuple))
        {
            // TPCH Spec: "DATE is the first day of a randomly
            // selected month between the first month of 1993 and the
            // 10th month of 1997."
            t1 = datestr_to_timet("1993-01-01");
            t1 = time_add_month(t1, thread_get_self()->rand(59));
            t2 = time_add_month(t1, 3);
        }

        /* Predication */
        virtual bool select(const tuple_t &input) {
            tpch_orders_tuple *tuple = aligned_cast<tpch_orders_tuple>(input.data);
            return tuple->O_ORDERDATE >= t1 && tuple->O_ORDERDATE < t2;
        }
    
        /* Projection */
        virtual void project(tuple_t &d, const tuple_t &s) {
            /* Should project  O_ORDERKEY and O_ORDERPRIORITY*/
            tpch_orders_tuple* src = aligned_cast<tpch_orders_tuple>(s.data);
            order_scan_tuple_t* dest = aligned_cast<order_scan_tuple_t>(d.data);
            dest->O_ORDERKEY = src->O_ORDERKEY;
            dest->O_ORDERPRIORITY = src->O_ORDERPRIORITY;
        }
        virtual orders_tscan_filter_t* clone() const {
            return new orders_tscan_filter_t(*this);
        }

        virtual c_str to_string() const {
            char* date1 = timet_to_datestr(t1);
            char* date2 = timet_to_datestr(t2);
            c_str result("select O_ORDERKEY, O_ORDERPRIORITY "
                         "where O_ORDERDATE >= %s and O_ORDERDATE < %s",
                         date1, date2);
            free(date1);
            free(date2);
            return result;
        }
    };

    tuple_filter_t* filter = new orders_tscan_filter_t(); 
    tuple_fifo* buffer = new tuple_fifo(sizeof(order_scan_tuple_t));
    packet_t *tscan_packet = new tscan_packet_t("order TSCAN",
                                                buffer,
                                                filter,
                                                tpch_orders);
    
    return tscan_packet;
}



// left is lineitem, right is orders
struct q4_join_t : public tuple_join_t {


    q4_join_t ()
        : tuple_join_t(sizeof(order_scan_tuple_t),
                       offsetof(order_scan_tuple_t, O_ORDERKEY),
                       sizeof(int),
                       0,
                       sizeof(int),
                       sizeof(int))
    {
    }
                       
    
    virtual void join(tuple_t &dest,
                      const tuple_t &left,
                      const tuple_t &)
    {
        // KLUDGE: this projection should go in a separate filter class
        order_scan_tuple_t* tuple = aligned_cast<order_scan_tuple_t>(left.data);
        *aligned_cast<int>(dest.data) = tuple->O_ORDERPRIORITY;
    }

    virtual q4_join_t* clone() const {
        return new q4_join_t(*this);
    }

    virtual c_str to_string() const {
        return "join LINEITEM and ORDERS, select O_ORDERPRIORITY";
    }
};


struct q4_tuple_t {
    int O_ORDERPRIORITY;
    int ORDER_COUNT;
};


struct q4_count_aggregate_t : public tuple_aggregate_t {
    default_key_extractor_t _extractor;
    
    q4_count_aggregate_t()
        : tuple_aggregate_t(sizeof(q4_tuple_t))
    {
    }
    virtual key_extractor_t* key_extractor() { return &_extractor; }
    
    virtual void aggregate(char* agg_data, const tuple_t &) {
        q4_tuple_t* agg = aligned_cast<q4_tuple_t>(agg_data);
        agg->ORDER_COUNT++;
    }

    virtual void finish(tuple_t &d, const char* agg_data) {
        memcpy(d.data, agg_data, tuple_size());
    }
    virtual q4_count_aggregate_t* clone() const {
        return new q4_count_aggregate_t(*this);
    }
    virtual c_str to_string() const {
        return "q4_count_aggregate_t";
    }
};


EXIT_NAMESPACE(q4);


using namespace q4;
using namespace workload;


ENTER_NAMESPACE(workload);


using namespace qpipe;


class tpch_q4_process_tuple_t : public process_tuple_t {
    
public:
        
    virtual void process(const tuple_t& output) {
        q4_tuple_t* r = aligned_cast<q4_tuple_t>(output.data);
        TRACE(TRACE_QUERY_RESULTS, "*** Q4 Priority: %d. Count: %d.  ***\n",
              r->O_ORDERPRIORITY,
              r->ORDER_COUNT);
    }

};


void tpch_q4_driver::submit(void* disp) {

    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
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

    tuple_fifo* buffer;

    // First deal with the lineitem half

    packet_t *line_item_packet = line_item_scan(tpch_tables[TPCH_TABLE_LINEITEM].db);
        
    // Now, the orders half
    packet_t* orders_packet = orders_scan(tpch_tables[TPCH_TABLE_ORDERS].db);

    // join them...
    tuple_filter_t* filter = new trivial_filter_t(sizeof(int));
    buffer = new tuple_fifo(sizeof(int));
    tuple_join_t* join = new q4_join_t();
    packet_t* join_packet = new hash_join_packet_t("Orders - Lineitem JOIN",
                                                   buffer,
                                                   filter,
                                                   orders_packet,
                                                   line_item_packet,
                                                   join,
                                                   false,
                                                   true);

    // sort/aggregate in one step
    filter = new trivial_filter_t(sizeof(q4_tuple_t));
    buffer = new tuple_fifo(sizeof(q4_tuple_t));
    tuple_aggregate_t *aggregate = new q4_count_aggregate_t();
    packet_t* agg_packet;
    agg_packet = new partial_aggregate_packet_t("O_ORDERPRIORITY COUNT",
                                                buffer,
                                                filter,
                                                join_packet,
                                                aggregate,
                                                new default_key_extractor_t(),
                                                new int_key_compare_t());

    qpipe::query_state_t* qs = dp->query_state_create();
    agg_packet->assign_query_state(qs);
    join_packet->assign_query_state(qs);
    orders_packet->assign_query_state(qs);
    line_item_packet->assign_query_state(qs);

    // Dispatch packet
    tpch_q4_process_tuple_t pt;
    process_query(agg_packet, pt);

    dp->query_state_destroy(qs);
}


EXIT_NAMESPACE(workload);
