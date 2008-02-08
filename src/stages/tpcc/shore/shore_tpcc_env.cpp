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

    /* 2. fire up the loading threads */

    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // DONE SERIALLY NOW
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    register tpcc_table_t* ptable = NULL;
    for(tpcc_table_list_iter table_iter = _table_list.begin(); 
        table_iter != _table_list.end(); table_iter++)
        {
            ptable = *table_iter;
            cout << "Loading " << ptable->name() << endl;
            W_DO(ptable->bulkload(_pssm, _scaling_factor));
            cout << "Done loading " << ptable->name() << endl;
        }

    /* 3. free the common cnt_array */
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
    register tpcc_table_t* ptable = NULL;
    for(tpcc_table_list_iter table_iter = _table_list.begin(); 
        table_iter != _table_list.end(); table_iter++)
        {
            ptable = *table_iter;
            cout << "Checking " << ptable->name() << endl;
            if (!ptable->check_all_indexes(_pssm))
                cerr << "Inconsistency in " << ptable->name() << endl;
            else
                cout << ptable->name() << " OK..." << endl;
        }

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

w_rc_t xct_new_order(new_order_input_t* no_input, const int xct_id) { return (RCOK); }
w_rc_t xct_payment(payment_input_t * pay_input, const int xct_id) { return (RCOK); }
w_rc_t xct_order_status(order_status_input_t* status_input, const int xct_id) { return (RCOK); }
w_rc_t xct_delivery(delivery_input_t* deliv_input, const int xct_id) { return (RCOK); }
w_rc_t xct_stock_level(stock_level_input_t* level_input, const int xct_id) { return (RCOK); }



EXIT_NAMESPACE(tpcc);
