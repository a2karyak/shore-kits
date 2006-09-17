/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"

#include "tests/common.h"
#include "workload/common.h"
#include "workload/tpch/tpch_db.h"

using namespace qpipe;

/**
 * Original TPC-H Query 13:
 *
 * select c_count, count(*) as custdist
 * from (select c_custkey, count(o_orderkey)
 *       from customer
 *       left outer join orders on
 *       c_custkey = o_custkey
 *       and o_comment not like %[WORD1]%[WORD2]%
 *       group by c_custkey)
 *       as c_orders (c_custkey, c_count)
 * group by c_count
 * order by custdist desc, c_count desc;
 *
 */

struct key_count_tuple_t {
    int KEY;
    int COUNT;
};

// this comparator sorts its keys in descending order
struct int_desc_key_extractor_t : public key_extractor_t {
    virtual int extract_hint(const char* tuple_data) {
        return -*(int*)tuple_data;
    }
    virtual int_desc_key_extractor_t* clone() const {
        return new int_desc_key_extractor_t(*this);
    }
};

struct q13_count_aggregate_t : public tuple_aggregate_t {
    default_key_extractor_t _extractor;
    
    q13_count_aggregate_t()
        : tuple_aggregate_t(sizeof(key_count_tuple_t))
    {
    }
    virtual key_extractor_t* key_extractor() { return &_extractor; }
    
    virtual void aggregate(char* agg_data, const tuple_t &) {
        key_count_tuple_t* agg = (key_count_tuple_t*) agg_data;
        agg->COUNT++;
    }

    virtual void finish(tuple_t &d, const char* agg_data) {
        memcpy(d.data, agg_data, tuple_size());
    }
    virtual q13_count_aggregate_t* clone() const {
        return new q13_count_aggregate_t(*this);
    }
    virtual c_str to_string() const {
        return "q13_count_aggregate_t";
    }
};


/**
 * @brief select c_cust_key from customer
 */
packet_t* customer_scan(Db* tpch_customer) {
    struct customer_tscan_filter_t : public tuple_filter_t {
        customer_tscan_filter_t() 
            : tuple_filter_t(sizeof(tpch_customer_tuple))
        {
        }

        virtual void project(tuple_t &d, const tuple_t &s) {
            tpch_customer_tuple* src = (tpch_customer_tuple*) s.data;
            int* dest = (int*) d.data;
            *dest = src->C_CUSTKEY;
        }
        virtual customer_tscan_filter_t* clone() const {
            return new customer_tscan_filter_t(*this);
        }
        virtual c_str to_string() const {
            return "select C_CUSTKEY";
        }
    };

    tuple_filter_t* filter = new customer_tscan_filter_t();
    tuple_fifo* buffer = new tuple_fifo(sizeof(int), dbenv);
    packet_t *packet = new tscan_packet_t("Customer TSCAN",
                                          buffer,
                                          filter,
                                          tpch_customer);

    return packet;
}

struct order_tscan_filter_t : public tuple_filter_t {
    char *word1;
    char *word2;
        
    order_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_orders_tuple))
    {
        // TODO: random word selection per TPC-H spec
        word1 = "special";
        word2 = "requests";
    }

    virtual bool select(const tuple_t &input) {
        tpch_orders_tuple* order = (tpch_orders_tuple*) input.data;

        // search for all instances of the first substring. Make sure
        // the second search is *after* the first...
        char* first = strstr(order->O_COMMENT, word1);
        if(!first)
            return true;

        char* second = strstr(first + strlen(word1), word2);
        if(!second)
            return true;

        // if we got here, match (and therefore reject)
        return false;
    }

    /* Projection */
    virtual void project(tuple_t &d, const tuple_t &s) {
        // project C_CUSTKEY
        tpch_orders_tuple* src = (tpch_orders_tuple*) s.data;
        int* dest = (int*) d.data;
        *dest = src->O_CUSTKEY;
    }

    virtual order_tscan_filter_t* clone() const {
        return new order_tscan_filter_t(*this);
    }
    virtual c_str to_string() const {
        return c_str("select O_CUSTKEY "
                     "where O_COMMENT like %%%s%%%s%%",
                     word1, word2);
    }
};

/**
 * @brief select c_custkey, count(*) as C_COUNT
 *        from orders
 *        where o_comment not like "%[WORD1]%[WORD2]%"
 *        group by c_custkey
 *        order by c_custkey desc
 */
packet_t* order_scan(Db* tpch_orders) {

    // Orders TSCAN
    tuple_filter_t* filter = new order_tscan_filter_t();
    tuple_fifo* buffer = new tuple_fifo(sizeof(int), dbenv);
    packet_t* tscan_packet = new tscan_packet_t("Orders TSCAN",
                                                buffer,
                                                filter,
                                                tpch_orders);

    // group by
    filter = new trivial_filter_t(sizeof(key_count_tuple_t));
    buffer = new tuple_fifo(sizeof(key_count_tuple_t), dbenv);
    tuple_aggregate_t* aggregator = new q13_count_aggregate_t();
    packet_t* pagg_packet;
    pagg_packet= new partial_aggregate_packet_t("Orders Group By",
                                                buffer,
                                                filter,
                                                tscan_packet,
                                                aggregator,
                                                new int_desc_key_extractor_t(),
                                                new int_key_compare_t());

    return pagg_packet;
}

