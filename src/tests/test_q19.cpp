/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"
#include "engine/stages/hash_join.h"
#include "engine/dispatcher.h"
#include "engine/util/stopwatch.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common.h"
#include "workload/common.h"
#include "workload/tpch/tpch_db.h"

// experimental!
#include "workload/common/predicates.h"

using namespace qpipe;

/*
 * select
 *      sum(l_extendedprice * (1 - l_discount) ) as revenue
 * from
 *      tpcd.lineitem,
 *      tpcd.part
 * where
 *      (
 *              p_partkey = l_partkey
 *              and p_brand = 'Brand#12'
 *              and p_container in ( 'SM CASE', 'SM BOX', 'SM PACK', 'SM PKG')
 *              and l_quantity >= 1 and l_quantity <= 1 + 10
 *              and p_size between 1 and 5
 *              and l_shipmode in ('AIR', 'AIR REG')
 *              and l_shipinstruct = 'DELIVER IN PERSON'
 *      )
 *      or
 *      (
 *              p_partkey = l_partkey
 *              and p_brand = 'Brand#23'
 *              and p_container in ('MED BAG', 'MED BOX', 'MED PKG', 'MED PACK')
 *              and l_quantity >= 10 and l_quantity <= 10 + 10
 *              and p_size between 1 and 10
 *              and l_shipmode in ('AIR', 'AIR REG')
 *              and l_shipinstruct = 'DELIVER IN PERSON'
 *      )
 *      or
 *      (
 *              p_partkey = l_partkey
 *              and p_brand = 'Brand#34'
 *              and p_container in ( 'LG CASE', 'LG BOX', 'LG PACK', 'LG PKG')
 *              and l_quantity >= 20 and l_quantity <= 20 + 10
 *              and p_size between 1 and 15
 *              and l_shipmode in ('AIR', 'AIR REG')
 *              and l_shipinstruct = 'DELIVER IN PERSON'
 *      );
 *
 * db2expln says the part scan has 6 predicates, lineitem has 8, and
 * there are 15 left over. My guess:
 *
 * part (2*3 = 6): (a and b) or (c and d) or (e and f)
 *      - p_brand = "Brand#12' and p_size between 1 and 5
 *
 * lineitem (1 + 1 + 3*2 = 8): a and b and ((c and d) or (e and f) or (g and h))
 *      - l_shipmode in ('AIR', 'AIR REG')
 *      - l_shipinstruct = 'DELIVER IN PERSON'
 *      - l_quantity >= [quantity] and l_quantity <= [quantity] + 10
 *
 *
 * left over (3*5 = 15): (a and b and c and d and e) or (...) or (...)
 *      - p_brand = 'Brand12' and p_container in (...) and p_size between ...
 *        and l_quantity >= ... and l_quantity <= ...
 */

struct part_scan_tuple {
    int P_PARTKEY;
    int P_SIZE;
    char P_CONTAINER[STRSIZE(10)];
    char P_BRAND[STRSIZE(10)];
};

struct lineitem_scan_tuple {
    double L_QUANTITY;
    double L_EXTENDEDPRICE;
    double L_DISCOUNT;
    int L_PARTKEY;
    tpch_l_shipmode L_SHIPMODE;
    char L_SHIPINSTRUCT[STRSIZE(25)];
};

struct join_tuple {
    double L_QUANTITY;
    double L_EXTENDEDPRICE;
    double L_DISCOUNT;
    int PARTKEY;
    int P_SIZE;
    tpch_l_shipmode L_SHIPMODE;
    char P_CONTAINER[STRSIZE(10)];
    char P_BRAND[STRSIZE(10)];
    char L_SHIPINSTRUCT[STRSIZE(25)];
};

struct q19_tuple {
    double L_EXTENDEDPRICE;
    double L_DISCOUNT;
};

/**
 * @brief select p_partkey, p_brand, p_container, p_size from part
 * where p_brand = ... and p_size between ...
 */
struct part_tscan_filter_t : public tuple_filter_t {
    or_predicate_t _filter;
    char* _brands[3];
    int _sizes[3];

