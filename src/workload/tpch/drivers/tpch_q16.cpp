/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "scheduler.h"
#include "workload/tpch/drivers/tpch_q16.h"

#include "workload/common.h"
#include "workload/tpch/tpch_db.h"

using namespace qpipe;

ENTER_NAMESPACE(workload);

struct supplier_tscan_filter_t : public tuple_filter_t {
    char const *word1;
    char const *word2;
        
    supplier_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_supplier_tuple))
    {
        word1 = "Customer";
        word2 = "Complaints";
    }

    virtual bool select(const tuple_t &t) {
        tpch_supplier_tuple* tuple = aligned_cast<tpch_supplier_tuple>(t.data);
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
        tpch_supplier_tuple* src = aligned_cast<tpch_supplier_tuple>(s.data);
        int* dest = aligned_cast<int>(d.data);
        *dest = src->S_SUPPKEY;
    }
    virtual supplier_tscan_filter_t* clone() const {
        return new supplier_tscan_filter_t(*this);
    }
    virtual c_str to_string() const {
        return c_str("select S_SUPPKEY where S_COMMENT like %%%s%%%s%%",
                     word1, word2);
    }
};

/**
 * @brief select s_suppkey from supplier where s_comment like
 * "%Customer%Complaints%"
 */
packet_t *supplier_scan(page_list* tpch_supplier) {
    tuple_filter_t* filter = new supplier_tscan_filter_t();
    tuple_fifo* buffer = new tuple_fifo(sizeof(int));
    packet_t* tscan_packet = new tscan_packet_t("supplier TSCAN",
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
    char brand[32];
    char type[32];
    int sizes[8];

    part_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_part_tuple))
    {
        // TODO: randomize
        if(0) {
#if 1
            strcpy(brand, "Brand#45");
            strcpy(type, "MEDIUM POLISHED");
            int base_sizes[] = {49, 14, 23, 45, 19, 3, 36, 9};
#else
            strcpy(brand, "Brand#33");
            strcpy(type, "LARGE PLATED");
            int base_sizes[] = {16, 19, 11, 6, 23, 42, 48, 13};
#endif
            memcpy(sizes, base_sizes, sizeof(base_sizes));
        }
        else {
            thread_t* self = thread_get_self();
            sprintf(brand, "Brand#%1d%1d", self->rand(5) + 1, self->rand(5) + 1);
            char const* TYPE1[] = {"STANDARD", "SMALL", "MEDIUM", "LARGE", "PROMO", "ECONOMY"};
            char const* TYPE2[] = {"ANODIZED", "BURNISHED", "PLATED", "POLISHED", "BRUSHED"};
            sprintf(type, "%s %s", TYPE1[self->rand(6)], TYPE2[self->rand(5)]);
            for(int i=0; i < 8; i++) {
                int j = -1;
                int size=0;
                // loop until we get a unique value
                while(j != i) {
                    size = self->rand(50) + 1;
                    for(j=0; j < i && sizes[j] != size; j++);
                    if(j == i)
                        break;
                }
                sizes[i] = size;
            }
        }
    }
    virtual bool select(const tuple_t &s) {
        tpch_part_tuple* part = aligned_cast<tpch_part_tuple>(s.data);

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
        tpch_part_tuple* src = aligned_cast<tpch_part_tuple>(s.data);
        part_scan_tuple_t* dest = aligned_cast<part_scan_tuple_t>(d.data);

        dest->p_partkey = src->P_PARTKEY;
        dest->p_size = src->P_SIZE;
        memcpy(dest->p_brand, src->P_BRAND, sizeof(src->P_BRAND));
        memcpy(dest->p_type, src->P_TYPE, sizeof(src->P_TYPE));
    }
    virtual part_tscan_filter_t* clone() const {
        return new part_tscan_filter_t(*this);
    }
    virtual c_str to_string() const {
        return c_str("select P_PARTKEY, P_SIZE, P_BRAND, P_TYPE "
                     "where P_BRAND <> %s and P_TYPE not like %s%%",
                     brand, type);
    }
};

/**
 * @brief select p_brand, p_type, p_size, p_partkey from part where
 * p_brand <> [brand] and p_type not like '[type]%' and p_size in
 * (...)
 */
packet_t* part_scan(page_list* tpch_part) {
    tuple_filter_t* filter = new part_tscan_filter_t();
    tuple_fifo* buffer = new tuple_fifo(sizeof(part_scan_tuple_t));
    packet_t* tscan_packet = new tscan_packet_t("part TSCAN",
                                                buffer,
                                                filter,
                                                tpch_part);

    return tscan_packet;
}

struct part_supp_tuple_t {
    static int const ALIGN;
    int PART_KEY;
    int SUPP_KEY;
};
int const part_supp_tuple_t::ALIGN = sizeof(int);

