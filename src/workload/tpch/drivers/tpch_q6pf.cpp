/* -*- mode:C++; c-basic-offset:4 -*- */
#include "stages.h"
#include "scheduler.h"

#include "workload/tpch/drivers/tpch_q6pf.h"
#include "workload/common/q6_packet.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "workload/tpch/tpch_env.h"
#include "workload/common.h"


ENTER_NAMESPACE(q6pf);


class q6pf_aggregate_t : public tuple_aggregate_t {

public:

    class q6pf_key_extractor_t : public key_extractor_t {
    public:
        q6pf_key_extractor_t()
            : key_extractor_t(0, 0)
        {
        }

        virtual int extract_hint(const char*) const {
            /* should never be called! */
            unreachable();
            return 0;
        }

        virtual key_extractor_t* clone() const {
            return new q6pf_key_extractor_t(*this);
        }
    };
    
private:

    q6pf_key_extractor_t    _extractor;
    q6_tscan_filter_t       _tscan_filter;
    q6_count_aggregate_t    _count_aggregate;

public:

    q6pf_aggregate_t()
        : tuple_aggregate_t(sizeof(q6_agg_t)),
          _extractor(),
          _tscan_filter(),
          _count_aggregate()
    {
    }

    virtual key_extractor_t* key_extractor() { return &_extractor; }
    
    virtual void aggregate(char* agg_data, const tuple_t& src) {
     
        /* Go through the motions if and only if the TSCAN would have
           selected this tuple. */
        if (_tscan_filter.select(src)) {
            decimal d[2];
            tuple_t temp((char*)&d, sizeof(d));
            _tscan_filter.project(temp, src);
            _count_aggregate.aggregate(agg_data, temp);
        }
    }
    
    virtual void finish(tuple_t &dest, const char* agg_data) {
        _count_aggregate.finish(dest, agg_data);
    }

    virtual q6pf_aggregate_t* clone() const {
        return new q6pf_aggregate_t(*this);
    }

    virtual c_str to_string() const {
        return "q6pf_aggregate_t";
    }
};


EXIT_NAMESPACE(q6pf);


using namespace q6pf;
using namespace workload;


ENTER_NAMESPACE(workload);


using namespace qpipe;


void tpch_q6pf_driver::submit(void* disp) {

    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
    // TSCAN
    struct cdb_table_s* tpch_lineitem = &tpch_tables[TPCH_TABLE_LINEITEM];
    tuple_fifo* tscan_output = new tuple_fifo(tpch_lineitem->tuple_size);
    tscan_packet_t *q6_tscan_packet =
        new tscan_packet_t("Q6PF_TSCAN_PACKET",
                           tscan_output,
                           new trivial_filter_t(tscan_output->tuple_size()),
                           tpch_lineitem->db);
    
    // AGGREGATE
    tuple_fifo* agg_output = new tuple_fifo(sizeof(q6_agg_t));
    aggregate_packet_t* q6_agg_packet =
        new aggregate_packet_t(c_str("Q6PF_AGGREGATE_PACKET"),
                               agg_output,
                               new trivial_filter_t(agg_output->tuple_size()),
                               new q6pf_aggregate_t(),
                               new q6pf_aggregate_t::q6pf_key_extractor_t(),
                               q6_tscan_packet);
    
    qpipe::query_state_t* qs = dp->query_state_create();
    q6_agg_packet->assign_query_state(qs);
    q6_tscan_packet->assign_query_state(qs);

    q6_process_tuple_t pt;
    process_query(q6_agg_packet, pt);

    dp->query_state_destroy(qs);
}


EXIT_NAMESPACE(workload);
