/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_env.h
 *
 *  @brief Definition of the Shore TPC-C environment
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_ENV_H
#define __SHORE_TPCC_ENV_H

#include "sm_vas.h"
#include "util.h"

#include "core/trx_packet.h"

#include "stages/tpcc/common/tpcc_scaling_factor.h"
#include "stages/tpcc/common/tpcc_const.h"
#include "stages/tpcc/common/tpcc_input.h"
#include "stages/tpcc/common/tpcc_trx_input.h"

#include "sm/shore/shore_env.h"
#include "sm/shore/shore_sort_buf.h"
#include "sm/shore/shore_trx_worker.h"

#include "workload/tpcc/shore_tpcc_schema_man.h"


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

    int other;

    ShoreTPCCTrxCount& operator+=(ShoreTPCCTrxCount const& rhs) {
        new_order += rhs.new_order; 
        payment += rhs.payment; 
        order_status += rhs.order_status;
        delivery += rhs.delivery;
        stock_level += rhs.stock_level; 
        other += rhs.other;
	return (*this);
    }

    ShoreTPCCTrxCount& operator-=(ShoreTPCCTrxCount const& rhs) {
        new_order -= rhs.new_order; 
        payment -= rhs.payment; 
        order_status -= rhs.order_status;
        delivery -= rhs.delivery;
        stock_level -= rhs.stock_level; 
        other -= rhs.other;
	return (*this);
    }

    const int total() const {
        return (new_order+payment+order_status+delivery+stock_level+
                other);
    }

}; // EOF: ShoreTPCCTrxCount



struct ShoreTPCCTrxStats
{
    ShoreTPCCTrxCount attempted;
    ShoreTPCCTrxCount failed;

    ShoreTPCCTrxStats& operator+=(ShoreTPCCTrxStats const& other) {
        attempted += other.attempted;
        failed    += other.failed;
        return (*this);
    }

