/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcb_env.h
 *
 *  @brief Definition of the Shore TPC-C environment
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCB_ENV_H
#define __SHORE_TPCB_ENV_H

#include "sm_vas.h"
#include "util.h"

#include "core/trx_packet.h"

#include "workload/tpcb/tpcb_input.h"

#include "sm/shore/shore_env.h"
#include "sm/shore/shore_sort_buf.h"

#include "workload/tpcb/shore_tpcb_schema_man.h"
#include "workload/tpcb/shore_tpcb_worker.h"

#include <map>


using namespace qpipe;
using namespace shore;



ENTER_NAMESPACE(tpcb);


using std::map;


enum { XCT_ACCT_UPDATE, XCT_POPULATE_DB };
enum { TELLERS_PER_BRANCH=10 };
enum { ACCOUNTS_PER_BRANCH=100000 };
enum { ACCOUNTS_CREATED_PER_POP_XCT=10000 }; // must evenly divide ACCOUNTS_PER_BRANCH



#define DEFINE_TUPLE_INTERFACE(Type, Name)     \
    Type* get_##Name##_tuple();                \
    void  give_##Name##_tuple(Type* atuple);


#define DECLARE_TUPLE_INTERFACE(Type, Name, PoolName, TableObjectName)    \
    DECLARE_TLS(block_alloc<Type>, PoolName);                             \
    Type* ShoreTPCBEnv::get_##Name##_tuple() {                            \
        Type* tuple = new (*PoolName) Type;                               \
        assert (tuple); tuple->setup(TableObjectName);                    \
        return (tuple); }                                                 \
    void ShoreTPCBEnv::give_##Name##_tuple(Type* atuple) {                \
        PoolName->destroy(atuple); }





/******************************************************************** 
 * 
 *  ShoreTPCBEnv
 *  
 *  Shore TPC-B Database.
 *
 ********************************************************************/

class ShoreTPCBEnv : public ShoreEnv
{
public:
    typedef tpcb_worker_t        Worker;
    typedef tpcb_worker_t*       WorkerPtr;
    typedef vector<WorkerPtr>    WorkerPool;
    typedef WorkerPool::iterator WorkerIt;

    class table_builder_t;
    class table_creator_t;
protected:       

    WorkerPool      _workers;            // list of worker threads
    int             _worker_cnt;         


    // TPC-C tables

    /** all the tables */
    guard<branch_t>         _pbranch_desc;
    guard<teller_t>          _pteller_desc;
    guard<account_t>          _paccount_desc;
    guard<history_t>           _phistory_desc;

    tpcb_table_desc_list       _table_desc_list;


    /** all the table managers */
    guard<branch_man_impl>  _pbranch_man;
    guard<teller_man_impl>   _pteller_man;
    guard<account_man_impl>   _paccount_man;
    guard<history_man_impl>    _phistory_man;

    table_man_list_t           _table_man_list;

    // scaling factors
    int             _scaling_factor; /* scaling factor - SF=1 -> 100MB database */
    pthread_mutex_t _scaling_mutex;
    int             _queried_factor; /* queried factor - how many of the WHs queried */
    pthread_mutex_t _queried_mutex;

    // --- kit baseline trxs --- //
    w_rc_t xct_populate_db(populate_db_input_t*, int xct_id, trx_result_tuple_t &trt);
    w_rc_t xct_acct_update(acct_update_input_t*, int xct_id, trx_result_tuple_t& trt);
    

private:
    w_rc_t _post_init_impl();
    
public:    

    /** Construction  */
    ShoreTPCBEnv(string confname)
        : ShoreEnv(confname), _worker_cnt(0),
          _scaling_factor(100), 
          _queried_factor(100)
    {
        // read the scaling factor from the configuration file
        pthread_mutex_init(&_scaling_mutex, NULL);
        pthread_mutex_init(&_queried_mutex, NULL);
    }


    virtual ~ShoreTPCBEnv() 
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

    inline tpcb_table_desc_list* table_desc_list() { return (&_table_desc_list); }
    inline table_man_list_t*  table_man_list() { return (&_table_man_list); }
    const int dump();


    // Public methods //    

    // --- operations over tables --- //
    w_rc_t loaddata();  
    w_rc_t warmup();
    w_rc_t check_consistency();


    // --- access to the tables --- //
    inline branch_t*  branch() { return (_pbranch_desc.get()); }
    inline teller_t*   teller()  { return (_pteller_desc.get()); }
    inline account_t*   account()  { return (_paccount_desc.get()); }
    inline history_t*    history()   { return (_phistory_desc.get()); }


    // --- access to the table managers --- //
    inline branch_man_impl*  branch_man() { return (_pbranch_man); }
    inline teller_man_impl*   teller_man()  { return (_pteller_man); }
    inline account_man_impl*   account_man()  { return (_paccount_man); }
    inline history_man_impl*    history_man()   { return (_phistory_man); }




    // --- kit baseline trxs --- //

    w_rc_t run_one_xct(int xct_type, const int xctid, const int whid, trx_result_tuple_t& trt);


    // --- with input specified --- //
    w_rc_t run_populate_db(const int xct_id, populate_db_input_t& anoin, trx_result_tuple_t& atrt);
    w_rc_t run_acct_update(const int xct_id, acct_update_input_t& apin, trx_result_tuple_t& atrt);

    // --- without input specified --- //
    w_rc_t run_populate_db (const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t run_acct_update (const int xct_id, trx_result_tuple_t& atrt, int specificWH);



    const int upd_worker_cnt();

    // accesses a worker from the pool
    inline tpcb_worker_t* tpcbworker(const int idx) { 
        assert (idx>=0);
        return (_workers[idx%_worker_cnt]); 
    } 

    //// request atomic trash stack
    typedef atomic_class_stack<tpcb_request_t> RequestStack;
    RequestStack _request_pool;


}; // EOF ShoreTPCBEnv
   


EXIT_NAMESPACE(tpcb);


#endif /* __SHORE_TPCB_ENV_H */

