// -*- mode:C++; c-basic-offset:4 -*-
#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"
#include "engine/stages/partial_aggregate.h"
#include "engine/stages/sort.h"
#include "engine/stages/hash_join.h"
#include "engine/stages/fdump.h"
#include "engine/stages/fscan.h"
#include "engine/stages/hash_join.h"
#include "engine/stages/sorted_in.h"
#include "engine/dispatcher.h"
#include "engine/util/stopwatch.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common.h"
#include "tests/common/tpch_db.h"

using namespace qpipe;

struct supplier_tscan_filter_t : public tuple_filter_t {
    char *word1;
    char *word2;
        
    supplier_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_supplier_tuple))
    {
        word1 = "Customer";
        word2 = "Complaints";
    }

    virtual bool select(const tuple_t &t) {
        tpch_supplier_tuple* tuple = (tpch_supplier_tuple*) t.data;
        // search for all instances of the first substring. Make sure
        // the second search is *after* the first...
        char* first = strstr(tuple->S_COMMENT, word1);
        if(!first)
            return false;

        char* second = strstr(first + strlen(word1), word2);
        if(!second)
            return false;

        // if we got here, match (and therefore accept)
        return true;
        
    }
    
    virtual void project(tuple_t &d, const tuple_t &s) {
        tpch_supplier_tuple* src = (tpch_supplier_tuple*) s.data;
        int* dest = (int*) d.data;
        *dest = src->S_SUPPKEY;
    }
    virtual supplier_tscan_filter_t* clone() {
        return new supplier_tscan_filter_t(*this);
    }
};

/**
 * @brief select s_suppkey from supplier where s_comment like
 * "%Customer%Complaints%"
 */
packet_t *supplier_scan(Db* tpch_supplier) {
    char* packet_id = copy_string("Supplier TSCAN");
    tuple_filter_t* filter = new supplier_tscan_filter_t();
    tuple_buffer_t* buffer = new tuple_buffer_t(sizeof(int));
    packet_t* tscan_packet = new tscan_packet_t(packet_id,
                                                buffer,
                                                filter,
                                                tpch_supplier);

    return tscan_packet;
}

struct part_scan_tuple_t {
    int p_partkey;
    int p_size;
    char p_brand[STRSIZE(10)];
    char p_type[STRSIZE(25)];
};

struct part_tscan_filter_t : public tuple_filter_t {
    char* brand;
    char* type;
    int sizes[8];

    part_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_part_tuple))
    {
        // TODO: randomize
#if 1
        brand = "Brand#45";
        type = "MEDIUM POLISHED";
        int base_sizes[] = {49, 14, 23, 45, 19, 3, 36, 9};
#else
        brand = "Brand#33";
        type = "LARGE PLATED";
        int base_sizes[] = {16, 19, 11, 6, 23, 42, 48, 13};
#endif
        memcpy(sizes, base_sizes, sizeof(base_sizes));
    }
    virtual bool select(const tuple_t &s) {
        tpch_part_tuple* part = (tpch_part_tuple*) s.data;

        // p_brand <> '[brand]'. 
        if(!strcmp(part->P_BRAND, brand))
            return false;

        // p_type not like '[type]%'. Note that we're doing a prefix
        // search here.
        if(!strncasecmp(part->P_TYPE, type, strlen(type)))
            return false;

        // p_size in (...)
        for(int i=0; i < 8; i++)
            if(part->P_SIZE == sizes[i])
                return true;

        // no size match
        return false;
    }
    virtual void project(tuple_t &d, const tuple_t &s) {
        tpch_part_tuple* src = (tpch_part_tuple*) s.data;
        part_scan_tuple_t* dest = (part_scan_tuple_t*) d.data;

        dest->p_partkey = src->P_PARTKEY;
        dest->p_size = src->P_SIZE;
        memcpy(dest->p_brand, src->P_BRAND, sizeof(src->P_BRAND));
        memcpy(dest->p_type, src->P_TYPE, sizeof(src->P_TYPE));
    }
    virtual part_tscan_filter_t* clone() {
        return new part_tscan_filter_t(*this);
    }
};

