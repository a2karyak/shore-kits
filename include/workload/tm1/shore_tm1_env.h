/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tm1_env.h
 *
 *  @brief:  Definition of the Shore TM1 environment
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */

#ifndef __SHORE_TM1_ENV_H
#define __SHORE_TM1_ENV_H

#include "sm_vas.h"
#include "util.h"

#include "core/trx_packet.h"

#include "workload/tm1/tm1_const.h"
#include "workload/tm1/tm1_input.h"

#include "sm/shore/shore_env.h"
#include "sm/shore/shore_sort_buf.h"
#include "sm/shore/shore_trx_worker.h"

#include "workload/tm1/shore_tm1_schema_man.h"

#include <map>


using namespace qpipe;
using namespace shore;



ENTER_NAMESPACE(tm1);


using std::map;


#define DECLARE_TRX(trx) \
    w_rc_t run_##trx(const int xct_id, trx_result_tuple_t& atrt, trx##_input_t& in);       \
    w_rc_t run_##trx(const int xct_id, trx_result_tuple_t& atrt, const int specificID);    \
    w_rc_t xct_##trx(const int xct_id, trx_result_tuple_t& atrt, trx##_input_t& in);       \
    void   _inc_##trx##_att();  \
    void   _inc_##trx##_failed();

#define DECLARE_TABLE(table,manimpl,abbrv)       \
    guard<table>     _p##abbrv##_desc;           \
    guard<manimpl>   _p##abbrv##_man;            \
    inline manimpl*  ##abbrv##_man() { return (_p##abbrv##_man); }


