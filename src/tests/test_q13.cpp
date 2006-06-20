// -*- mode:C++; c-basic-offset:4 -*-
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
#include "tests/common/tpch_db.h"

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
    virtual void extract_key(void* key, const void* tuple_data) {
        int value = -*(const int*)tuple_data;
        *(int*)key = value;
    }
    virtual int_desc_key_extractor_t* clone() {
        return new int_desc_key_extractor_t(*this);
    }
};

struct q13_count_aggregate_t : public tuple_aggregate_t {
    bool _first;
    int  _last_value;
    int  _count;
    
    q13_count_aggregate_t()
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
        key_count_tuple_t* dest = (key_count_tuple_t*) d.data;
        dest->KEY = _last_value;
        dest->COUNT = _count;
        _last_value = value;
        _count = 1;
        return true;
    }

    virtual bool eof(tuple_t  &d) {
        if ( _first )
            // we've seen no tuples!
            return false;

        key_count_tuple_t* dest = (key_count_tuple_t*) d.data;
        dest->KEY = _last_value;
        dest->COUNT = _count;
        return true;
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
        virtual customer_tscan_filter_t* clone() {
            return new customer_tscan_filter_t(*this);
        }
    };

    char* packet_id = copy_string("Customer TSCAN");
    tuple_filter_t* filter = new customer_tscan_filter_t();
    tuple_buffer_t* buffer = new tuple_buffer_t(sizeof(int));
    packet_t *packet = new tscan_packet_t(packet_id,
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

    virtual order_tscan_filter_t* clone() {
        return new order_tscan_filter_t(*this);
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
    char* packet_id = copy_string("Orders TSCAN");
    tuple_filter_t* filter = new order_tscan_filter_t();
    tuple_buffer_t* buffer = new tuple_buffer_t(sizeof(int));
    packet_t* tscan_packet = new tscan_packet_t(packet_id,
                                                buffer,
                                                filter,
                                                tpch_orders);

    // sort into groups 
    packet_id = copy_string("Orders SORT");
    filter = new trivial_filter_t(sizeof(int));
    buffer = new tuple_buffer_t(sizeof(int));
    packet_t* sort_packet = new sort_packet_t(packet_id,
                                              buffer,
                                              filter,
                                              new int_desc_key_extractor_t(),
                                              new int_key_compare_t(),
                                              tscan_packet);

    // count over groups
    packet_id = copy_string("Orders COUNT");
    filter = new trivial_filter_t(sizeof(key_count_tuple_t));
    buffer = new tuple_buffer_t(sizeof(key_count_tuple_t));
    tuple_aggregate_t* aggregator = new q13_count_aggregate_t();
    packet_t* agg_packet = new aggregate_packet_t(packet_id,
                                                  buffer,
                                                  filter,
                                                  aggregator,
                                                  sort_packet);

    return agg_packet;
}

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
    register_stage<sort_stage_t>(3);
    register_stage<merge_stage_t>(10);
    register_stage<fdump_stage_t>(10);
    register_stage<fscan_stage_t>(20);
    register_stage<hash_join_stage_t>(2);


    for(int i=0; i < 1; i++) {

        stopwatch_t timer;

        /*
         * select c_count, count(*) as custdist
         * from customer natural left outer join cust_order_count
         * group by c_count
         * order by custdist desc, c_count desc
         */
        struct q13_join_t : public tuple_join_t {
            struct right_key_extractor_t : public key_extractor_t {
                virtual void extract_key(void* key, const void* tuple_data) {
                    const key_count_tuple_t* tuple = (const key_count_tuple_t*) tuple_data;
                    memcpy(key, &tuple->KEY, key_size());
                }
                virtual right_key_extractor_t* clone() {
                    return new right_key_extractor_t(*this);
                }
            };
    
            struct left_key_extractor_t : public key_extractor_t {
                virtual void extract_key(void* key, const void* tuple_data) {
                    memcpy(key, tuple_data, key_size());
                }
                virtual left_key_extractor_t* clone() {
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
        };

        struct q13_key_extract_t : public key_extractor_t {
            q13_key_extract_t() : key_extractor_t(sizeof(key_count_tuple_t)) { }
            
            virtual int extract_hint(const void* tuple_data) {
                key_count_tuple_t* tuple = (key_count_tuple_t*) tuple_data;
                // confusing -- custdist is a count of counts...
                return -tuple->COUNT;
            }
            virtual void extract_key(void* key, const void* tuple_data) {
                // the whole tuple is the key
                memcpy(key, tuple_data, key_size());
            }
            virtual q13_key_extract_t* clone() {
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
            virtual q13_key_compare_t* clone() {
                return new q13_key_compare_t(*this);
            }
        };

        // TODO: consider using a sort-merge join instead of hash
        // join? cust_order_count is already sorted on c_custkey

        // get the inputs to the join
        packet_t* customer_packet = customer_scan(tpch_customer);
        packet_t* order_packet = order_scan(tpch_orders);

        char* packet_id = copy_string("Orders - Customer JOIN");
        tuple_filter_t* filter = new trivial_filter_t(sizeof(int));
        tuple_buffer_t* buffer = new tuple_buffer_t(sizeof(int));
        tuple_join_t* join = new q13_join_t();
        packet_t* join_packet = new hash_join_packet_t(packet_id,
                                                       buffer,
                                                       filter,
                                                       customer_packet,
                                                       order_packet,
                                                       join,
                                                       true);

        // sort to group by c_count
        packet_id = copy_string("c_count SORT");
        filter = new trivial_filter_t(sizeof(int));
        buffer = new tuple_buffer_t(sizeof(int));
        packet_t *sort_packet = new sort_packet_t(packet_id,
                                                  buffer,
                                                  filter,
                                                  new int_desc_key_extractor_t(),
                                                  new int_key_compare_t(),
                                                  join_packet);

        // aggregate over c_count
        packet_id = copy_string("c_count COUNT");
        filter = new trivial_filter_t(sizeof(key_count_tuple_t));
        buffer = new tuple_buffer_t(sizeof(key_count_tuple_t));
        tuple_aggregate_t* agg = new q13_count_aggregate_t();
        packet_t *agg_packet = new aggregate_packet_t(packet_id,
                                                      buffer,
                                                      filter,
                                                      agg,
                                                      sort_packet);

        // final sort of results
        packet_id = copy_string("custdist, c_count SORT");
        filter = new trivial_filter_t(sizeof(key_count_tuple_t));
        buffer = new tuple_buffer_t(sizeof(key_count_tuple_t));
        sort_packet = new sort_packet_t(packet_id,
                                        buffer,
                                        filter,
                                        new q13_key_extract_t(),
                                        new q13_key_compare_t(),
                                        agg_packet);
        
        // Dispatch packet
        dispatcher_t::dispatch_packet(sort_packet);
        buffer = sort_packet->_output_buffer;
        
        tuple_t output;
        while(!buffer->get_tuple(output)) {
            key_count_tuple_t* r = (key_count_tuple_t*) output.data;
            TRACE(TRACE_ALWAYS, "*** Q4 Count: %d. CustDist: %d.  ***\n",
                  r->KEY, r->COUNT);
        }
        


        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }
    
    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
