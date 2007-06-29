/* -*- mode:C++; c-basic-offset:4 -*- */
#include "stages.h"
#include "scheduler.h"

#include "workload/common.h"

#include "workload/tpch/drivers/tpch_q6pipe.h"
#include "workload/tpch/q6_packet.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "workload/tpch/tpch_env.h"



ENTER_NAMESPACE(q6pipe);



/**
 * @brief This sieve receives a decimal[2] and output a decimal.
 */
class q6pipe_sieve_t : public tuple_sieve_t {

public:

    q6pipe_sieve_t()
        : tuple_sieve_t(sizeof(decimal))
    {
    }
    
    virtual bool pass(tuple_t& dest, const tuple_t &src) {
        decimal* in = aligned_cast<decimal>(src.data);
        decimal* out = aligned_cast<decimal>(dest.data);
        *out = in[0] * in[1];
        return true;
    }
    
    virtual tuple_sieve_t* clone() const {
        return new q6pipe_sieve_t(*this);
    }

    virtual c_str to_string() const {
        return "q6pipe_sieve_t";
    }
};



class q6pipe_aggregate_t : public tuple_aggregate_t {

public:

    class q6pipe_key_extractor_t : public key_extractor_t {
    public:
        q6pipe_key_extractor_t()
            : key_extractor_t(0, 0)
        {
        }

        virtual int extract_hint(const char*) const {
            /* should never be called! */
            unreachable();
            return 0;
        }

        virtual key_extractor_t* clone() const {
            return new q6pipe_key_extractor_t(*this);
        }
    };
    
private:

    q6pipe_key_extractor_t _extractor;

public:

    q6pipe_aggregate_t()
        : tuple_aggregate_t(sizeof(q6_agg_t)),
          _extractor()
    {
    }

    virtual key_extractor_t* key_extractor() { return &_extractor; }
    
    virtual void aggregate(char* agg_data, const tuple_t& src) {
        q6_agg_t* agg = aligned_cast<q6_agg_t>(agg_data);
	decimal * d = aligned_cast<decimal>(src.data);
        agg->count++;
        agg->sum += *d;
    }
    
    virtual void finish(tuple_t &dest, const char* agg_data) {
        q6_agg_t* agg = aligned_cast<q6_agg_t>(agg_data);
        q6_agg_t* output = aligned_cast<q6_agg_t>(dest.data);
        output->count = agg->count;
        output->sum   = agg->sum;
    }

    virtual q6pipe_aggregate_t* clone() const {
        return new q6pipe_aggregate_t(*this);
    }

    virtual c_str to_string() const {
        return "q6pipe_aggregate_t";
    }
};



EXIT_NAMESPACE(q6pipe);


using namespace q6pipe;
using namespace qpipe;


ENTER_NAMESPACE(workload);


void tpch_q6pipe_driver::submit(void* disp, memObject_t*) {

    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
    // TSCAN
    struct cdb_table_s* tpch_lineitem = &tpch_tables[TPCH_TABLE_LINEITEM];
    tuple_fifo* tscan_output = new tuple_fifo(tpch_lineitem->tuple_size);
    tscan_packet_t *q6_tscan_packet =
        new tscan_packet_t("Q6PIPE_TSCAN_PACKET",
                           tscan_output,
                           new trivial_filter_t(tscan_output->tuple_size()),
                           tpch_lineitem->db);
    
    
    // ECHO (filter)
    tuple_fifo* echo_output = new tuple_fifo(sizeof(decimal[2]));
    echo_packet_t *q6_echo_packet =
        new echo_packet_t("Q6PIPE_ECHO_PACKET",
                          echo_output,
                          new q6_tscan_filter_t(),
                          q6_tscan_packet);
    
    
    // SIEVE (multiplication)
    tuple_fifo* sieve_output = new tuple_fifo(sizeof(decimal));
    sieve_packet_t *q6_sieve_packet =
        new sieve_packet_t("Q6PIPE_SIEVE_PACKET",
                           sieve_output,
                           new trivial_filter_t(sieve_output->tuple_size()),
                           new q6pipe_sieve_t(),
                           q6_echo_packet);
    
    
    // AGGREGATE
    tuple_fifo* agg_output = new tuple_fifo(sizeof(q6_agg_t));
    aggregate_packet_t* q6_agg_packet =
        new aggregate_packet_t(c_str("Q6PIPE_AGGREGATE_PACKET"),
                               agg_output,
                               new trivial_filter_t(agg_output->tuple_size()),
                               new q6pipe_aggregate_t(),
                               new q6pipe_aggregate_t::q6pipe_key_extractor_t(),
                               q6_sieve_packet);
    
    qpipe::query_state_t* qs = dp->query_state_create();
    q6_agg_packet->assign_query_state(qs);
    q6_sieve_packet->assign_query_state(qs);
    q6_echo_packet->assign_query_state(qs);
    q6_tscan_packet->assign_query_state(qs);

    q6_process_tuple_t pt;
    process_query(q6_agg_packet, pt);
    dp->query_state_destroy(qs);
}


EXIT_NAMESPACE(workload);