/**
 * @brief select p_brand, p_type, p_size, p_partkey from part where
 * p_brand <> [brand] and p_type not like '[type]%' and p_size in
 * (...)
 */
packet_t* part_scan(Db* tpch_part) {
    char* packet_id = copy_string("Part TSCAN");
    tuple_filter_t* filter = new part_tscan_filter_t();
    tuple_buffer_t* buffer = new tuple_buffer_t(sizeof(part_scan_tuple_t));
    packet_t* tscan_packet = new tscan_packet_t(packet_id,
                                                buffer,
                                                filter,
                                                tpch_part);

    return tscan_packet;
}

struct part_supp_tuple_t {
    int PART_KEY;
    int SUPP_KEY;
};

struct partsupp_tscan_filter_t : public tuple_filter_t {
    partsupp_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_partsupp_tuple))
    {
    }
    virtual void project(tuple_t &d, const tuple_t &s) {
        tpch_partsupp_tuple* src = (tpch_partsupp_tuple*) s.data;
        part_supp_tuple_t* dest = (part_supp_tuple_t*) d.data;
        dest->PART_KEY = src->PS_PARTKEY;
        dest->SUPP_KEY = src->PS_SUPPKEY;
    }
    virtual partsupp_tscan_filter_t* clone() {
        return new partsupp_tscan_filter_t(*this);
    }
};

packet_t* partsupp_scan(Db* tpch_partsupp) {
    char* packet_id = copy_string("Partsupp TSCAN");
    tuple_filter_t* filter = new partsupp_tscan_filter_t();
    tuple_buffer_t* buffer = new tuple_buffer_t(sizeof(part_supp_tuple_t));
    packet_t* tscan_packet = new tscan_packet_t(packet_id,
                                                buffer,
                                                filter,
                                                tpch_partsupp);

    // sort in preparation for the not-in test
    packet_id = copy_string("Partsupp SORT");
    filter = new trivial_filter_t(sizeof(part_supp_tuple_t));
    buffer = new tuple_buffer_t(sizeof(part_supp_tuple_t));
    size_t offset = (size_t) &((part_supp_tuple_t*)NULL)->SUPP_KEY;
    packet_t* sort_packet = new sort_packet_t(packet_id,
                                              buffer,
                                              filter,
                                              new int_key_extractor_t(sizeof(int), offset),
                                              new int_key_compare_t(),
                                              tscan_packet);
                                              
    return sort_packet;
}

struct partsupp_filter_t : public tuple_filter_t {
    partsupp_filter_t()
        : tuple_filter_t(sizeof(part_supp_tuple_t))
    {
    }
    virtual void project(tuple_t &dest, const tuple_t &s) {
        part_supp_tuple_t* src = (part_supp_tuple_t*) s.data;
        memcpy(dest.data, &src->PART_KEY, sizeof(int));
    }
    virtual partsupp_filter_t* clone() {
        return new partsupp_filter_t(*this);
    }
};

struct q16_join_t : public tuple_join_t {
    q16_join_t()
        : tuple_join_t(sizeof(int),
                       new int_key_extractor_t(),
                       sizeof(part_scan_tuple_t),
                       new int_key_extractor_t(),
                       new int_key_compare_t(),
                       sizeof(part_scan_tuple_t))
    {
    }

    virtual void join(tuple_t &d, const tuple_t &l, const tuple_t &r) {
        part_scan_tuple_t* dest = (part_scan_tuple_t*) d.data;
        // double cheat -- use p_partkey to store the ps_suppkey, and
        // also project out the real part key instead of using a
        // filter after the fact
        memcpy(dest, r.data, sizeof(part_scan_tuple_t));
        part_supp_tuple_t *left = (part_supp_tuple_t*) l.data;
        dest->p_partkey = left->SUPP_KEY;
    }

    virtual q16_join_t* clone() {
        return new q16_join_t(*this);
    }
};

