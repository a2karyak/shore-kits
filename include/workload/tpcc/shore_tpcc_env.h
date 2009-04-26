/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_env.h
 *
 *  @brief:  Definition of the Shore TPC-C environment
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_ENV_H
#define __SHORE_TPCC_ENV_H

#include "sm_vas.h"
#include "util.h"

#include "sm/shore/shore_env.h"
#include "sm/shore/shore_sort_buf.h"
#include "sm/shore/shore_trx_worker.h"

#include "core/trx_packet.h"

#include "stages/tpcc/common/tpcc_scaling_factor.h"
#include "stages/tpcc/common/tpcc_const.h"

#include "workload/tpcc/shore_tpcc_schema_man.h"
#include "workload/tpcc/tpcc_input.h"

#include <map>


using namespace qpipe;
using namespace shore;



ENTER_NAMESPACE(tpcc);


using std::map;



#define DEFINE_TUPLE_INTERFACE(Type, Name)     \
    Type* get_##Name##_tuple();                \
    void  give_##Name##_tuple(Type* atuple);


#define DECLARE_TUPLE_INTERFACE(Type, Name, PoolName, TableObjectName)    \
    DECLARE_TLS(block_alloc<Type>, PoolName);                             \
    Type* ShoreTPCCEnv::get_##Name##_tuple() {                            \
        Type* tuple = new (*PoolName) Type;                               \
        assert (tuple); tuple->setup(TableObjectName);                    \
        return (tuple); }                                                 \
    void ShoreTPCCEnv::give_##Name##_tuple(Type* atuple) {                \
        PoolName->destroy(atuple); }



/****************************************************************** 
 *
 *  @struct: ShoreTPCCEnv Stats
 *
 *  @brief:  TPCC Environment statistics
 *
 ******************************************************************/

struct ShoreTPCCTrxCount 
{
    int new_order;
    int payment;
    int order_status;
    int delivery;
    int stock_level;

    int mbench_wh;
    int mbench_cust;

    ShoreTPCCTrxCount& operator+=(ShoreTPCCTrxCount const& rhs) {
        new_order += rhs.new_order; 
        payment += rhs.payment; 
        order_status += rhs.order_status;
        delivery += rhs.delivery;
        stock_level += rhs.stock_level; 
        mbench_wh += rhs.mbench_wh;
        mbench_cust += rhs.mbench_cust;
	return (*this);
    }

    ShoreTPCCTrxCount& operator-=(ShoreTPCCTrxCount const& rhs) {
        new_order -= rhs.new_order; 
        payment -= rhs.payment; 
        order_status -= rhs.order_status;
        delivery -= rhs.delivery;
        stock_level -= rhs.stock_level; 
        mbench_wh -= rhs.mbench_wh;
        mbench_cust -= rhs.mbench_cust;
	return (*this);
    }

    const int total() const {
        return (new_order+payment+order_status+delivery+stock_level+
                mbench_wh+mbench_cust);
    }
    
}; // EOF: ShoreTPCCTrxCount



struct ShoreTPCCTrxStats
{
    ShoreTPCCTrxCount attempted;
    ShoreTPCCTrxCount failed;
    ShoreTPCCTrxCount deadlocked;

    ShoreTPCCTrxStats& operator+=(ShoreTPCCTrxStats const& other) {
        attempted  += other.attempted;
        failed     += other.failed;
        deadlocked += other.deadlocked;
        return (*this);
    }

    ShoreTPCCTrxStats& operator-=(ShoreTPCCTrxStats const& other) {
        attempted  -= other.attempted;
        failed     -= other.failed;
        deadlocked -= other.deadlocked;
        return (*this);
    }

}; // EOF: ShoreTPCCTrxStats



/******************************************************************** 
 * 
 *  ShoreTPCCEnv
 *  
 *  Shore TPC-C Database.
 *
 ********************************************************************/

// For P-Loader 
static int const NORD_PER_UNIT = 9;
static int const CUST_PER_UNIT = 30;
static int const HIST_PER_UNIT = 30;
static int const ORDERS_PER_UNIT = 30;
static int const STOCK_PER_UNIT = 100;
static int const UNIT_PER_WH = 1000;
static int const UNIT_PER_DIST = 100;
static int const ORDERS_PER_DIST = 3000;


