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

#include "sm/shore/shore_env.h"

#include "stages/tpcc/shore/shore_tpcc_schema_man.h"

#include <map>


using namespace qpipe;
using namespace shore;



ENTER_NAMESPACE(tpcc);


using std::map;


/******** Exported variables ********/

class ShoreTPCCEnv;
extern ShoreTPCCEnv* _g_shore_env;

//typedef atomic_trash_stack<char*> ts_buf_t;



/****************************************************************** 
 *
 *  @struct: tpcc_stats_t
 *
 *  @brief:  TPCC Environment statistics
 *
 ******************************************************************/

struct tpcc_stats_t 
{
    int        _no_att;
    int        _no_com;
    tatas_lock _no_lock;
    int        _pay_att;
    int        _pay_com;
    tatas_lock _pay_lock;
    int        _ord_att;
    int        _ord_com;
    tatas_lock _ord_lock;
    int        _del_att;
    int        _del_com;
    tatas_lock _del_lock;
    int        _sto_att;
    int        _sto_com;
    tatas_lock _sto_lock;

    tpcc_stats_t() 
        : _no_att(0), _no_com(0), _pay_att(0), _pay_com(0), _ord_att(0),
          _ord_com(0), _del_att(0), _del_com(0), _sto_att(0), _sto_com(0)
    {
    }

    ~tpcc_stats_t()
    {
        print_trx_stats();
    }

    // Reset counters
    void reset() {
        // grabs all the locks and resets
        CRITICAL_SECTION(res_no_cs, _no_lock);
        CRITICAL_SECTION(res_pay_cs, _pay_lock);
        CRITICAL_SECTION(res_ord_cs, _ord_lock);
        CRITICAL_SECTION(res_del_cs, _del_lock);
        CRITICAL_SECTION(res_sto_cs, _sto_lock);
        
        _no_att = 0;
        _no_com = 0;
        _pay_att = 0;
        _pay_com = 0;
        _ord_att = 0;
        _ord_com = 0;
        _del_att = 0;
        _del_com = 0;
        _sto_att = 0;
        _sto_com = 0;
    }



    // Prints stats
    void print_trx_stats();

    // Access methods
    int inc_no_att() { 
        CRITICAL_SECTION(att_no_cs, _no_lock);
        return (++_no_att); 
    }

    int inc_no_com() { 
        CRITICAL_SECTION(com_no_cs, _no_lock);
        ++_no_att;
        return (++_no_com); 
    }

    int get_no_com() {
        CRITICAL_SECTION(read_no_cs, _no_lock);
        return (_no_com);
    }

    int inc_pay_att() { 
        CRITICAL_SECTION(att_pay_cs, _pay_lock);
        return (++_pay_att); 
    }

    int inc_pay_com() { 
        CRITICAL_SECTION(com_pay_cs, _pay_lock);
        ++_pay_att;
        return (++_pay_com); 
    }

    int inc_ord_att() { 
        CRITICAL_SECTION(att_ord_cs, _ord_lock);
        return (++_ord_att); 
    }

    int inc_ord_com() { 
        CRITICAL_SECTION(com_ord_cs, _ord_lock);
        ++_ord_att;
        return (++_ord_com); 
    }

    int inc_del_att() { 
        CRITICAL_SECTION(att_del_cs, _del_lock);
        return (++_del_att); 
    }

    int inc_del_com() { 
        CRITICAL_SECTION(com_del_cs, _del_lock);
        ++_del_att;
        return (++_del_com); 
    }

    int inc_sto_att() { 
        CRITICAL_SECTION(att_sto_cs, _sto_lock);
        return (++_sto_att); 
    }

    int inc_sto_com() { 
        CRITICAL_SECTION(com_sto_cs, _sto_lock);
        ++_sto_att;
        return (++_sto_com); 
    }

}; // EOF tpcc_stats_t


/******************************************************************** 
 * 
 *  ShoreTPCCEnv
 *  
 *  Shore TPC-C Database.
 *
 ********************************************************************/

class ShoreTPCCEnv : public ShoreEnv
{
private:       
    // TPC-C tables

    /** all the tables */
    warehouse_t          _warehouse_desc;
    district_t           _district_desc;
    customer_t           _customer_desc;
    history_t            _history_desc;
    new_order_t          _new_order_desc;
    order_t              _order_desc;
    order_line_t         _order_line_desc;
    item_t               _item_desc;
    stock_t              _stock_desc;

    tpcc_table_list_t    _table_desc_list;


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