    part_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_part_tuple))
    {
        int sizes[] = {5, 10, 15};
        
        // TODO: randomize these
        char* brands[] = {"Brand#12", "Brand#23", "Brand#34"};
        memcpy(_brands, brands, sizeof(brands));
        memcpy(_sizes, sizes, sizeof(sizes));

        for(int i=0; i < 3; i++) {
            and_predicate_t* andp = new and_predicate_t();
            predicate_t* p;
            size_t offset;
            
            // brand = [brand]
            offset = offsetof(tpch_part_tuple, P_BRAND);
            p = new string_predicate_t<>(brands[i], offset);
            andp->add(p);
                             
            // size between ...
            offset = offsetof(tpch_part_tuple, P_SIZE);
            p = new scalar_predicate_t<int, greater_equal>(1, offset);
            andp->add(p);
            p = new scalar_predicate_t<int, less_equal>(sizes[i], offset);
            andp->add(p);

            _filter.add(andp);
        }
    }
    virtual void project(tuple_t &d, const tuple_t &s) {
        part_scan_tuple *dest = (part_scan_tuple*) d.data;
        tpch_part_tuple *src = (tpch_part_tuple*) s.data;
        dest->P_PARTKEY = src->P_PARTKEY;
        dest->P_SIZE = src->P_SIZE;
        memcpy(dest->P_CONTAINER, src->P_CONTAINER, sizeof(src->P_CONTAINER));
        memcpy(dest->P_BRAND, src->P_BRAND, sizeof(src->P_BRAND));
    }
    virtual bool select(const tuple_t &t) {
        return _filter.select(t);
    }
    virtual part_tscan_filter_t* clone() const {
        return new part_tscan_filter_t(*this);
    }
    virtual c_str to_string() const {
        return c_str("select P_PARTKEY, P_SIZE, P_CONTAINER, P_BRAND "
                     "where (brand = %s and size between 1 and %d) "
                     "or (brand = %s and size between 1 and %d) "
                     "or (brand = %s and size between 1 and %d) ",
                     _brands[0], _sizes[0],
                     _brands[1], _sizes[1],
                     _brands[2], _sizes[2]);
    }
};

/**
 * @brief select l_quantity, l_extendedprice, l_discount, l_partkey,
 * l_shipmode, l_shipinstruct from lineitem where l_shipinstruct =
 * ... and l_shipmode in (...) and ((...) or (...) or (...))
 */
struct lineitem_tscan_filter_t : tuple_filter_t {
    and_predicate_t _filter;
    int _quantities[3];
    lineitem_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_lineitem_tuple))
    {
        // TODO: randomize these
        int quantities[] = {1, 10, 20};
        memcpy(_quantities, quantities, sizeof(quantities));

        predicate_t* p;
        or_predicate_t* orp;
        size_t offset;

        // l_shipinstruct = ...
        offset = offsetof(tpch_lineitem_tuple, L_SHIPINSTRUCT);
        p = new string_predicate_t<>("DELIVER IN PERSON", offset);
        _filter.add(p);

        // l_shipmode in (...)
        offset = offsetof(tpch_lineitem_tuple, L_SHIPMODE);
        orp = new or_predicate_t();
        p = new scalar_predicate_t<tpch_l_shipmode>(AIR, offset);
        orp->add(p);
        p = new scalar_predicate_t<tpch_l_shipmode>(REG_AIR, offset);
        orp->add(p);
        _filter.add(orp);
        
        orp = new or_predicate_t();
        for(int i=0; i < 3; i++) {
            and_predicate_t* andp = new and_predicate_t();
            // l_quantity >= [quantity] and l_quantity <= [quantity]+10
            offset = offsetof(tpch_lineitem_tuple, L_QUANTITY);
            p = new scalar_predicate_t<double, greater_equal>(quantities[i], offset);
            andp->add(p);
            p = new scalar_predicate_t<double, less_equal>(quantities[i]+10, offset);
            andp->add(p);
            orp->add(andp);
        }
        _filter.add(orp);
    }
    virtual void project(tuple_t &d, const tuple_t &s) {
        lineitem_scan_tuple *dest = (lineitem_scan_tuple*) d.data;
        tpch_lineitem_tuple *src = (tpch_lineitem_tuple*) s.data;
        dest->L_QUANTITY = src->L_QUANTITY;
        dest->L_EXTENDEDPRICE = src->L_EXTENDEDPRICE;
        dest->L_DISCOUNT = src->L_DISCOUNT;
        dest->L_PARTKEY = src->L_PARTKEY;
        dest->L_SHIPMODE = src->L_SHIPMODE;
        memcpy(dest->L_SHIPINSTRUCT, src->L_SHIPINSTRUCT, sizeof(dest->L_SHIPINSTRUCT));
    }
    virtual bool select(const tuple_t &t) {
        return _filter.select(t);
    }
    virtual lineitem_tscan_filter_t* clone() const {
        return new lineitem_tscan_filter_t(*this);
    }
    virtual c_str to_string() const {
        return c_str("select L_QUANTITY, L_EXTENDEDPRICE, L_DISCOUNT, "
                     "L_PARTKEY, L_SHIPMODE, L_SHIPINSTRUCT "
                     "where L_SHIPINSTRUCT = 'DELIVER IN PERSON' "
                     "and L_SHIPMODE in (REG_AIR, AIR) "
                     "and ((L_QUANTITY >= %d and L_QUANTITY <= %d) "
                     "or (L_QUANTITY >= %d and L_QUANTITY <= %d) "
                     "or (L_QUANTITY >= %d and L_QUANTITY <= %d)) ",
                     _quantities[0], _quantities[0] + 10,
                     _quantities[1], _quantities[1] + 10,
                     _quantities[2], _quantities[2] + 10);
                     
    }
};

