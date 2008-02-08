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

    /* 1. initially produce the cnt_array for order_t and order_line_t */
    _order.produce_cnt_array(_scaling_factor);
    _order_line.set_cnt_array(_order.get_cnt_array());
    assert(_order.get_cnt_array() == _order_line.get_cnt_array());

    /* 2. create the loader threads */
    tpcc_table_t* ptable = NULL;
    int num_tbl = _table_list.size();
    int cnt = 0;
    guard<tpcc_loading_smt_t> loaders[SHORE_TPCC_TABLES];
    int ACTUAL_TBLS = 0;

    for(tpcc_table_list_iter table_iter = _table_list.begin(); 
        table_iter != _table_list.end(); table_iter++)
        {
            ptable = *table_iter;
            loaders[cnt] = new tpcc_loading_smt_t(_pssm, ptable, 
                                                  _scaling_factor, cnt++);

//@@@@@@@@@@@@@@@@@@@@@@@@
// remove the cnt>1 condition
//@@@@@@@@@@@@@@@@@@@@@@@@

            if (cnt > ACTUAL_TBLS)
                break;
        }

#if 1
    /* 3. fork the loading threads */
    cnt = 0;
    time_t tstart = time(NULL);
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->fork();
        if (++cnt > ACTUAL_TBLS)
            break;
    }

    /* 4. join the loading threads */
    cnt = 0;
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->join();        
        if (loaders[i]->rv()) {
            cerr << "~~~~ Error at loader " << i << endl;
            assert (false);
        }
        if (++cnt > ACTUAL_TBLS)
            break;
    }    
    time_t tstop = time(NULL);
#else
    /* 3. fork & join the loading threads SERIALLY */
    cnt = 0;
    time_t tstart = time(NULL);
    for(int i=0; i<num_tbl; i++) {
	loaders[i]->fork();
	loaders[i]->join();        
        if (loaders[i]->rv()) {
            cerr << "~~~~ Error at loader " << i << endl;
            assert (false);
        }        
        if (++cnt > ACTUAL_TBLS)
            break;
    }
    time_t tstop = time(NULL);
#endif

    /* 5. print stats */
    cout << "Loading finished in " << (tstop - tstart) << " secs..." << endl;
    cout << num_tbl << " tables loaded..." << endl;        

    /* 6. free the common cnt_array */
    _order.free_cnt_array();

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
    int ACTUAL_TBLS = 0;
    guard<tpcc_checking_smt_t> checkers[SHORE_TPCC_TABLES];

    for(tpcc_table_list_iter table_iter = _table_list.begin(); 
        table_iter != _table_list.end(); table_iter++)
        {
            ptable = *table_iter;
            checkers[cnt] = new tpcc_checking_smt_t(_pssm, ptable, cnt++);


//@@@@@@@@@@@@@@@@@@@@@@@@
// remove the cnt>1 condition
//@@@@@@@@@@@@@@@@@@@@@@@@

            if (++cnt > ACTUAL_TBLS)
                break;
        }

#if 0
    /* 2. fork the threads */
    cnt = 0;
    time_t tstart = time(NULL);
    for(int i=0; i<num_tbl; i++) {
	checkers[i]->fork();
        if (++cnt > ACTUAL_TBLS)
            break;
    }

    /* 3. join the threads */
    cnt = 0;
    for(int i=0; i < num_tbl; i++) {
	checkers[i]->join();
        if (++cnt > ACTUAL_TBLS)
            break;
    }    
    time_t tstop = time(NULL);
#else
    /* 2. fork & join the threads SERIALLY */
    cnt = 0;
    time_t tstart = time(NULL);
    for(int i=0; i<num_tbl; i++) {
	checkers[i]->fork();
	checkers[i]->join();
        if (++cnt > ACTUAL_TBLS)
            break;
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
    cout << _cnt << ". Loading " << _ptable->name() << endl;
    w_rc_t e = _ptable->bulkload(_pssm, _sf);
    if (e != RCOK) {
        cerr << _cnt << " error while loading " << _ptable->name();
        _rv = 1;
    }

    cout << _cnt << ". Done loading " << _ptable->name() << endl;
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
