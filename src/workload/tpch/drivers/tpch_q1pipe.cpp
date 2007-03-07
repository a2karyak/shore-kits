/* -*- mode:C++; c-basic-offset:4 -*- */
#include "stages.h"
#include "scheduler.h"

#include "workload/tpch/drivers/tpch_q1pipe.h"
#include "workload/tpch/drivers/tpch_q1_types.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "workload/tpch/tpch_env.h"
#include "workload/common.h"

using q1::q1_tscan_filter_t;
using q1::projected_lineitem_tuple;
using q1::aggregate_tuple;
using q1::tpch_q1_process_tuple_t;


ENTER_NAMESPACE(q1pipe);


struct q1pipe_sieve_output_t {
    decimal L_QUANTITY;
    decimal L_EXTENDEDPRICE;
    decimal L_DISCOUNT;
    decimal L_DISC_PRICE;
    decimal L_CHARGE;
    char L_RETURNFLAG;
    char L_LINESTATUS;
};


/**
 * @brief This sieve receives a projected_lineitem_tuple (see q1
 * definition) and outputs a q1pipe_sieve_output_t.
 */
class q1pipe_sieve_t : public tuple_sieve_t {

public:

    q1pipe_sieve_t()
        : tuple_sieve_t(sizeof(q1pipe_sieve_output_t))
    {
    }
    
    virtual bool pass(tuple_t& dest, const tuple_t &src) {

        projected_lineitem_tuple* in =
            aligned_cast<projected_lineitem_tuple>(src.data);
        q1pipe_sieve_output_t* out =
            aligned_cast<q1pipe_sieve_output_t>(dest.data);

        decimal L_DISCOUNT = in->L_DISCOUNT;
        decimal L_EXTENDEDPRICE = in->L_EXTENDEDPRICE;
        decimal L_TAX = in->L_TAX;
        decimal L_DISC_PRICE = L_EXTENDEDPRICE * (1 - L_DISCOUNT);
        decimal L_CHARGE = L_DISC_PRICE * (1 + L_TAX);
        
        out->L_QUANTITY = in->L_QUANTITY;
        out->L_EXTENDEDPRICE = L_EXTENDEDPRICE;
        out->L_DISCOUNT = L_DISCOUNT;
        out->L_DISC_PRICE = L_DISC_PRICE;
        out->L_CHARGE = L_CHARGE;
        out->L_RETURNFLAG = in->L_RETURNFLAG;
        out->L_LINESTATUS = in->L_LINESTATUS;

        return true;
    }
    
    virtual tuple_sieve_t* clone() const {
        return new q1pipe_sieve_t(*this);
    }

    virtual c_str to_string() const {
        return "q1pipe_sieve_t";
    }
};



struct q1pipe_key_extract_t : public key_extractor_t {

    q1pipe_key_extract_t()
        : key_extractor_t(sizeof(char)*2, offsetof(q1pipe_sieve_output_t, L_RETURNFLAG))
    {
        assert(sizeof(char) == 1);
    }

    virtual int extract_hint(const char* tuple_data) const {

        q1pipe_sieve_output_t* tup =
            aligned_cast<q1pipe_sieve_output_t>(tuple_data);
        
        int result = (tup->L_RETURNFLAG << 8)
            + tup->L_LINESTATUS;

        return result;
    }

    virtual q1pipe_key_extract_t* clone() const {
        return new q1pipe_key_extract_t(*this);
    }
};



// aggregate
class q1pipe_count_aggregate_t : public tuple_aggregate_t {

private:
    q1pipe_key_extract_t _extractor;

public:
  
    q1pipe_count_aggregate_t()
        : tuple_aggregate_t(sizeof(aggregate_tuple))
    {
    }
    virtual key_extractor_t* key_extractor() { return &_extractor; }

