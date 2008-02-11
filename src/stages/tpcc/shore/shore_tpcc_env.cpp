/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_env.cpp
 *
 *  @brief Declaration of the Shore TPC-C environment (database)
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/shore_tpcc_env.h"


using namespace shore;

ENTER_NAMESPACE(tpcc);


/** Exported variables */

ShoreTPCCEnv* shore_env;


/** Exported functions */


/********
 ******** Caution: The functions below should be invoked inside
 ********          the context of a smthread
 ********/


/****************************************************************** 
 *
 * @fn:    loaddata()
 *
 * @brief: Loads the data for all the TPCC tables, given the current
 *         scaling factor value. During the loading the SF cannot be
 *         changed.
 *
 ******************************************************************/

w_rc_t ShoreTPCCEnv::loaddata() 
{
    /* 0. lock the scaling factor */
    critical_section_t cs(_scaling_mutex);

    /* 1. create the loader threads */
    tpcc_table_t* ptable = NULL;
    int num_tbl = _table_list.size();
    int cnt = 0;
    guard<tpcc_loading_smt_t> loaders[SHORE_TPCC_TABLES];
    time_t tstart = time(NULL);

    for(tpcc_table_list_iter table_iter = _table_list.begin(); 
        table_iter != _table_list.end(); table_iter++)
        {
            ptable = *table_iter;

            char fname[MAX_FILENAME_LEN];
            strcpy(fname, SHORE_TPCC_DATA_DIR);
            strcat(fname, "/");
            strcat(fname, ptable->name());
            strcat(fname, ".dat");
            cout << ++cnt << ". Loading " << ptable->name() << endl;
            time_t ttablestart = time(NULL);
            w_rc_t e = ptable->load_from_file(_pssm, fname);
            time_t ttablestop = time(NULL);
            if (e != RCOK)
                cerr << cnt << ". Error while loading " << ptable->name() << " *****" << endl;            
            else
                cout << cnt << ". Done loading " << ptable->name() << endl;

           cout << "Table " << ptable->name() << " loaded in " << (ttablestop - ttablestart) << " secs..." << endl;

//            loaders[cnt] = new tpcc_loading_smt_t(_pssm, ptable, 
//                                                  _scaling_factor, cnt++);

        }

#if 0
#if 1
    /* 3. fork the loading threads */
    cnt = 0;
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->fork();
    }

    /* 4. join the loading threads */
    cnt = 0;
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->join();        
        if (loaders[i]->rv()) {
            cerr << "~~~~ Error at loader " << i << endl;
            assert (false);
        }
    }    
#else
    /* 3. fork & join the loading threads SERIALLY */
    cnt = 0;
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->fork();
	loaders[i]->join();        
        if (loaders[i]->rv()) {
            cerr << "~~~~ Error at loader " << i << endl;
            assert (false);
        }        
    }
#endif
#endif
    time_t tstop = time(NULL);

    /* 5. print stats */
    cout << "Loading finished in " << (tstop - tstart) << " secs..." << endl;
    cout << num_tbl << " tables loaded..." << endl;        

    return (RCOK);
}



/****************************************************************** 
 *
 * @fn:    check_consistency()
 *
 * @brief: Iterates over all tables and checks consistency between
 *         the values stored in the base table (file) and the 
 *         corresponding indexes.
 *
 ******************************************************************/

w_rc_t ShoreTPCCEnv::check_consistency()
{
    /* 1. create the checker threads */
    tpcc_table_t* ptable = NULL;
    int num_tbl = _table_list.size();
    int cnt = 0;
    guard<tpcc_checking_smt_t> checkers[SHORE_TPCC_TABLES];

    for(tpcc_table_list_iter table_iter = _table_list.begin(); 
        table_iter != _table_list.end(); table_iter++)
        {
            ptable = *table_iter;
            checkers[cnt] = new tpcc_checking_smt_t(_pssm, ptable, cnt++);
        }

#if 0
    /* 2. fork the threads */
    cnt = 0;
    time_t tstart = time(NULL);
    for(int i=0; i<num_tbl; i++) {
	checkers[i]->fork();
    }

    /* 3. join the threads */
    cnt = 0;
    for(int i=0; i < num_tbl; i++) {
	checkers[i]->join();
    }    
    time_t tstop = time(NULL);
#else
    /* 2. fork & join the threads SERIALLY */
    cnt = 0;
    time_t tstart = time(NULL);
    for(int i=0; i<num_tbl; i++) {
	checkers[i]->fork();
	checkers[i]->join();
    }
    time_t tstop = time(NULL);
#endif

    /* 4. print stats */
    cout << "Checking finished in " << (tstop - tstart) << " secs..." << endl;
    cout << num_tbl << " tables checked..." << endl; 
    return (RCOK);
}



void ShoreTPCCEnv::set_qf(const int aQF)
{
    if ((aQF >= 0) && (aQF <= _scaling_factor)) {
        critical_section_t cs(_queried_mutex);
        cout << "New Queried factor: " << aQF << endl;
        _queried_factor = aQF;
    }
    else {
        cerr << "Invalid queried factor input: " << aQF << endl;
    }
}

w_rc_t ShoreTPCCEnv::xct_new_order(new_order_input_t* no_input, const int xct_id) { return (RCOK); }
w_rc_t ShoreTPCCEnv::xct_payment(payment_input_t * pay_input, const int xct_id) { return (RCOK); }
w_rc_t ShoreTPCCEnv::xct_order_status(order_status_input_t* status_input, const int xct_id) { return (RCOK); }
w_rc_t ShoreTPCCEnv::xct_delivery(delivery_input_t* deliv_input, const int xct_id) { return (RCOK); }
w_rc_t ShoreTPCCEnv::xct_stock_level(stock_level_input_t* level_input, const int xct_id) { return (RCOK); }



/****************************************************************** 
 *
 * class tpcc_loading_smt_t
 *
 ******************************************************************/

void tpcc_loading_smt_t::run()
{
    char fname[MAX_FILENAME_LEN];
    strcpy(fname, SHORE_TPCC_DATA_DIR);
    strcat(fname, "/");
    strcat(fname, _ptable->name());
    strcat(fname, ".dat");
    cout << _cnt << ". Loading " << _ptable->name() << endl;
    time_t ttablestart = time(NULL);
    w_rc_t e = _ptable->load_from_file(_pssm, fname);
    time_t ttablestop = time(NULL);
    if (e != RCOK) {
        cerr << _cnt << ". Error while loading " << _ptable->name() << " *****" << endl;
        _rv = 1;
    }
    else
        cout << _cnt << ". Done loading " << _ptable->name() << endl;

    cout << "Table " << _ptable->name() << " loaded in " << (ttablestop - ttablestart) << " secs..." << endl;
    _rv = 0;
}



/****************************************************************** 
 *
 * class tpcc_checking_smt_t
 *
 ******************************************************************/

void tpcc_checking_smt_t::run()
{
    cout << _cnt << ". Checking " << _ptable->name() << endl;
    if (!_ptable->check_all_indexes(_pssm))
        cerr << _cnt << ". Inconsistency in " << _ptable->name() << endl;
    else
        cout << _cnt << ". " << _ptable->name() << " OK..." << endl;
}


EXIT_NAMESPACE(tpcc);