struct q16_compare1_t : public key_compare_t {
    virtual int operator()(const void* key1, const void* key2) {
        part_scan_tuple_t* a = (part_scan_tuple_t*) key1;
        part_scan_tuple_t* b = (part_scan_tuple_t*) key2;

        // sort by brand...
        int diff = strcmp(a->p_brand, b->p_brand);
        if(diff) return diff;

        // ... type ...
        diff = strcmp(a->p_type, b->p_type);
        if(diff) return diff;

        // ... size ...
        diff = a->p_size - b->p_size;
        if(diff) return diff;

        // ... suppkey
        diff = a->p_partkey - b->p_partkey;
        if(diff) return diff;

        return 0;
    }
    virtual q16_compare1_t* clone() {
        return new q16_compare1_t(*this);
    }
};

struct q16_extractor1_t : public key_extractor_t {
    q16_extractor1_t()
        : key_extractor_t(sizeof(part_scan_tuple_t))
    {
    }
#if 1
    virtual int extract_hint(const char* key) {
        return 0;
        part_scan_tuple_t* pst = (part_scan_tuple_t*) key;
        int result;
        memcpy(&result, pst->p_brand, sizeof(int));
        return result;
    }
#endif
    virtual q16_extractor1_t* clone() {
        return new q16_extractor1_t(*this);
    }
};

struct q16_aggregate1_t : public tuple_aggregate_t {
    q16_extractor1_t _extractor;
    q16_aggregate1_t()
        : tuple_aggregate_t(sizeof(part_scan_tuple_t))
    {
    }
    virtual key_extractor_t* key_extractor() { return &_extractor; }
    virtual void aggregate(char*, const tuple_t &) {
        // do nothing -- no data fields
    }
    virtual void finish(tuple_t &d, const char* agg_data) {
        memcpy(d.data, agg_data, tuple_size());
    }
    virtual q16_aggregate1_t* clone() {
        return new q16_aggregate1_t(*this);
    }
};

struct q16_extractor2_t : public key_extractor_t {
    static size_t calc_key_offset() {
        part_scan_tuple_t* pst = NULL;
        return (size_t) &pst->p_size;
    }
    q16_extractor2_t()
        : key_extractor_t(sizeof(part_scan_tuple_t) - calc_key_offset(),
                          calc_key_offset())
    {
    }
    virtual int extract_hint(const char* key) {
        part_scan_tuple_t* pst = (part_scan_tuple_t*) (key - key_offset());
        int result;
        memcpy(&result, pst->p_brand, sizeof(int));
        return result;
    }
    virtual q16_extractor2_t* clone() {
        return new q16_extractor2_t(*this);
    }
};


struct q16_aggregate2_t : public tuple_aggregate_t {
    q16_extractor2_t _extractor;
    q16_aggregate2_t()
        : tuple_aggregate_t(sizeof(part_scan_tuple_t))
    {
    }
    virtual key_extractor_t* key_extractor() { return &_extractor; }
    virtual void aggregate(char* agg_data, const tuple_t &) {
        part_scan_tuple_t* agg = (part_scan_tuple_t*) agg_data;

        // use the "part key" field to store the count
        agg->p_partkey++;
    }
    virtual void finish(tuple_t &d, const char* agg_data) {
        memcpy(d.data, agg_data, tuple_size());
    }
    virtual q16_aggregate2_t* clone() {
        return new q16_aggregate2_t(*this);
    }
};

struct q16_extractor3_t : public key_extractor_t {
    q16_extractor3_t()
        : key_extractor_t(sizeof(part_scan_tuple_t))
    {
    }
    virtual int extract_hint(const char* key) {
        return -*(int*)key;
    }
    virtual q16_extractor3_t* clone() {
        return new q16_extractor3_t(*this);
    }
};