    virtual void aggregate(char* agg_data, const tuple_t &s) {
        q1pipe_sieve_output_t* src =
            aligned_cast<q1pipe_sieve_output_t>(s.data);
        aggregate_tuple* tuple =
            aligned_cast<aggregate_tuple>(agg_data);

        // update count
        tuple->L_COUNT_ORDER++;
        
        // update sums
        decimal L_EXTENDEDPRICE = src->L_EXTENDEDPRICE;
        decimal L_QUANTITY = src->L_QUANTITY;
        tuple->L_SUM_QTY += L_QUANTITY;
        tuple->L_SUM_BASE_PRICE += L_EXTENDEDPRICE;
        tuple->L_SUM_DISC_PRICE += src->L_DISC_PRICE;
        tuple->L_SUM_CHARGE += src->L_CHARGE;
        tuple->L_AVG_QTY += L_QUANTITY;
        tuple->L_AVG_PRICE += L_EXTENDEDPRICE;
        tuple->L_AVG_DISC += src->L_DISCOUNT;
    }

    
    virtual void finish(tuple_t &d, const char* agg_data) {

        aggregate_tuple *dest  = aligned_cast<aggregate_tuple>(d.data);
        aggregate_tuple* tuple = aligned_cast<aggregate_tuple>(agg_data);

        *dest = *tuple;
        // compute averages
        dest->L_AVG_QTY /= dest->L_COUNT_ORDER;
        dest->L_AVG_PRICE /= dest->L_COUNT_ORDER;
        dest->L_AVG_DISC /= dest->L_COUNT_ORDER;
    }
    
    virtual q1pipe_count_aggregate_t* clone() const {
        return new q1pipe_count_aggregate_t(*this);
    }

    virtual c_str to_string() const {
        return "q1pipe_count_aggregate_t";
    }
};


EXIT_NAMESPACE(q1pipe);


using namespace q1pipe;
using namespace workload;
using namespace qpipe;


ENTER_NAMESPACE(workload);


void tpch_q1pipe_driver::submit(void* disp) {

    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
    // TSCAN PACKET
    struct cdb_table_s* tpch_lineitem = &tpch_tables[TPCH_TABLE_LINEITEM];
    tuple_fifo* tscan_output = new tuple_fifo(tpch_lineitem->tuple_size);
    tscan_packet_t* q1_tscan_packet =
        new tscan_packet_t("Q1PIPE_TSCAN_PACKET",
                           tscan_output,
                           new trivial_filter_t(tscan_output->tuple_size()),
                           tpch_lineitem->db);
    
    // ECHO (filter)
    tuple_fifo* echo_output = new tuple_fifo(sizeof(projected_lineitem_tuple));
    echo_packet_t* q1_echo_packet =
        new echo_packet_t("Q1PIPE_ECHO_PACKET",
                          echo_output,
                          new q1_tscan_filter_t(),
                          q1_tscan_packet);
    
    
    // SIEVE (multiplication)
    tuple_fifo* sieve_output = new tuple_fifo(sizeof(q1pipe_sieve_output_t));
    sieve_packet_t* q1_sieve_packet =
        new sieve_packet_t("Q1PIPE_SIEVE_PACKET",
                           sieve_output,
                           new trivial_filter_t(sieve_output->tuple_size()),
                           new q1pipe_sieve_t(),
                           q1_echo_packet);
    

    // AGG PACKET CREATION
    tuple_fifo* agg_output_buffer = new tuple_fifo(sizeof(aggregate_tuple));
    packet_t* q1_agg_packet =
        new partial_aggregate_packet_t("Q1PIPE_AGG_PACKET",
                                       agg_output_buffer,
                                       new trivial_filter_t(agg_output_buffer->tuple_size()),
                                       q1_echo_packet,
                                       new q1pipe_count_aggregate_t(),
                                       new q1pipe_key_extract_t(),
                                       new int_key_compare_t());

    qpipe::query_state_t* qs = dp->query_state_create();
    q1_agg_packet->assign_query_state(qs);
    q1_sieve_packet->assign_query_state(qs);
    q1_echo_packet->assign_query_state(qs);
    q1_tscan_packet->assign_query_state(qs);

    tpch_q1_process_tuple_t pt;
    process_query(q1_agg_packet, pt);
    dp->query_state_destroy(qs);
}


EXIT_NAMESPACE(workload);
