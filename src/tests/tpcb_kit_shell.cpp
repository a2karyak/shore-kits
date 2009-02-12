/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_kit_shell.cpp
 *
 *  @brief:  Test shore/dora tpc-c kit with shell
 *
 *  @author: Ippokratis Pandis, Sept 2008
 *
 */

#include "tests/common.h"
#include "util/shell.h"

#include "workload/tpcb/shore_tpcb_env.h"

#include "tests/common/tester_shore_shell.h"
#include "tests/common/tester_shore_client.h"

using namespace shore;
using namespace tpcb;





//////////////////////////////

template<class Client,class DB>
class kit_t : public shore_shell_t
{ 
private:
    DB* _tpcbdb;

public:

    kit_t(const char* prompt) 
        : shore_shell_t(prompt)
    {
        // load supported trxs and binding policies maps
        load_trxs_map();
        load_bp_map();
    }
    ~kit_t() { }


    virtual const int inst_test_env(int argc, char* argv[]);
    virtual const int load_trxs_map(void);
    virtual const int load_bp_map(void);


    // impl of supported commands
    virtual int _cmd_TEST_impl(const int iQueriedBranches, const int iSpread,
                               const int iNumOfThreads, const int iNumOfTrxs,
                               const int iSelectedTrx, const int iIterations,
                               const eBindingType abt);
    virtual int _cmd_MEASURE_impl(const int iQueriedBranches, const int iSpread,
                                  const int iNumOfThreads, const int iDuration,
                                  const int iSelectedTrx, const int iIterations,
                                  const eBindingType abt);    

    virtual int process_cmd_LOAD(const char* command, const char* command_tag);        



    // cmd - RESTART
    struct restart_cmd_t : public command_handler_t {
        DB* _pdb;
        restart_cmd_t(DB* adb) : _pdb(adb) { };
        ~restart_cmd_t() { }
        void setaliases() { 
            _name = string("restart"); _aliases.push_back("restart"); }
        const int handle(const char* cmd) { 
            assert (_pdb); _pdb->stop(); _pdb->start(); 
            return (SHELL_NEXT_CONTINUE); }
        const string desc() { return (string("Restart")); }               
    };
    guard<restart_cmd_t> _restarter;

    // cmd - INFO
    struct info_cmd_t : public command_handler_t {
        DB* _pdb;
        info_cmd_t(DB* adb) : _pdb(adb) { };
        ~info_cmd_t() { }
        void setaliases() { 
            _name = string("info"); _aliases.push_back("info"); _aliases.push_back("i"); }
        const int handle(const char* cmd) { 
            assert (_pdb); _pdb->info(); return (SHELL_NEXT_CONTINUE); }
        const string desc() { return (string("Print info about the state of db instance")); }
    };
    guard<info_cmd_t> _informer;

    // cmd - STATS
    struct stats_cmd_t : public command_handler_t {
        DB* _pdb;
        stats_cmd_t(DB* adb) : _pdb(adb) { };
        ~stats_cmd_t() { }
        void setaliases() { 
            _name = string("stats"); _aliases.push_back("stats"); _aliases.push_back("st"); }
        const int handle(const char* cmd) { 
            assert (_pdb); _pdb->statistics(); return (SHELL_NEXT_CONTINUE); }
        const string desc() { return (string("Print statistics")); }
    };
    guard<stats_cmd_t> _stater;

    // cmd - DUMP
    struct dump_cmd_t : public command_handler_t {
        DB* _pdb;
        dump_cmd_t(DB* adb) : _pdb(adb) { };
        ~dump_cmd_t() { }
        void setaliases() { 
            _name = string("dump"); _aliases.push_back("dump"); _aliases.push_back("d"); }
        const int handle(const char* cmd) { 
            assert (_pdb); _pdb->dump(); return (SHELL_NEXT_CONTINUE); }
        const string desc() { return (string("Dump db instance data")); }
    };
    guard<dump_cmd_t> _dumper;

    virtual const int register_commands() 
    {
        // FROM TESTER_SHORE_SHELL
        _fakeioer = new fake_iodelay_cmd_t(_tpcbdb);
        _fakeioer->setaliases();
        add_cmd(_fakeioer.get());

        // FROM TESTER_SHORE_SHELL
        _slier = new sli_enable_cmd_t(_tpcbdb);
        _slier->setaliases();
        add_cmd(_slier.get());

        _elrer = new elr_enable_cmd_t(_tpcbdb);
        _elrer->setaliases();
        add_cmd(_elrer.get());


        // TEMPLATE-BASED
        _restarter = new restart_cmd_t(_tpcbdb);
        _restarter->setaliases();
        add_cmd(_restarter.get());

        _informer = new info_cmd_t(_tpcbdb);
        _informer->setaliases();
        add_cmd(_informer.get());

        _stater = new stats_cmd_t(_tpcbdb);
        _stater->setaliases();
        add_cmd(_stater.get());

        _dumper = new dump_cmd_t(_tpcbdb);
        _dumper->setaliases();
        add_cmd(_dumper.get());

        return (0);
    }

}; // EOF: kit_t



