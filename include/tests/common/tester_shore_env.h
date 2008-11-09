/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_shore_env.h
 *
 *  @brief:  Defines common tester functions and variables related to the 
 *           Shore environment.
 *
 *  @author: Ippokratis Pandis, July 2008
 *
 */

#ifndef __TESTER_SHORE_ENV_H
#define __TESTER_SHORE_ENV_H


// for binding LWP to cores
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>


#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"


using namespace shore;
using namespace tpcc;


// enumuration of different binding types
enum eBindingType { BT_NONE=0, BT_NEXT=1, BT_SPREAD=2 };


/** Default values for the environment **/

// default database size (scaling factor)
const int DF_NUM_OF_WHS         = 10;
extern int _numOfWHs;



/** Default values for the power-runs **/

// default queried number of warehouses (queried factor)
const int DF_NUM_OF_QUERIED_WHS    = 10;

// default value to spread threads at WHs
const int DF_SPREAD_THREADS_TO_WHS = 1;

// default number of threads
const int DF_NUM_OF_THR            = 5;

// maximum number of threads
const int MAX_NUM_OF_THR           = 100;

// default number of transactions executed per thread
const int DF_TRX_PER_THR           = 100;

// default transaction id to be executed
const int DF_TRX_ID                = XCT_PAYMENT;

// default duration for time-based measurements (in secs)
const int DF_DURATION              = 20;

// default number of iterations
const int DF_NUM_OF_ITERS          = 5;

// default use sli
const int DF_USE_SLI               = 0;

// default processor binding
const eBindingType DF_BINDING_TYPE = BT_NONE;


/** Default values for the warmups **/

// default number of queried WHs during warmup
const int DF_WARMUP_QUERIED_WHS = 10;

// default number of transactions executed per thread during warmup
const int DF_WARMUP_TRX_PER_THR = 1000;

// default duration of warmup (in secs)
const int DF_WARMUP_DURATION    = 20;

// default number of iterations during warmup
const int DF_WARMUP_ITERS       = 3;



// default batch size
const int BATCH_SIZE = 10;


// Instanciate and close the Shore environment
int inst_test_env(int argc, char* argv[]);
int close_test_env();

// Helper
void print_wh_usage(char* argv[]);




/******************************************************************** 
 *
 * @enum  MeasurementType
 *
 * @brief Possible measurement types for tester thread
 *
 ********************************************************************/

const int DF_WARMUP_INTERVAL = 2; // 2 secs

enum MeasurementType { MT_UNDEF, MT_NUM_OF_TRXS, MT_TIME_DUR };


/******************************************************************** 
 *
 * @enum:  client_smt_t
 *
 * @brief: An smthread-based class for the test clients
 *
 ********************************************************************/

class client_smt_t : public thread_t 
{
private:

    ShoreTPCCEnv* _env;    

    // workload parameters
    MeasurementType _measure_type;
    int _wh;
    int _trxid;
    int _notrxs;

    int _use_sli;

    // used for submitting batches
    condex_pair* _cp;

    // for processor binding
    bool          _is_bound;
    processorid_t _prs_id;

public:
    int	_rv;

    static void abort_test();
    static void resume_test();
    
    client_smt_t(ShoreTPCCEnv* env, 
               MeasurementType aType,
               int sWH, int trxId, int numOfTrxs, int useSLI,
               c_str tname,
               processorid_t aprsid = PBIND_NONE) 
	: thread_t(tname), 
          _env(env), _measure_type(aType),
          _wh(sWH), _trxid(trxId), 
          _notrxs(numOfTrxs), 
          _use_sli(useSLI),
          _is_bound(false), _prs_id(aprsid),
          _rv(0)
    {
        assert (_env);
        assert (_measure_type != MT_UNDEF);
        assert (_wh>=0);
        assert (_notrxs || (_measure_type == MT_TIME_DUR));

        _cp = new condex_pair();
        assert (_cp);
    }


    ~client_smt_t() { 
        if (_cp) {
            delete (_cp);
            _cp = NULL;
        }
    }
        

    void submit_batch(ShoreTPCCEnv* env, int xct_type, 
                      int& trx_cnt, const int batch_size = BATCH_SIZE);

    w_rc_t run_xcts(ShoreTPCCEnv* env, int xct_type, int num_xct);
    w_rc_t run_one_tpcc_xct(ShoreTPCCEnv* env, int xct_type, int xctid);    

    // regular trx impl
    w_rc_t xct_new_order(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_payment(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_order_status(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_delivery(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_stock_level(ShoreTPCCEnv* env, int xctid);

    w_rc_t xct_mbench_wh(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_mbench_cust(ShoreTPCCEnv* env, int xctid);


    // dora trx impl
    w_rc_t xct_dora_new_order(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_dora_payment(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_dora_order_status(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_dora_delivery(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_dora_stock_level(ShoreTPCCEnv* env, int xctid);

    w_rc_t xct_dora_mbench_wh(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_dora_mbench_cust(ShoreTPCCEnv* env, int xctid);


    // thread entrance
    void work() {

        // 1. bind to the specified processor
        if (processor_bind(P_LWPID, P_MYID, _prs_id, NULL)) {
            TRACE( TRACE_ALWAYS, "Cannot bind to processor (%d)\n", _prs_id);
            _is_bound = false;
        }
        else {
            TRACE( TRACE_DEBUG, "Binded to processor (%d)\n", _prs_id);
            _is_bound = true;
        }

        // 2. init env in not initialized
        if (!_env->is_initialized()) {
            if (_env->init()) {
                // Couldn't initialize the Shore environment
                // cannot proceed
                TRACE( TRACE_ALWAYS, "Couldn't initialize Shore...\n");
                _rv = 1;
                return;
            }
        }

        // 3. set SLI option
        ss_m::set_sli_enabled(_use_sli);

        // 4. run test
        _rv = test();
    }

    // methods
    int test() {
        w_rc_t rc = _env->loaddata();
        if(rc.is_error()) return rc.err_num();
        return (run_xcts(_env, _trxid, _notrxs).err_num());
    }

    
    const bool is_bound() const { return (_is_bound); }

    void print_tables();

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }

}; // EOF: client_smt_t



#endif /** __TESTER_SHORE_ENV_H */