int main() {
    thread_init();
    db_open();
    TRACE_SET(TRACE_ALWAYS);


    // line up the stages...
    register_stage<tscan_stage_t>(2);
    register_stage<partial_aggregate_stage_t>(2);
    register_stage<sort_stage_t>(3);
    register_stage<merge_stage_t>(10);
    register_stage<fdump_stage_t>(10);
    register_stage<fscan_stage_t>(20);
    register_stage<hash_join_stage_t>(2);


    for(int i=0; i < 10; i++) {

        stopwatch_t timer;

        /*
         * select c_count, count(*) as custdist
         * from customer natural left outer join cust_order_count
         * group by c_count
         * order by custdist desc, c_count desc
         */
        struct q13_join_t : public tuple_join_t {
            struct right_key_extractor_t : public key_extractor_t {
                virtual int extract_hint(const char* tuple_data) {
                    key_count_tuple_t* tuple = (key_count_tuple_t*) tuple_data;
                    return tuple->KEY;
                }
                virtual right_key_extractor_t* clone() const {
                    return new right_key_extractor_t(*this);
                }
            };
    
            struct left_key_extractor_t : public key_extractor_t {
                virtual int extract_hint(const char* tuple_data) {
                    return *(int*) tuple_data;
                }
                virtual left_key_extractor_t* clone() const {
                    return new left_key_extractor_t(*this);
                }
            };
    
            
            q13_join_t()
                : tuple_join_t(sizeof(int), new left_key_extractor_t(),
                               sizeof(key_count_tuple_t), new right_key_extractor_t(),
                               new int_key_compare_t(), sizeof(int))
            {
            }
            virtual void join(tuple_t &dest,
                              const tuple_t &,
                              const tuple_t &right)
            {
                // KLUDGE: this projection should go in a separate filter class
                key_count_tuple_t* tuple = (key_count_tuple_t*) right.data;
                memcpy(dest.data, &tuple->COUNT, sizeof(int));
            }
            virtual void outer_join(tuple_t &dest, const tuple_t &) {
                int zero = 0;
                memcpy(dest.data, &zero, sizeof(int));
            }
            virtual c_str to_string() const {
                return "CUSTOMER left outer join CUST_ORDER_COUNT, select COUNT";
            }
        };

        struct q13_key_extract_t : public key_extractor_t {
            q13_key_extract_t() : key_extractor_t(sizeof(key_count_tuple_t)) { }
            
            virtual int extract_hint(const char* tuple_data) {
                key_count_tuple_t* tuple = (key_count_tuple_t*) tuple_data;
                // confusing -- custdist is a count of counts... and
                // descending sort
                return -tuple->COUNT;
            }
            virtual q13_key_extract_t* clone() const {
                return new q13_key_extract_t(*this);
            }
        };
        struct q13_key_compare_t : public key_compare_t {
            virtual int operator()(const void* key1, const void* key2) {
                // at this point we know the custdist (count) fields are
                // different, so just check the c_count (key) fields
                key_count_tuple_t* a = (key_count_tuple_t*) key1;
                key_count_tuple_t* b = (key_count_tuple_t*) key2;
                return b->KEY - a->KEY;
            }
            virtual q13_key_compare_t* clone() const {
                return new q13_key_compare_t(*this);
            }
        };

        // TODO: consider using a sort-merge join instead of hash
        // join? cust_order_count is already sorted on c_custkey

        // get the inputs to the join
        packet_t* customer_packet = customer_scan(tpch_customer);
        packet_t* order_packet = order_scan(tpch_orders);

        tuple_filter_t* filter = new trivial_filter_t(sizeof(int));
        tuple_fifo* buffer = new tuple_fifo(sizeof(int), dbenv);
        tuple_join_t* join = new q13_join_t();
        packet_t* join_packet = new hash_join_packet_t("Orders - Customer JOIN",
                                                       buffer,
                                                       filter,
                                                       customer_packet,
                                                       order_packet,
                                                       join,
                                                       true);


        // group by c_count
        filter = new trivial_filter_t(sizeof(key_count_tuple_t));
        buffer = new tuple_fifo(sizeof(key_count_tuple_t), dbenv);
        packet_t *pagg_packet;
        pagg_packet = new partial_aggregate_packet_t("c_count SORT",
                                                     buffer,
                                                     filter,
                                                     join_packet,
                                                     new q13_count_aggregate_t(),
                                                     new int_desc_key_extractor_t(),
                                                     new int_key_compare_t());

        // final sort of results
        filter = new trivial_filter_t(sizeof(key_count_tuple_t));
        buffer = new tuple_fifo(sizeof(key_count_tuple_t), dbenv);
        packet_t *sort_packet;
        sort_packet = new sort_packet_t("custdist, c_count SORT",
                                        buffer,
                                        filter,
                                        new q13_key_extract_t(),
                                        new q13_key_compare_t(),
                                        pagg_packet);
        
        // Dispatch packet
        dispatcher_t::dispatch_packet(sort_packet);
        guard<tuple_fifo> result = sort_packet->output_buffer();
        
        tuple_t output;
        while(result->get_tuple(output)) {
            key_count_tuple_t* r = (key_count_tuple_t*) output.data;
            TRACE(TRACE_ALWAYS, "*** Q13 Count: %d. CustDist: %d.  ***\n",
                  r->KEY, r->COUNT);
        }
        


        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }
    
    db_close();
    return 0;
}