/********************************************************************* 
 *
 *  @fn:      load_trxs_map
 *
 *  @brief:   Instanciates a client and calls the function that loads
 *            the map of supported trxs.
 *
 *  @returns: The number of supported trxs
 *
 *********************************************************************/

template<class Client,class DB>
const int kit_t<Client,DB>::load_trxs_map(void)
{
    CRITICAL_SECTION(shell_cs,_lock);
    // gets the supported trxs from the client
    return (Client::load_sup_xct(_sup_trxs));
}

template<class Client,class DB>
const int kit_t<Client,DB>::load_bp_map(void)
{
    CRITICAL_SECTION(shell_cs,_lock);
    // Basic binding policies
    _sup_bps[BT_NONE]          = "NoBinding";
    _sup_bps[BT_NEXT]          = "Adjacent";
    _sup_bps[BT_SPREAD]        = "SpreadToCores";
    return (_sup_bps.size());
}


/********************************************************************* 
 *
 *  @fn:      inst_test_env
 *
 *  @brief:   Instanciates the Shore environment, 
 *            Opens the database and sets the appropriate number of Branches
 *  
 *  @returns: 1 on error
 *
 *********************************************************************/

template<class Client,class DB>
const int kit_t<Client,DB>::inst_test_env(int argc, char* argv[]) 
{    
    // 1. Instanciate the Shore Environment
    _env = _tpcbdb = new DB(SHORE_CONF_FILE);

    // 2. Initialize the Shore Environment
    // the initialization must be executed in a shore context
    db_init_smt_t* initializer = new db_init_smt_t(c_str("init"), _env);
    assert (initializer);
    initializer->fork();
    initializer->join(); 
    int rv = initializer->rv();
    delete (initializer);
    initializer = NULL;

    if (rv) {
        TRACE( TRACE_ALWAYS, "Exiting...\n");
        return (rv);
    }

    // 3. set SF - if param valid
    assert (_tpcbdb);
    if (argc>1) {
        int numQueriedOfBranches = atoi(argv[1]);
        _tpcbdb->set_qf(numQueriedOfBranches);
    }
    _numOfWHs = _tpcbdb->get_sf();


    // 4. Load supported trxs and bps
    if (!load_trxs_map()) {
        TRACE( TRACE_ALWAYS, "No supported trxs...\nExiting...");
        return (1);
    }
    if (!load_bp_map()) {
        TRACE( TRACE_ALWAYS, "No supported binding policies...\nExiting...");
        return (1);
    }

    // 5. Now that everything is set, register any additional commands
    register_commands();

    // 6. Start the VAS
    return (_tpcbdb->start());
}



/******************************************************************** 
 *
 * @enum:  baseline_tpcb_client_t
 *
 * @brief: The Baseline TPC-C kit smthread-based test client class
 *
 ********************************************************************/

class baseline_tpcb_client_t : public base_client_t 
{
private:
    // workload parameters
    ShoreTPCBEnv* _tpcbdb;    
    int _wh;
    tpcb_worker_t* _worker;
    int _qf;
    

public:

    baseline_tpcb_client_t() { }     

    baseline_tpcb_client_t(c_str tname, const int id, ShoreTPCBEnv* env, 
                           const MeasurementType aType, const int trxid, 
                           const int numOfTrxs, 
                           processorid_t aprsid, const int sWH, const int qf) 
	: base_client_t(tname,id,env,aType,trxid,numOfTrxs,aprsid),
          _tpcbdb(env), _wh(sWH), _qf(qf)
    {
        assert (env);
        assert (_wh>=-1 && _qf>0);

        // pick worker thread
        _worker = _tpcbdb->tpcbworker(_id);
        TRACE( TRACE_DEBUG, "Picked worker (%s)\n", _worker->thread_name().data());
        assert (_worker);
    }

    ~baseline_tpcb_client_t() { }

    // INTERFACE 

    w_rc_t run_one_xct(int xct_type, int xctid);    

    static int load_sup_xct(mapSupTrxs& stmap)
    {
	// clears the supported trx map and loads its own
	stmap.clear();

	// Baseline TPC-C trxs
	stmap[XCT_ACCT_UPDATE]          = "AccountUpdate";
	stmap[XCT_POPULATE_DB]    = "PopulateDB";

	return (stmap.size());
    }
}; // EOF: baseline_tpcb_client_t