struct partsupp_tscan_filter_t : public tuple_filter_t {
    partsupp_tscan_filter_t()
        : tuple_filter_t(sizeof(tpch_partsupp_tuple))
    {
    }
    virtual void project(tuple_t &d, const tuple_t &s) {
        tpch_partsupp_tuple* src = aligned_cast<tpch_partsupp_tuple>(s.data);
        part_supp_tuple_t* dest = aligned_cast<part_supp_tuple_t>(d.data);
        dest->PART_KEY = src->PS_PARTKEY;
        dest->SUPP_KEY = src->PS_SUPPKEY;
    }
    virtual partsupp_tscan_filter_t* clone() const {
        return new partsupp_tscan_filter_t(*this);
    }
    virtual c_str to_string() const {
        return "select PS_PARTKEY, PS_SUPPKEY";
    }
};

packet_t* partsupp_scan(page_list* tpch_partsupp) {
    tuple_filter_t* filter = new partsupp_tscan_filter_t();
    tuple_fifo* buffer = new tuple_fifo(sizeof(part_supp_tuple_t));
    packet_t* tscan_packet = new tscan_packet_t("partsupp TSCAN",
                                                buffer,
                                                filter,
                                                tpch_partsupp);

    // sort in preparation for the not-in test
    filter = new trivial_filter_t(sizeof(part_supp_tuple_t));
    buffer = new tuple_fifo(sizeof(part_supp_tuple_t));
    size_t offset = offsetof(part_supp_tuple_t, SUPP_KEY);
    packet_t* sort_packet = new sort_packet_t("Partsupp SORT",
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
        part_supp_tuple_t* src = aligned_cast<part_supp_tuple_t>(s.data);
        *aligned_cast<int>(dest.data) = src->PART_KEY;
    }
    virtual partsupp_filter_t* clone() const {
        return new partsupp_filter_t(*this);
    }
    virtual c_str to_string() const {
        return "select PART_KEY";
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
        part_scan_tuple_t* dest = aligned_cast<part_scan_tuple_t>(d.data);
        // double cheat -- use p_partkey to store the ps_suppkey, and
        // also project out the real part key instead of using a
        // filter after the fact
        memcpy(dest, r.data, sizeof(part_scan_tuple_t));
        part_supp_tuple_t *left = aligned_cast<part_supp_tuple_t>(l.data);
        dest->p_partkey = left->SUPP_KEY;
    }

    virtual q16_join_t* clone() const {
        return new q16_join_t(*this);
    }
    virtual c_str to_string() const {
        return "q16_join, select SUPP_KEY";
    }
};