    /** scaling factors */
    int             _scaling_factor; /* scaling factor - SF=1 -> 100MB database */
    pthread_mutex_t _scaling_mutex;
    int             _queried_factor; /* queried factor - how many of the WHs queried */
    pthread_mutex_t _queried_mutex;

    /** some stats */
    tpcc_stats_t   _tpcc_stats;     // the stats for the whole env life (never reset)
    tpcc_stats_t   _tmp_tpcc_stats; // temp stats (reset between runs)


    /* --- kit baseline trxs --- */
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

    
public:    

    /** Construction  */
    ShoreTPCCEnv(string confname, 
                 int aSF = TPCC_SCALING_FACTOR, 
                 int aQF = QUERIED_TPCC_SCALING_FACTOR) 
        : ShoreEnv(confname), _scaling_factor(aSF), _queried_factor(aQF)
    {
        assert (aSF > 0);
        assert (aQF > 0);
        assert (aSF >= aQF);

        pthread_mutex_init(&_scaling_mutex, NULL);
        pthread_mutex_init(&_queried_mutex, NULL);

        // initiate the table managers
        _pwarehouse_man  = new warehouse_man_impl(&_warehouse_desc);
        _pdistrict_man   = new district_man_impl(&_district_desc);
        _pstock_man      = new stock_man_impl(&_stock_desc);
        _porder_line_man = new order_line_man_impl(&_order_line_desc);
        _pcustomer_man   = new customer_man_impl(&_customer_desc);
        _phistory_man    = new history_man_impl(&_history_desc);
        _porder_man      = new order_man_impl(&_order_desc);
        _pnew_order_man  = new new_order_man_impl(&_new_order_desc);
        _pitem_man       = new item_man_impl(&_item_desc);

        // XXX: !!! Warning !!!
        //
        // The two tables should have the description and the manager
        // of the same table in the same position
        //

        /* add the table managers to a list */
        // (ip) Adding them in descending file order, so that the large
        //      files to be loaded at the begining. Expection is the
        //      WH and DISTR which are always the first two.
        _table_man_list.push_back(_pwarehouse_man);
        _table_man_list.push_back(_pdistrict_man);
        _table_man_list.push_back(_pstock_man);
        _table_man_list.push_back(_porder_line_man);
        _table_man_list.push_back(_pcustomer_man);
        _table_man_list.push_back(_phistory_man);
        _table_man_list.push_back(_porder_man);
        _table_man_list.push_back(_pnew_order_man);
        _table_man_list.push_back(_pitem_man);

        assert (_table_man_list.size() == SHORE_TPCC_TABLES);

        
        /* add the table descriptions to a list */
        _table_desc_list.push_back(&_warehouse_desc);
        _table_desc_list.push_back(&_district_desc);
        _table_desc_list.push_back(&_stock_desc);
        _table_desc_list.push_back(&_order_line_desc);
        _table_desc_list.push_back(&_customer_desc);
        _table_desc_list.push_back(&_history_desc);
        _table_desc_list.push_back(&_order_desc);
        _table_desc_list.push_back(&_new_order_desc);
        _table_desc_list.push_back(&_item_desc);

        assert (_table_desc_list.size() == SHORE_TPCC_TABLES);
    }


    ~ShoreTPCCEnv() 
    {
        pthread_mutex_destroy(&_scaling_mutex);
        pthread_mutex_destroy(&_queried_mutex);
                
        _table_desc_list.clear();
        _table_man_list.clear();     
    }

    virtual int post_init();

private:
    w_rc_t _post_init_impl();

public:


    /* --- statistics --- */
    void print_tpcc_stats() { 
        _tpcc_stats.print_trx_stats(); 
        _env_stats.print_env_stats(); 
    }

    tpcc_stats_t* get_tpcc_stats() {
        return (&_tpcc_stats);
    }

    void print_temp_tpcc_stats() {
        _tmp_tpcc_stats.print_trx_stats();
    }

    tpcc_stats_t* get_tmp_tpcc_stats() {
        return (&_tmp_tpcc_stats);
    }

    void reset_tmp_tpcc_stats() {
        _tmp_tpcc_stats.reset();
    }


    /* --- scaling and querying factor --- */
    void print_sf();
    void set_qf(const int aQF);
    inline int get_qf() { return (_queried_factor); }
    void set_sf(const int aSF);
    inline int get_sf() { return (_scaling_factor); }


    inline tpcc_table_list_t* table_desc_list() { return (&_table_desc_list); }
    inline table_man_list_t*  table_man_list() { return (&_table_man_list); }
    void dump();