w_rc_t baseline_tpcb_client_t::run_one_xct(int xct_type, int xctid) 
{
    // 1. Initiate and detach from transaction
    tid_t atid = tid_t::null;
    xct_t* pxct = 0;
    
    // 2. Set input
    trx_result_tuple_t atrt;
    W_DO(_tpcbdb->db()->begin_xct(atid));
    return _tpcbdb->run_one_xct(xct_type, xctid, _wh, atrt);
}



namespace tpcb {

struct xct_stats {
    int attempted;
    int deadlocked;
    int failed;
    xct_stats &operator+=(xct_stats const &other) {
	attempted += other.attempted;
	failed += other.failed;
	deadlocked += other.deadlocked;
	return *this;
    };
    xct_stats &operator-=(xct_stats const &other) {
	attempted -= other.attempted;
	failed -= other.failed;
	deadlocked -= other.deadlocked;
	return *this;
    };
};


extern xct_stats shell_get_xct_stats();
}

namespace shore {
    extern void shell_expect_clients(int count);
    extern void shell_await_clients();

}

void print_xct_stats(const xct_stats& stats) 
{
}






/********************************************************************* 
 *
 * COMMANDS
 *
 *********************************************************************/

/** cmd: TEST **/

template<class Client,class DB>
int kit_t<Client,DB>::_cmd_TEST_impl(const int iQueriedBranches, 
                                     const int iSpread,
                                     const int iNumOfThreads, 
                                     const int iNumOfTrxs,
                                     const int iSelectedTrx, 
                                     const int iIterations,
                                     const eBindingType abt)
{
    // print test information
    print_TEST_info(iQueriedBranches, iSpread, iNumOfThreads, 
                    iNumOfTrxs, iSelectedTrx, iIterations, abt);

    _tpcbdb->upd_sf();

    Client* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iIterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iIterations);

        // reset starting cpu and wh id
        _current_prs_id = _start_prs_id;
        int wh_id = 0;

        xct_stats statsb = shell_get_xct_stats();

        // set measurement state to measure - start counting everything
        _env->set_measure(MST_MEASURE);
	stopwatch_t timer;

        // 1. create and fork client clients
        for (int i=0; i<iNumOfThreads; i++) {
            // create & fork testing threads
            if (iSpread)
                wh_id = i+1;
            testers[i] = new Client(c_str("CL-%d",i), i, _tpcbdb, 
                                    MT_NUM_OF_TRXS, iSelectedTrx, iNumOfTrxs,
                                    _current_prs_id, wh_id, iQueriedBranches);
            assert (testers[i]);
            testers[i]->fork();
            _current_prs_id = next_cpu(abt, _current_prs_id);
        }

        // 2. join the tester threads
        for (int i=0; i<iNumOfThreads; i++) {
            testers[i]->join();
            if (testers[i]->rv()) {
                TRACE( TRACE_ALWAYS, "Error in testing...\n");
                TRACE( TRACE_ALWAYS, "Exiting...\n");
                assert (false);
            }    
            delete (testers[i]);
        }

	double delay = timer.time();

        xct_stats stats = shell_get_xct_stats();
        stats -= statsb;
	
        printf("Branche:\t%d\n"
               "Spread:\t%s\n"
               "SLI:\t%s\n"
               "Threads:\t%d\n"
               "Trx Att:\t%d\n"
               "Trx Abt:\t%d\n"
               "Secs:\t%.2lf\n"
               "TPS:\t%.2lf\n",
               iQueriedBranches, iSpread? "yes" : "no", iNumOfThreads,
               stats.attempted, stats.failed,
               delay, (stats.attempted-stats.failed)/delay);


        // flush the log before the next iteration
        TRACE( TRACE_ALWAYS, "db checkpoint - start\n");
        //_env->checkpoint();
        TRACE( TRACE_ALWAYS, "db checkpoint - end\n");

    }

    // print processor usage info
    //    _cpustater->myinfo.print();

    // set measurement state
    _env->set_measure(MST_DONE);
    return (SHELL_NEXT_CONTINUE);
}


/** cmd: MEASURE **/