    ShoreTPCCTrxStats& operator-=(ShoreTPCCTrxStats const& other) {
        attempted -= other.attempted;
        failed    -= other.failed;
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

class ShoreTPCCEnv : public ShoreEnv
{
public:
    typedef trx_worker_t<ShoreTPCCEnv>    Worker;
    typedef trx_worker_t<ShoreTPCCEnv>*   WorkerPtr;
    typedef vector<WorkerPtr>    WorkerPool;
    typedef WorkerPool::iterator WorkerIt;

    typedef std::map<pthread_t, ShoreTPCCTrxStats*> statmap_t;

protected:       

    WorkerPool      _workers;            // list of worker threads
    int             _worker_cnt;         


    // TPC-C tables

    /** all the tables */
    guard<warehouse_t>         _pwarehouse_desc;
    guard<district_t>          _pdistrict_desc;
    guard<customer_t>          _pcustomer_desc;
    guard<history_t>           _phistory_desc;
    guard<new_order_t>         _pnew_order_desc;
    guard<order_t>             _porder_desc;
    guard<order_line_t>        _porder_line_desc;
    guard<item_t>              _pitem_desc;
    guard<stock_t>             _pstock_desc;

    tpcc_table_desc_list       _table_desc_list;


    /** all the table managers */
    guard<warehouse_man_impl>  _pwarehouse_man;
    guard<district_man_impl>   _pdistrict_man;
    guard<customer_man_impl>   _pcustomer_man;
    guard<history_man_impl>    _phistory_man;
    guard<new_order_man_impl>  _pnew_order_man;
    guard<order_man_impl>      _porder_man;
    guard<order_line_man_impl> _porder_line_man;
    guard<item_man_impl>       _pitem_man;
    guard<stock_man_impl>      _pstock_man;

    table_man_list_t           _table_man_list;

    // scaling factors
    int             _scaling_factor; /* scaling factor - SF=1 -> 100MB database */
    pthread_mutex_t _scaling_mutex;
    int             _queried_factor; /* queried factor */
    pthread_mutex_t _queried_mutex;


    // --- kit baseline trxs --- //
    w_rc_t xct_new_order(new_order_input_t* no_input, 
                         const int xct_id, 
                         trx_result_tuple_t& trt);
    w_rc_t xct_payment(payment_input_t* pay_input, 
                       const int xct_id, 
                       trx_result_tuple_t& trt);
    w_rc_t xct_order_status(order_status_input_t* status_input, 
                            const int xct_id, 
                            trx_result_tuple_t& trt);
    w_rc_t xct_delivery(delivery_input_t* deliv_input, 
                        const int xct_id, 
                        trx_result_tuple_t& trt);
    w_rc_t xct_stock_level(stock_level_input_t* level_input, 
                           const int xct_id, 
                           trx_result_tuple_t& trt);
    

private:
    w_rc_t _post_init_impl();
    
public:    

    /** Construction  */
    ShoreTPCCEnv(string confname)
        : ShoreEnv(confname), _worker_cnt(0),
          _scaling_factor(TPCC_SCALING_FACTOR), 
          _queried_factor(QUERIED_TPCC_SCALING_FACTOR)
    {
        // read the scaling factor from the configuration file
        pthread_mutex_init(&_scaling_mutex, NULL);
        pthread_mutex_init(&_queried_mutex, NULL);
    }


    virtual ~ShoreTPCCEnv() 
    {
        pthread_mutex_destroy(&_scaling_mutex);
        pthread_mutex_destroy(&_queried_mutex);
                
        _table_desc_list.clear();
        _table_man_list.clear();     
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
    

    // --- scaling and querying factor --- //
    void print_sf(void);
    void set_qf(const int aQF);
    inline int get_qf() { return (_queried_factor); }
    void set_sf(const int aSF);
    inline int get_sf() { return (_scaling_factor); }
    const int upd_sf();

    inline tpcc_table_desc_list* table_desc_list() { return (&_table_desc_list); }
    inline table_man_list_t*  table_man_list() { return (&_table_man_list); }
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


    // --- access to the tables --- //
    inline warehouse_t*  warehouse() { return (_pwarehouse_desc.get()); }
    inline district_t*   district()  { return (_pdistrict_desc.get()); }
    inline customer_t*   customer()  { return (_pcustomer_desc.get()); }
    inline history_t*    history()   { return (_phistory_desc.get()); }
    inline new_order_t*  new_order() { return (_pnew_order_desc.get()); }
    inline order_t*      order()     { return (_porder_desc.get()); }
    inline order_line_t* orderline() { return (_porder_line_desc.get()); }
    inline item_t*       item()      { return (_pitem_desc.get()); }
    inline stock_t*      stock()     { return (_pstock_desc.get()); }


    // --- access to the table managers --- //
    inline warehouse_man_impl*  warehouse_man() { return (_pwarehouse_man); }
    inline district_man_impl*   district_man()  { return (_pdistrict_man); }
    inline customer_man_impl*   customer_man()  { return (_pcustomer_man); }
    inline history_man_impl*    history_man()   { return (_phistory_man); }
    inline new_order_man_impl*  new_order_man() { return (_pnew_order_man); }
    inline order_man_impl*      order_man()     { return (_porder_man); }
    inline order_line_man_impl* orderline_man() { return (_porder_line_man); }
    inline item_man_impl*       item_man()      { return (_pitem_man); }
    inline stock_man_impl*      stock_man()     { return (_pstock_man); }
    





    // --- kit baseline trxs --- //

    w_rc_t run_one_xct(int xct_type, const int xctid, const int specID, trx_result_tuple_t& trt);


    // --- with input specified --- //
    w_rc_t run_new_order(const int xct_id, new_order_input_t& anoin, trx_result_tuple_t& atrt);
    w_rc_t run_payment(const int xct_id, payment_input_t& apin, trx_result_tuple_t& atrt);
    w_rc_t run_order_status(const int xct_id, order_status_input_t& aordstin, trx_result_tuple_t& atrt);
    w_rc_t run_delivery(const int xct_id, delivery_input_t& adelin, trx_result_tuple_t& atrt);
    w_rc_t run_stock_level(const int xct_id, stock_level_input_t& astoin, trx_result_tuple_t& atrt);

    // --- without input specified --- //
    w_rc_t run_new_order(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t run_payment(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t run_order_status(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t run_delivery(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t run_stock_level(const int xct_id, trx_result_tuple_t& atrt, int specificWH);

    // --- baseline mbench --- //
    w_rc_t run_mbench_cust(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t run_mbench_wh(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t _run_mbench_cust(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t _run_mbench_wh(const int xct_id, trx_result_tuple_t& atrt, int specificWH);



    

    // update statistics
    void _inc_other_att();
    void _inc_other_failed();
    void _inc_nord_att();
    void _inc_nord_failed();
    void _inc_pay_att();
    void _inc_pay_failed();
    void _inc_ordst_att();
    void _inc_ordst_failed();
    void _inc_deliv_att();
    void _inc_deliv_failed();
    void _inc_stock_att();
    void _inc_stock_failed();

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
    virtual void env_thread_init();
    virtual void env_thread_fini();   

    // stat map
    statmap_t _statmap;

    // snapshot taken at the beginning of each experiment    
    ShoreTPCCTrxStats _last_stats;
    virtual void reset_stats();
    const ShoreTPCCTrxStats _get_stats();
    
    
}; // EOF ShoreTPCCEnv
   


EXIT_NAMESPACE(tpcc);


#endif /* __SHORE_TPCC_ENV_H */