struct q16_compare1_t : public key_compare_t {
    virtual int operator()(const void* key1, const void* key2) const {
        part_scan_tuple_t* a = aligned_cast<part_scan_tuple_t>(key1);
        part_scan_tuple_t* b = aligned_cast<part_scan_tuple_t>(key2);

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
    virtual q16_compare1_t* clone() const {
        return new q16_compare1_t(*this);
    }
};

struct q16_extractor1_t : public key_extractor_t {
    q16_extractor1_t()
        : key_extractor_t(sizeof(part_scan_tuple_t))
    {
    }
#if 1
    virtual int extract_hint(const char* key) const{
        return 0;
        part_scan_tuple_t* pst = aligned_cast<part_scan_tuple_t>(key);
        int result;
        memcpy(&result, pst->p_brand, sizeof(int));
        return result;
    }
#endif
    virtual q16_extractor1_t* clone() const {
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
    virtual q16_aggregate1_t* clone() const {
        return new q16_aggregate1_t(*this);
    }
    virtual c_str to_string() const {
        return "q16_aggregate1_t";
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
    virtual int extract_hint(const char* key) const {
        part_scan_tuple_t* pst = aligned_cast<part_scan_tuple_t>(key - key_offset());
        int result;
        memcpy(&result, pst->p_brand, sizeof(int));
        return result;
    }
    virtual q16_extractor2_t* clone() const {
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
        part_scan_tuple_t* agg = aligned_cast<part_scan_tuple_t>(agg_data);

        // use the "part key" field to store the count
        agg->p_partkey++;
    }
    virtual void finish(tuple_t &d, const char* agg_data) {
        memcpy(d.data, agg_data, tuple_size());
    }
    virtual q16_aggregate2_t* clone() const {
        return new q16_aggregate2_t(*this);
    }
    virtual c_str to_string() const {
        return "q16_aggregate2_t";
    }
};

struct q16_extractor3_t : public key_extractor_t {
    q16_extractor3_t()
        : key_extractor_t(sizeof(part_scan_tuple_t))
    {
    }
    virtual int extract_hint(const char* key) const {
        return -*aligned_cast<int>(key);
    }
    virtual q16_extractor3_t* clone() const {
        return new q16_extractor3_t(*this);
    }
};

struct q16_compare2_t : public key_compare_t {
    virtual int operator()(const void* key1, const void* key2) const {
        part_scan_tuple_t* a = aligned_cast<part_scan_tuple_t>(key1);
        part_scan_tuple_t* b = aligned_cast<part_scan_tuple_t>(key2);

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
    virtual q16_compare2_t* clone() const {
        return new q16_compare2_t(*this);
    }
};



void tpch_q16_driver::submit(void* disp) {

    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
    qpipe::query_state_t* qs = dp->query_state_create();
    

    // figure out where the SUPP_KEY lives inside part_supp_tuple_t
    part_supp_tuple_t* pst = NULL;
    size_t offset = (size_t) &pst->SUPP_KEY;
    
    // ps_suppkey not in (select ... from supplier ...)
    tuple_filter_t* filter = new trivial_filter_t(sizeof(part_supp_tuple_t));
    tuple_fifo* buffer = new tuple_fifo(sizeof(part_supp_tuple_t));


    packet_t* partsupp_scan_packet = partsupp_scan(tpch_partsupp);
    packet_t* supplier_scan_packet = supplier_scan(tpch_supplier);


    packet_t* not_in_packet = new sorted_in_packet_t("ps_suppkey NOT IN",
                                                     buffer,
                                                     filter,
                                                     partsupp_scan_packet,
                                                     supplier_scan_packet,
                                                     new int_key_extractor_t(sizeof(int), offset),
                                                     new int_key_extractor_t(),
                                                     new int_key_compare_t(),
                                                     true);
    
    // join with part
    filter = new trivial_filter_t(sizeof(part_scan_tuple_t));
    buffer = new tuple_fifo(sizeof(part_scan_tuple_t));


    packet_t* part_scan_packet = part_scan(tpch_part);
    packet_t* join_packet = new hash_join_packet_t("Part JOIN",
                                                   buffer,
                                                   filter,
                                                   not_in_packet,
                                                   part_scan_packet,
                                                   new q16_join_t());
    
    // aggregate to make ps_suppkey distinct
    filter = new trivial_filter_t(sizeof(part_scan_tuple_t));
    buffer = new tuple_fifo(sizeof(part_scan_tuple_t));
    
    key_extractor_t* extractor = new q16_extractor1_t();
    key_compare_t* compare = new q16_compare1_t();
    tuple_aggregate_t* aggregate = new q16_aggregate1_t();
    packet_t* pagg_packet = new partial_aggregate_packet_t("Group by #1",
                                                           buffer,
                                                           filter,
                                                           join_packet,
                                                           aggregate,
                                                           extractor,
                                                           compare);

    // aggregate again to count ps_suppkey
    filter = new trivial_filter_t(sizeof(part_scan_tuple_t));
    buffer = new tuple_fifo(sizeof(part_scan_tuple_t));
    extractor = new q16_extractor2_t();
    aggregate = new q16_aggregate2_t();
    packet_t* agg_packet;
    agg_packet = new aggregate_packet_t("Group by #2",
                                        buffer,
                                        filter,
                                        aggregate,
                                        extractor,
                                        pagg_packet);
    
    
    // sort the output
    filter = new trivial_filter_t(sizeof(part_scan_tuple_t));
    buffer = new tuple_fifo(sizeof(part_scan_tuple_t));
    extractor = new q16_extractor3_t();
    compare = new q16_compare2_t();
    packet_t* sort_packet = new sort_packet_t("Final SORT",
                                              buffer,
                                              filter,
                                              extractor,
                                              compare,
                                              agg_packet);
    

    // assign packets to cpus...
    partsupp_scan_packet->assign_query_state(qs);
    supplier_scan_packet->assign_query_state(qs);
    not_in_packet->assign_query_state(qs);
    part_scan_packet->assign_query_state(qs);
    join_packet->assign_query_state(qs);
    pagg_packet->assign_query_state(qs);
    agg_packet->assign_query_state(qs);
    sort_packet->assign_query_state(qs);
    
    // Dispatch packet
    dispatcher_t::dispatch_packet(sort_packet);
    guard<tuple_fifo> result = sort_packet->output_buffer();
    
    TRACE(TRACE_ALWAYS,
          "*** Q16 %10s %25s %10s %10s\n",
          "Brand", "Type", "Size", "Suppliers");
    
    tuple_t output;
    while(result->get_tuple(output)) {
        part_scan_tuple_t* r = aligned_cast<part_scan_tuple_t>(output.data);
        TRACE(TRACE_QUERY_RESULTS,
              "*** Q16 %10s %25s %10d %10d\n",
              r->p_brand, r->p_type, r->p_size, r->p_partkey);
    }


    dp->query_state_destroy(qs);
}

EXIT_NAMESPACE(workload);