struct q16_compare2_t : public key_compare_t {
    virtual int operator()(const void* key1, const void* key2) {
        part_scan_tuple_t* a = (part_scan_tuple_t*) key1;
        part_scan_tuple_t* b = (part_scan_tuple_t*) key2;

        // sort by suppkey desc ... (hint took care of it)
        
        // ... brand...
        int diff = strcmp(a->p_brand, b->p_brand);
        if(diff) return diff;

        // ... type ...
        diff = strcmp(a->p_type, b->p_type);
        if(diff) return diff;

        // ... size 
        diff = a->p_size - b->p_size;
        if(diff) return diff;

        return 0;
    }
    virtual q16_compare2_t* clone() {
        return new q16_compare2_t(*this);
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
    register_stage<aggregate_stage_t>();
    register_stage<partial_aggregate_stage_t>();
    register_stage<sort_stage_t>();
    register_stage<merge_stage_t>();
    register_stage<fdump_stage_t>();
    register_stage<fscan_stage_t>(20);
    register_stage<hash_join_stage_t>();
    register_stage<sorted_in_stage_t>();


    for(int i=0; i < 1; i++) {

        stopwatch_t timer;


        // figure out where the SUPP_KEY lives inside part_supp_tuple_t
        part_supp_tuple_t* pst = NULL;
        size_t offset = (size_t) &pst->SUPP_KEY;
        
        // ps_suppkey not in (select ... from supplier ...)
        char* packet_id = copy_string("ps_suppkey NOT IN");
        tuple_filter_t* filter = new trivial_filter_t(sizeof(part_supp_tuple_t));
        tuple_buffer_t* buffer = new tuple_buffer_t(sizeof(part_supp_tuple_t));
        packet_t* not_in_packet;
        not_in_packet = new sorted_in_packet_t(packet_id,
                                               buffer,
                                               filter,
                                               partsupp_scan(tpch_partsupp),
                                               supplier_scan(tpch_supplier),
                                               new int_key_extractor_t(sizeof(int), offset),
                                               new int_key_extractor_t(),
                                               new int_key_compare_t(),
                                               true);

        // join with part
        packet_id = copy_string("Part JOIN");
        filter = new trivial_filter_t(sizeof(part_scan_tuple_t));
        buffer = new tuple_buffer_t(sizeof(part_scan_tuple_t));
        packet_t* join_packet = new hash_join_packet_t(packet_id,
                                                       buffer,
                                                       filter,
                                                       not_in_packet,
                                                       part_scan(tpch_part),
                                                       new q16_join_t());
                                                       
        // aggregate to make ps_suppkey distinct
        packet_id = copy_string("Group by #1");
        filter = new trivial_filter_t(sizeof(part_scan_tuple_t));
        buffer = new tuple_buffer_t(sizeof(part_scan_tuple_t));

        key_extractor_t* extractor = new q16_extractor1_t();
        key_compare_t* compare = new q16_compare1_t();
        tuple_aggregate_t* aggregate = new q16_aggregate1_t();
        packet_t* pagg_packet;
        pagg_packet = new partial_aggregate_packet_t(packet_id,
                                                     buffer,
                                                     filter,
                                                     join_packet,
                                                     aggregate,
                                                     extractor,
                                                     compare);

        // aggregate again to count ps_suppkey
        packet_id = copy_string("Group by #2");
        filter = new trivial_filter_t(sizeof(part_scan_tuple_t));
        buffer = new tuple_buffer_t(sizeof(part_scan_tuple_t));
        extractor = new q16_extractor2_t();
        aggregate = new q16_aggregate2_t();
        packet_t* agg_packet;
        agg_packet = new aggregate_packet_t(packet_id,
                                            buffer,
                                            filter,
                                            aggregate,
                                            extractor,
                                            pagg_packet);
                                            

        // sort the output
        packet_id = copy_string("Final SORT");
        filter = new trivial_filter_t(sizeof(part_scan_tuple_t));
        buffer = new tuple_buffer_t(sizeof(part_scan_tuple_t));
        extractor = new q16_extractor3_t();
        compare = new q16_compare2_t();
        packet_t* sort_packet = new sort_packet_t(packet_id,
                                                  buffer,
                                                  filter,
                                                  extractor,
                                                  compare,
                                                  agg_packet);
        
        // Dispatch packet
        dispatcher_t::dispatch_packet(sort_packet);
        buffer = sort_packet->_output_buffer;
        
        tuple_t output;
        TRACE(TRACE_ALWAYS,
              "*** Q16 %10s %25s %10s %10s\n",
              "Brand", "Type", "Size", "Suppliers");
        while(!buffer->get_tuple(output)) {
            part_scan_tuple_t* r = (part_scan_tuple_t*) output.data;
            TRACE(TRACE_ALWAYS,
                  "*** Q16 %10s %25s %10d %10d\n",
                  r->p_brand, r->p_type, r->p_size, r->p_partkey);
        }
        


        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }
    
    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
