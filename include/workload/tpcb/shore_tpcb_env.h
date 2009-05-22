/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcb_env.h
 *
 *  @brief:  Definition of the Shore TPC-C environment
 *
 *  @author: Ryan Johnson, Feb 2009
 *  @author: Ippokratis Pandis, Feb 2009
 */

#ifndef __SHORE_TPCB_ENV_H
#define __SHORE_TPCB_ENV_H

#include "sm_vas.h"
#include "util.h"

#include "workload/tpcb/tpcb_input.h"

#include "sm/shore/shore_env.h"
#include "sm/shore/shore_trx_worker.h"

#include "workload/tpcb/shore_tpcb_schema_man.h"

#include <map>

using namespace shore;



ENTER_NAMESPACE(tpcb);


using std::map;




/******************************************************************** 
 * 
 *  ShoreTPCBEnv Stats
 *  
 *  Shore TPC-B Database transaction statistics
 *
 ********************************************************************/

struct ShoreTPCBTrxCount
{
    int acct_update;
    int populate_db;

    ShoreTPCBTrxCount& operator+=(ShoreTPCBTrxCount const& rhs) {
        acct_update += rhs.acct_update;        
	return (*this);
    }

    ShoreTPCBTrxCount& operator-=(ShoreTPCBTrxCount const& rhs) {
        acct_update -= rhs.acct_update;
	return (*this);
    }

    const int total() const {
        return (acct_update);
    }
    
}; // EOF: ShoreTPCBTrxCount


struct ShoreTPCBTrxStats
{
    ShoreTPCBTrxCount attempted;
    ShoreTPCBTrxCount failed;
    ShoreTPCBTrxCount deadlocked;

    ShoreTPCBTrxStats& operator+=(ShoreTPCBTrxStats const& other) {
        attempted  += other.attempted;
        failed     += other.failed;
        deadlocked += other.deadlocked;
        return (*this);
    }

    ShoreTPCBTrxStats& operator-=(ShoreTPCBTrxStats const& other) {
        attempted  -= other.attempted;
        failed     -= other.failed;
        deadlocked -= other.deadlocked;
        return (*this);
    }

}; // EOF: ShoreTPCBTrxStats



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
    typedef trx_worker_t<ShoreTPCBEnv> tpcb_worker_t;        
    typedef tpcb_worker_t              Worker;
    typedef tpcb_worker_t*             WorkerPtr;
    typedef vector<WorkerPtr>          WorkerPool;
    typedef WorkerPool::iterator       WorkerIt;

    typedef std::map<pthread_t, ShoreTPCBTrxStats*> statmap_t;

    class table_builder_t;
    class table_creator_t;

protected:       

    WorkerPool      _workers;            // list of worker threads
    int             _worker_cnt;         

    // scaling factors
    int             _scaling_factor; /* scaling factor - SF=1 -> 100MB database */
    pthread_mutex_t _scaling_mutex;
    int             _queried_factor; /* queried factor - how many of the WHs queried */
    pthread_mutex_t _queried_mutex;
    

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


    // TPCB Tables
    DECLARE_TABLE(branch_t,branch_man_impl,branch);
    DECLARE_TABLE(teller_t,teller_man_impl,teller);
    DECLARE_TABLE(account_t,account_man_impl,account);
    DECLARE_TABLE(history_t,history_man_impl,history);


    // --- kit baseline trxs --- //

    w_rc_t run_one_xct(const int xctid, int xct_type, const int whid, 
                       trx_result_tuple_t& trt);

    DECLARE_TRX(populate_db);
    DECLARE_TRX(acct_update);


    const int upd_worker_cnt();

    // accesses a worker from the pool
    inline tpcb_worker_t* tpcbworker(const int idx) { 
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

    // snapshot taken at the beginning of each experiment    
    ShoreTPCBTrxStats _last_stats;
    virtual void reset_stats();
    const ShoreTPCBTrxStats _get_stats();


}; // EOF ShoreTPCBEnv
   


EXIT_NAMESPACE(tpcb);


#endif /* __SHORE_TPCB_ENV_H */