struct q19_join_t : tuple_join_t {
    static key_extractor_t* create_left_extractor() {
        size_t offset = offsetof(lineitem_scan_tuple, L_PARTKEY);
        return new int_key_extractor_t(sizeof(int), offset);
    }
    static key_extractor_t* create_right_extractor() {
        size_t offset = offsetof(part_scan_tuple, P_PARTKEY);
        return new int_key_extractor_t(sizeof(int), offset);
    }
    q19_join_t()
        : tuple_join_t(sizeof(lineitem_scan_tuple),
                       create_left_extractor(),
                       sizeof(part_scan_tuple),
                       create_right_extractor(),
                       new int_key_compare_t(),
                       sizeof(join_tuple))
    {
    }

    virtual void join(tuple_t &d, const tuple_t &l, const tuple_t &r) {
        join_tuple* dest = (join_tuple*) d.data;
        lineitem_scan_tuple* left = (lineitem_scan_tuple*) l.data;
        part_scan_tuple* right = (part_scan_tuple*) r.data;

        dest->L_QUANTITY = left->L_QUANTITY;
        dest->L_EXTENDEDPRICE = left->L_EXTENDEDPRICE;
        dest->L_DISCOUNT = left->L_DISCOUNT;
        dest->PARTKEY = left->L_PARTKEY;
        dest->P_SIZE = right->P_SIZE;
        dest->L_SHIPMODE = left->L_SHIPMODE;
        memcpy(dest->P_CONTAINER, right->P_CONTAINER, sizeof(right->P_CONTAINER));
        memcpy(dest->P_BRAND, right->P_BRAND, sizeof(right->P_BRAND));
        memcpy(dest->L_SHIPINSTRUCT, left->L_SHIPINSTRUCT, sizeof(left->L_SHIPINSTRUCT));
    }

    virtual q19_join_t* clone() const {
        return new q19_join_t(*this);
    }
    virtual c_str to_string() const {
        return "LINEITEM join PART, "
            "select L_QUANTITY, L_EXTENDEDPRICE, L_PARTKEY, P_SIZE, "
            "L_SHIPINSTRUCT, P_CONTAINER, P_BRAND, L_SHIPINSTRUCT";
    }
                        
};

/**
 * @brief select l_extendedprice, l_discount from join_result where
 * (p_brand = ... and p_container in (...) and p_size between ... and
 * l_quantity >= ... and l_quantity <= ...) or (...) or (...)
 */