template<class Client,class DB>
int kit_t<Client,DB>::_cmd_MEASURE_impl(const int iQueriedBranches, 
                                        const int iSpread,
                                        const int iNumOfThreads, 
                                        const int iDuration,
                                        const int iSelectedTrx, 
                                        const int iIterations,
                                        const eBindingType abt)
{
    // print measurement info
    print_MEASURE_info(iQueriedBranches, iSpread, iNumOfThreads, iDuration, 
                       iSelectedTrx, iIterations, abt);

    _tpcbdb->upd_sf();

    // create and fork client threads
    Client* testers[MAX_NUM_OF_THR];
        // reset starting cpu and wh id
        _current_prs_id = _start_prs_id;
        int wh_id = -1;

        // set measurement state
        _env->set_measure(MST_WARMUP);
	shell_expect_clients(iNumOfThreads);

        // 1. create and fork client threads
        for (int i=0; i<iNumOfThreads; i++) {
            // create & fork testing threads
            if (iSpread)
                wh_id = i;
            testers[i] = new Client(c_str("%s-%d", _cmd_prompt,i), i, _tpcbdb, 
                                    MT_TIME_DUR, iSelectedTrx, 0,
                                    _current_prs_id, wh_id, iQueriedBranches);
            assert (testers[i]);
            testers[i]->fork();
            _current_prs_id = next_cpu(abt, _current_prs_id);
        }

        // give them some time (2secs) to start-up
	shell_await_clients();

    
    for (int j=0; j<iIterations; j++) {

        sleep(1);
        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iIterations);

        // set measurement state
	TRACE(TRACE_ALWAYS, "begin measurement\n");
        _env->set_measure(MST_MEASURE);
	xct_stats statsb = shell_get_xct_stats();
	//        alarm(iDuration);
	stopwatch_t timer;
	//	_cpustater->myinfo.reset();
	sleep(iDuration);
	xct_stats stats = shell_get_xct_stats();
	stats -= statsb;
	
	double delay = timer.time();
	//_cpustater->myinfo.print();
	TRACE(TRACE_ALWAYS, "end measurement\n");

	printf("Branches:\t%d\n"
	       "Spread:\t%s\n"
	       "Threads:\t%d\n"
	       "Trx Att:\t%d\n"
	       "Trx DLK:\t%d\n"
	       "Trx FAIL:\t%d\n"
	       "Secs:\t%.2lf\n"
	       "TPS:\t%.2lf\n",
	       iQueriedBranches, iSpread? "yes" : "no", iNumOfThreads,
	       stats.attempted, stats.deadlocked, stats.failed, 
	       delay, (stats.attempted-stats.failed-stats.deadlocked)/delay);
	       
    }

        // 2. join the tester threads
	_env->set_measure(MST_DONE);
        for (int i=0; i<iNumOfThreads; i++) {
            testers[i]->join();
	    //fprintf(stderr,".");
            if (testers[i]->rv()) {
                TRACE( TRACE_ALWAYS, "Error in testing...\n");
                assert (false);
            }    
            assert (testers[i]);
            delete (testers[i]);
        }

	//	alarm(0); // cancel the alarm, if any
        
        // flush the log before the next iteration
        TRACE( TRACE_ALWAYS, "db checkpoint - start\n");
	stopwatch_t timer;
        _env->checkpoint();
        TRACE( TRACE_ALWAYS, "db checkpoint - end (took %d seconds)\n", int(timer.time()));

    // print processor usage info
    //    _cpustater->myinfo.print();

    // set measurement state
    _env->set_measure(MST_DONE);
    return (SHELL_NEXT_CONTINUE);
}

template<class Client,class DB>
int kit_t<Client,DB>::process_cmd_LOAD(const char* command, 
                                       const char* command_tag)
{
    TRACE( TRACE_DEBUG, "Gotcha\n");
    return (SHELL_NEXT_CONTINUE);
}


//////////////////////////////

typedef kit_t<baseline_tpcb_client_t,ShoreTPCBEnv> baselineTPCBKit;


//////////////////////////////


int main(int argc, char* argv[]) 
{
    // initialize cordoba threads
    thread_init();

    TRACE_SET( TRACE_ALWAYS | TRACE_STATISTICS 
               //               | TRACE_NETWORK 
               //               | TRACE_CPU_BINDING
               //              | TRACE_QUERY_RESULTS
               //              | TRACE_PACKET_FLOW
               //               | TRACE_RECORD_FLOW
               //               | TRACE_TRX_FLOW
               //| TRACE_DEBUG
               );

    // 1. Get env vars
    envVar* ev = envVar::instance();       

    // 2. Initialize shell
    guard<shore_shell_t> kit = NULL;
    string sysname = ev->getSysname();

    TRACE( TRACE_ALWAYS, "Starting (%s) kit\n", sysname.c_str());

    kit = new baselineTPCBKit("(tpcb) ");
    assert (kit.get());


    // 2. Instanciate and start the Shore environment
    if (kit->inst_test_env(argc, argv))
        return (2);

    // 3. Make sure that the correct schema is used
    assert (kit->db());
    if (kit->db()->sysname().compare(sysname)) {
        TRACE( TRACE_ALWAYS, 
               "Incorrect schema at configuration file" \
               " Wanted (%s) - Found (%s)" \
               "\nExiting...\n", kit->db()->sysname().c_str(), sysname.c_str());
        return (3);
    }

    // 4. Make sure data is loaded
    w_rc_t rcl = kit->db()->loaddata();
    if (rcl.is_error()) {
        return (4);
    }

    // set the global variable to the kit's db - for alarm() to work
    _g_shore_env = kit->db();

    // 5. Start processing commands
    kit->start();

    // 6. the Shore environment will close at the destructor of the kit
    return (0);
}