#define DEFINE_RUN_WITH_INPUT_TRX_WRAPPER(cname,trx)                   \
    w_rc_t cname::run_##trx(const int xct_id, trx_result_tuple_t& atrt, trx##_input_t& in) { \
        TRACE( TRACE_TRX_FLOW, "%d. %s ...\n", xct_id, #trx);         \
        ++my_stats.attempted.##trx;                                   \
        w_rc_t e = xct_##trx(xct_id, atrt, in);                       \
        if (e.is_error()) {                                           \
            ++my_stats.failed.##trx;                                  \
            TRACE( TRACE_TRX_FLOW, "Xct (%d) %s aborted [0x%x]\n", xct_id, #trx, e.err_num()); \
            w_rc_t e2 = _pssm->abort_xct();                           \
            if(e2.is_error()) TRACE( TRACE_ALWAYS, "Xct (%d) %s abort failed [0x%x]\n", xct_id, #trx, e2.err_num()); \
            if (atrt.get_notify()) atrt.get_notify()->signal();       \
            if (_measure!=MST_MEASURE) return (e);                    \
            _env_stats.inc_trx_att();                                 \
            return (e); }                                             \
        TRACE( TRACE_TRX_FLOW, "Xct (%d) %s completed\n", xct_id, #trx); \
        if (atrt.get_notify()) atrt.get_notify()->signal();           \
        if (_measure!=MST_MEASURE) return (RCOK);                     \
        _env_stats.inc_trx_com();                                     \
        return (RCOK); }

#define DEFINE_RUN_WITHOUT_INPUT_TRX_WRAPPER(cname,trx)               \
    w_rc_t cname::run_##trx(const int xct_id, trx_result_tuple_t& atrt, const int specificID) { \
        trx##_input_t in = create_##trx##_input(_scaling_factor, specificID);                   \
        return (run_##trx(xct_id, atrt, in)); }


#define DEFINE_TRX_STATS(cname,trx)                                   \
    void cname::_inc_##trx##_att()    { ++my_stats.attempted.##trx; } \
    void cname::_inc_##trx##_failed() { ++my_stats.failed.##trx; }


#define DEFINE_TRX(cname,trx) \
    DEFINE_RUN_WITHOUT_INPUT_TRX_WRAPPER(cname,trx); \
    DEFINE_RUN_WITH_INPUT_TRX_WRAPPER(cname,trx);    \
    DEFINE_TRX_STATS(cname,trx);    





/******************************************************************** 
 * 
 *  ShoreTM1Env Stats
 *  
 *  Shore TM1 Database transaction statistics
 *
 ********************************************************************/

struct ShoreTM1TrxCount
{
    int get_sub_data;
    int get_new_dest;
    int get_acc_data;
    int upd_sub_data;
    int upd_loc;
    int ins_call_fwd;
    int del_call_fwd;

    int other;

    ShoreTM1TrxCount& operator+=(ShoreTM1TrxCount const& rhs) {
        get_sub_data += rhs.get_sub_data;
        get_new_dest += rhs.get_new_dest;
        get_acc_data += rhs.get_acc_data;
        upd_sub_data += rhs.upd_sub_data;
        upd_loc += rhs.upd_loc;
        ins_call_fwd += rhs.ins_call_fwd;
        del_call_fwd += rhs.del_call_fwd;
        other += rhs.other;
	return (*this);
    }

    ShoreTM1TrxCount& operator-=(ShoreTM1TrxCount const& rhs) {
        get_sub_data -= rhs.get_sub_data;
        get_new_dest -= rhs.get_new_dest;
        get_acc_data -= rhs.get_acc_data;
        upd_sub_data -= rhs.upd_sub_data;
        upd_loc -= rhs.upd_loc;
        ins_call_fwd -= rhs.ins_call_fwd;
        del_call_fwd -= rhs.del_call_fwd;
        other -= rhs.other;
	return (*this);
    }

    const int total() const {
        return (get_sub_data+get_new_dest+get_acc_data+
                upd_sub_data+upd_loc+ins_call_fwd+del_call_fwd+
                other);
    }
    
}; // EOF: ShoreTM1TrxCount


struct ShoreTM1TrxStats
{
    ShoreTM1TrxCount attempted;
    ShoreTM1TrxCount failed;

    ShoreTM1TrxStats& operator+=(ShoreTM1TrxStats const& other) {
        attempted += other.attempted;
        failed    += other.failed;
        return (*this);
    }

    ShoreTM1TrxStats& operator-=(ShoreTM1TrxStats const& other) {
        attempted -= other.attempted;
        failed    -= other.failed;
        return (*this);
    }

}; // EOF: ShoreTM1TrxStats




/******************************************************************** 
 * 
 *  ShoreTM1Env
 *  
 *  Shore TM1 Database
 *
 ********************************************************************/

class ShoreTM1Env : public ShoreEnv
{
public:
    typedef trx_worker_t<ShoreTM1Env>   tm1_worker_t;
    typedef tm1_worker_t                Worker;
    typedef tm1_worker_t*               WorkerPtr;
    typedef vector<WorkerPtr>           WorkerPool;
    typedef WorkerPool::iterator        WorkerIt;

    typedef std::map<pthread_t, ShoreTM1TrxStats*> statmap_t;

    class table_builder_t;
    class table_creator_t;

protected:       

    WorkerPool      _workers;            // list of worker threads
    int             _worker_cnt;         

    // scaling factors
    int             _scaling_factor; /* scaling factor - SF=1 -> 100MB database */
    pthread_mutex_t _scaling_mutex;
    int             _queried_factor; /* queried factor */
    pthread_mutex_t _queried_mutex;

private:
    w_rc_t _post_init_impl();
    
public:    

    /** Construction  */
    ShoreTM1Env(string confname)
        : ShoreEnv(confname), _worker_cnt(0),
          _scaling_factor(TM1_DEF_SF),
          _queried_factor(TM1_DEF_QF)
    { }

    virtual ~ShoreTM1Env() { }


    // DB INTERFACE

    virtual const int set(envVarMap* vars) { return(0); /* do nothing */ };
    virtual const int open() { return(0); /* do nothing */ };
    virtual const int pause() { return(0); /* do nothing */ };
    virtual const int resume() { return(0); /* do nothing */ };    
    virtual const int newrun() { return(0); /* do nothing */ };

    virtual const int post_init();
    virtual const int load_schema();

    virtual const int conf();
    virtual const int start();
    virtual const int stop();
    virtual const int info();
    

    virtual w_rc_t warmup() { return(RCOK); /* do nothing */ };
    virtual w_rc_t check_consistency() { return(RCOK); /* do nothing */ };


    // --- scaling and querying factor --- //
    void set_qf(const int aQF);
    inline int get_qf() { return (_queried_factor); }
    void set_sf(const int aSF);
    inline int get_sf() { return (_scaling_factor); }
    const int upd_sf();

    virtual void print_throughput(const int iQueriedSF, 
                                  const int iSpread, 
                                  const int iNumOfThreads,
                                  const double delay);

    // Public methods //    

    // --- operations over tables --- //
    virtual w_rc_t loaddata();  
    w_rc_t xct_populate_one(const int sub_id);

    // TM1 tables
    DECLARE_TABLE(subscriber_t,sub_man_impl,sub)
    DECLARE_TABLE(access_info_t,ai_man_impl,ai)
    DECLARE_TABLE(special_facility_t,sf_man_impl,sf)
    DECLARE_TABLE(call_forwarding_t,cf_man_impl,cf)


    // --- kit baseline trxs --- //

    w_rc_t run_one_xct(int xct_type, const int xctid, const int specificID,
                       trx_result_tuple_t& trt);

    DECLARE_TRX(get_sub_data)
    DECLARE_TRX(get_new_dest)
    DECLARE_TRX(get_acc_data)
    DECLARE_TRX(upd_sub_data)
    DECLARE_TRX(upd_loc)
    DECLARE_TRX(ins_call_fwd)
    DECLARE_TRX(del_call_fwd)


    const int upd_worker_cnt();

    // accesses a worker from the pool
    inline tm1_worker_t* tm1worker(const int idx) { 
        assert (idx>=0);
        return (_workers[idx%_worker_cnt]); 
    } 

    //// request atomic trash stack
    typedef atomic_class_stack<trx_request_t> RequestStack;
    RequestStack _request_pool;
    

    // for thread-local stats
    virtual void env_thread_init();
    virtual void env_thread_fini();   

    // stat map
    statmap_t _statmap;


    // snapshot taken at the beginning of each experiment    
    ShoreTM1TrxStats _last_stats;
    virtual void reset_stats();
    const ShoreTM1TrxStats _get_stats();

}; // EOF ShoreTM1Env
   


EXIT_NAMESPACE(tm1);


#endif /* __SHORE_TM1_ENV_H */