struct q19_filter_t : tuple_filter_t {
    or_predicate_t _filter;
    q19_filter_t()
        : tuple_filter_t(sizeof(join_tuple))
    {
        // TODO use the values generated by the other two filters
        int sizes[] = {5, 10, 15};
        char* brands[] = {"Brand#12", "Brand#23", "Brand#34"};
        int quantities[] = {1, 10, 20};
        char* containers[3][4] = {
            {"SM CASE", "SM BOX", "SM PACK", "SM PKG"},
            {"MED BAG", "MED BOX", "MED PKG", "MED PACK"},
            {"LG CASE", "LG BOX", "LG PACK", "LG PKG"},
        };

        for(int i=0; i < 3; i++) {
            and_predicate_t* andp = new and_predicate_t();
            predicate_t* p;
            size_t offset;
            
            // p_brand = [brand]
            offset = offsetof(join_tuple, P_BRAND);
            p = new string_predicate_t<>(brands[i], offset);
            andp->add(p);

            // p_container in (...)
            offset = offsetof(join_tuple, P_CONTAINER);
            or_predicate_t* orp = new or_predicate_t();
            for(int j=0; j < 4; j++) {
                p = new string_predicate_t<>(containers[i][j], offset);
                orp->add(p);
            }
            andp->add(orp);
            
            // p_size between ...
            offset = offsetof(join_tuple, P_SIZE);
            p = new scalar_predicate_t<int, greater_equal>(1, offset);
            andp->add(p);
            p = new scalar_predicate_t<int, less_equal>(sizes[i], offset);
            andp->add(p);

            // l_quantity ...
            offset = offsetof(join_tuple, L_QUANTITY);
            double q = quantities[i];
            p = new scalar_predicate_t<double, greater_equal>(q, offset);
            andp->add(p);
            p = new scalar_predicate_t<double, less_equal>(q + 10, offset);
            andp->add(p);

            _filter.add(andp);
        }
    }
    virtual void project(tuple_t &d, const tuple_t &s) {
        q19_tuple *dest = (q19_tuple*) d.data;
        join_tuple *src = (join_tuple*) s.data;
        dest->L_EXTENDEDPRICE = src->L_EXTENDEDPRICE;
        dest->L_DISCOUNT = src->L_DISCOUNT;
    }
    virtual bool select(const tuple_t &t) {
        return _filter.select(t);
    }
    virtual q19_filter_t* clone() const {
        return new q19_filter_t(*this);
    }
    virtual c_str to_string() const {
        return "q19_filter_t";
    }
};

struct q19_sum_t : tuple_aggregate_t {
    default_key_extractor_t _extractor;
    
    q19_sum_t()
        : tuple_aggregate_t(sizeof(double)),
          _extractor(0, 0)
    {
    }

    virtual key_extractor_t* key_extractor() {
        return &_extractor;
    }
    virtual void aggregate(char* agg_data, const tuple_t &t) {
        double* dest = (double*) agg_data;
        q19_tuple* src = (q19_tuple*) t.data;

        *dest += src->L_EXTENDEDPRICE*(1 - src->L_DISCOUNT);
    }
    virtual void finish(tuple_t &dest, const char* agg_data) {
        memcpy(dest.data, agg_data, dest.size);
    }
    virtual q19_sum_t* clone() const {
        return new q19_sum_t(*this);
    }
    virtual c_str to_string() const {
        return "q19_sum_t";
    }
};

int main() {
    thread_init();
    db_open();
    TRACE_SET(TRACE_ALWAYS);


    // line up the stages...
    register_stage<tscan_stage_t>();
    register_stage<aggregate_stage_t>();
    register_stage<hash_join_stage_t>();


    for(int i=0; i < 5; i++) {

        stopwatch_t timer;

        // lineitem scan
        tuple_filter_t* filter = new lineitem_tscan_filter_t();
        tuple_fifo* buffer = new tuple_fifo(sizeof(lineitem_scan_tuple), dbenv);
        packet_t* lineitem_packet;
        lineitem_packet = new tscan_packet_t("lineitem TSCAN",
                                             buffer,
                                             filter,
                                             tpch_lineitem);

        // part scan
        filter = new part_tscan_filter_t();
        buffer = new tuple_fifo(sizeof(part_scan_tuple), dbenv);
        packet_t* part_packet;
        part_packet = new tscan_packet_t("part TSCAN",
                                         buffer,
                                         filter,
                                         tpch_part);

        // join
        filter = new q19_filter_t();
        buffer = new tuple_fifo(sizeof(q19_tuple), dbenv);
        packet_t* join_packet;
        join_packet = new hash_join_packet_t("lineitem-part HJOIN",
                                             buffer, filter,
                                             lineitem_packet,
                                             part_packet,
                                             new q19_join_t());

        // sum
        filter = new trivial_filter_t(sizeof(double));
        buffer = new tuple_fifo(sizeof(double), dbenv);
        key_extractor_t* extractor = new default_key_extractor_t(0, 0);
        packet_t* sum_packet;
        sum_packet = new aggregate_packet_t("final SUM",
                                            buffer, filter,
                                            new q19_sum_t(),
                                            extractor,
                                            join_packet);

        // go!
        dispatcher_t::dispatch_packet(sum_packet);
        
        tuple_t output;
        while(!buffer->get_tuple(output)) {
            double* r = (double*) output.data;
            TRACE(TRACE_ALWAYS, "*** Q19 Revenue: %lf\n", *r);
        }

        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }
    
    db_close();
    return 0;
}