class ShoreTPCCEnv : public ShoreEnv
{
public:
    typedef trx_worker_t<ShoreTPCCEnv>    Worker;
    typedef trx_worker_t<ShoreTPCCEnv>*   WorkerPtr;
    typedef vector<WorkerPtr>    WorkerPool;
    typedef WorkerPool::iterator WorkerIt;

    typedef std::map<pthread_t, ShoreTPCCTrxStats*> statmap_t;

    class table_builder_t;
    class table_creator_t;
    class checkpointer_t;

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
    ShoreTPCCEnv(string confname)
        : ShoreEnv(confname), _worker_cnt(0),
          _scaling_factor(TPCC_SCALING_FACTOR), 
          _queried_factor(QUERIED_TPCC_SCALING_FACTOR)
    {
        pthread_mutex_init(&_scaling_mutex, NULL);
        pthread_mutex_init(&_queried_mutex, NULL);
    }


    virtual ~ShoreTPCCEnv() 
    {
        pthread_mutex_destroy(&_scaling_mutex);
        pthread_mutex_destroy(&_queried_mutex);
    }


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
    virtual const int statistics();    
    

    // --- scaling and querying factor --- //
    void print_sf(void);
    void set_qf(const int aQF);
    inline int get_qf() { return (_queried_factor); }
    void set_sf(const int aSF);
    inline int get_sf() { return (_scaling_factor); }
    const int upd_sf();

    const int dump();

    virtual void print_throughput(const int iQueriedSF, 
                                  const int iSpread, 
                                  const int iNumOfThreads,
                                  const double delay);



    // Public methods //    

    // --- operations over tables --- //
    w_rc_t loaddata();  
    w_rc_t warmup();
    w_rc_t check_consistency();

    
    // TPCC Tables
    DECLARE_TABLE(warehouse_t,warehouse_man_impl,warehouse);
    DECLARE_TABLE(district_t,district_man_impl,district);
    DECLARE_TABLE(customer_t,customer_man_impl,customer);
    DECLARE_TABLE(history_t,history_man_impl,history);
    DECLARE_TABLE(new_order_t,new_order_man_impl,new_order);
    DECLARE_TABLE(order_t,order_man_impl,order);
    DECLARE_TABLE(order_line_t,order_line_man_impl,order_line);
    DECLARE_TABLE(item_t,item_man_impl,item);
    DECLARE_TABLE(stock_t,stock_man_impl,stock);
    

    // --- kit trxs --- //

    w_rc_t run_one_xct(const int xctid, int xct_type, const int specID, 
                       trx_result_tuple_t& trt);

    DECLARE_TRX(new_order);
    DECLARE_TRX(payment);
    DECLARE_TRX(order_status);
    DECLARE_TRX(delivery);
    DECLARE_TRX(stock_level);

    DECLARE_TRX(mbench_wh);
    DECLARE_TRX(mbench_cust);

    // P-Loader
    DECLARE_TRX(populate_baseline);
    DECLARE_TRX(populate_one_unit);
    

    const int upd_worker_cnt();

    // accesses a worker from the pool
    inline WorkerPtr trxworker(const int idx) { 
        assert (idx>=0);
        return (_workers[idx%_worker_cnt]); 
    } 

    //// request atomic trash stack
    typedef atomic_class_stack<trx_request_t> RequestStack;
    RequestStack _request_pool;

    // for thread-local stats
    virtual void env_thread_init(base_worker_t* aworker);
    virtual void env_thread_fini(base_worker_t* aworker);   

    // stat map
    statmap_t _statmap;

#ifdef ACCESS_RECORD_TRACE
    void add_rat(string event);
#endif

    // snapshot taken at the beginning of each experiment    
    ShoreTPCCTrxStats _last_stats;
    virtual void reset_stats();
    const ShoreTPCCTrxStats _get_stats();
    
    
}; // EOF ShoreTPCCEnv
   


EXIT_NAMESPACE(tpcc);


#endif /* __SHORE_TPCC_ENV_H */