    /** Public methods */    

    /* --- operations over tables --- */
    w_rc_t loaddata();  
    w_rc_t check_consistency();


    /* --- access to the tables --- */
    warehouse_t*  warehouse() { return (&_warehouse_desc); }
    district_t*   district()  { return (&_district_desc); }
    customer_t*   customer()  { return (&_customer_desc); }
    history_t*    history()   { return (&_history_desc); }
    new_order_t*  new_order() { return (&_new_order_desc); }
    order_t*      order()     { return (&_order_desc); }
    order_line_t* orderline() { return (&_order_line_desc); }
    item_t*       item()      { return (&_item_desc); }
    stock_t*      stock()     { return (&_stock_desc); }

    /* --- access to the table managers --- */
    warehouse_man_impl*  warehouse_man() { return (_pwarehouse_man); }
    district_man_impl*   district_man()  { return (_pdistrict_man); }
    customer_man_impl*   customer_man()  { return (_pcustomer_man); }
    history_man_impl*    history_man()   { return (_phistory_man); }
    new_order_man_impl*  new_order_man() { return (_pnew_order_man); }
    order_man_impl*      order_man()     { return (_porder_man); }
    order_line_man_impl* orderline_man() { return (_porder_line_man); }
    item_man_impl*       item_man()      { return (_pitem_man); }
    stock_man_impl*      stock_man()     { return (_pstock_man); }


    /* --- kit baseline trxs --- */

    /* --- without input specified --- */
    w_rc_t run_new_order(const int xct_id, 
                         trx_result_tuple_t& atrt,
                         int specificWH);
    w_rc_t run_payment(const int xct_id, 
                       trx_result_tuple_t& atrt,
                       int specificWH);
    w_rc_t run_order_status(const int xct_id, 
                            trx_result_tuple_t& atrt,
                            int specificWH);
    w_rc_t run_delivery(const int xct_id, 
                        trx_result_tuple_t& atrt,
                        int specificWH);
    w_rc_t run_stock_level(const int xct_id, 
                           trx_result_tuple_t& atrt,
                           int specificWH);

    /* --- with input specified --- */
    w_rc_t run_new_order(const int xct_id, new_order_input_t& anoin,
                         trx_result_tuple_t& atrt);
    w_rc_t run_payment(const int xct_id, payment_input_t& apin,
                       trx_result_tuple_t& atrt);
    w_rc_t run_order_status(const int xct_id, order_status_input_t& aordstin,
                            trx_result_tuple_t& atrt);
    w_rc_t run_delivery(const int xct_id, delivery_input_t& adelin,
                        trx_result_tuple_t& atrt);
    w_rc_t run_stock_level(const int xct_id, stock_level_input_t& astoin,
                           trx_result_tuple_t& atrt);



    /* --- kit staged trxs --- */
    w_rc_t staged_pay_updateShoreWarehouse(payment_input_t* ppin, 
                                           const int xct_id, 
                                           trx_result_tuple_t& trt);
    w_rc_t staged_pay_updateShoreDistrict(payment_input_t* ppin, 
                                          const int xct_id, 
                                          trx_result_tuple_t& trt);
    w_rc_t staged_pay_updateShoreCustomer(payment_input_t* ppin, 
                                          const int xct_id, 
                                          trx_result_tuple_t& trt);
    w_rc_t staged_pay_insertShoreHistory(payment_input_t* ppin, 
                                         char* p_wh_name,
                                         char* p_d_name,
                                         const int xct_id, 
                                         trx_result_tuple_t& trt);

    w_rc_t staged_no_outside_loop(new_order_input_t* pnoin, 
                                  time_t tstamp,
                                  const int xct_id, 
                                  trx_result_tuple_t& trt);

    w_rc_t staged_no_one_ol(ol_item_info* polin,
                            time_t tstamp, 
                            int a_wh_id,
                            int a_d_id,
                            int item_cnt,
                            const int xct_id, 
                            trx_result_tuple_t& trt);


}; // EOF ShoreTPCCEnv



inline void ShoreTPCCEnv::print_sf()
{
    TRACE( TRACE_ALWAYS, "*** ShoreTPCCEnv ***\n");
    TRACE( TRACE_ALWAYS, "Scaling Factor = (%d)\n", get_sf());
    TRACE( TRACE_ALWAYS, "Queried Factor = (%d)\n", get_qf());
}

    


EXIT_NAMESPACE(tpcc);


#endif /* __SHORE_TPCC_ENV_H */

