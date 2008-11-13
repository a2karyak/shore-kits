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
    int volatile  _no_att;
    int volatile  _no_com;
    tatas_lock    _no_lock;
    int volatile  _pay_att;
    int volatile  _pay_com;
    tatas_lock    _pay_lock;
    int volatile  _ord_att;
    int volatile  _ord_com;
    tatas_lock    _ord_lock;
    int volatile  _del_att;
    int volatile  _del_com;
    tatas_lock    _del_lock;
    int volatile  _sto_att;
    int volatile  _sto_com;
    tatas_lock    _sto_lock;

    int volatile  _other_att;
    int volatile  _other_com;
    tatas_lock    _other_lock;    

    tpcc_stats_t() 
        : _no_att(0), _no_com(0), _pay_att(0), _pay_com(0), _ord_att(0),
          _ord_com(0), _del_att(0), _del_com(0), _sto_att(0), _sto_com(0),
          _other_att(0), _other_com(0)
    {
    }

    ~tpcc_stats_t()
    {
        //print_trx_stats();
    }

    // Reset counters
    void reset() {
        // grabs all the locks and resets
        CRITICAL_SECTION(res_no_cs, _no_lock);
        CRITICAL_SECTION(res_pay_cs, _pay_lock);
        CRITICAL_SECTION(res_ord_cs, _ord_lock);
        CRITICAL_SECTION(res_del_cs, _del_lock);
        CRITICAL_SECTION(res_sto_cs, _sto_lock);
        CRITICAL_SECTION(res_other_cs, _other_lock);
                
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
        _other_att = 0;
        _other_com = 0;
    }


    // Prints stats
    void print_trx_stats() const;

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

    int inc_other_att() { 
        CRITICAL_SECTION(att_other_cs, _other_lock);
        return (++_other_att); 
    }

    int inc_other_com() { 
        CRITICAL_SECTION(com_other_cs, _other_lock);
        ++_other_att;
        return (++_other_com); 
    }

    int get_total_committed() {
        CRITICAL_SECTION(com_no_cs,  _no_lock);
        CRITICAL_SECTION(com_pay_cs, _pay_lock);
        CRITICAL_SECTION(com_ord_cs, _ord_lock);
        CRITICAL_SECTION(com_del_cs, _del_lock);
        CRITICAL_SECTION(com_sto_cs, _sto_lock);
        CRITICAL_SECTION(com_other_cs, _other_lock);
        return (_no_com + _pay_com + _ord_com + _del_com + _sto_com + _other_com);
    }

    int get_total_attempted() {
        CRITICAL_SECTION(att_no_cs,  _no_lock);
        CRITICAL_SECTION(att_pay_cs, _pay_lock);
        CRITICAL_SECTION(att_ord_cs, _ord_lock);
        CRITICAL_SECTION(att_del_cs, _del_lock);
        CRITICAL_SECTION(att_sto_cs, _sto_lock);
        CRITICAL_SECTION(att_other_cs, _other_lock);
        return (_no_att + _pay_att + _ord_att + _del_att + _sto_att + _other_att);
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
protected:       
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
    int             _queried_factor; /* queried factor - how many of the WHs queried */
    pthread_mutex_t _queried_mutex;

    // some stats
    tpcc_stats_t   _total_tpcc_stats;     // the stats for the whole env life (never reset)
    tpcc_stats_t   _session_tpcc_stats; // temp stats (reset between runs)


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
    
    
public:    

    /** Construction  */
    ShoreTPCCEnv(string confname)
        : ShoreEnv(confname), 
          _scaling_factor(TPCC_SCALING_FACTOR), 
          _queried_factor(QUERIED_TPCC_SCALING_FACTOR)
    {
        // read the scaling factor from the configuration file
        pthread_mutex_init(&_scaling_mutex, NULL);
        pthread_mutex_init(&_queried_mutex, NULL);
    }


    ~ShoreTPCCEnv() 
    {
        pthread_mutex_destroy(&_scaling_mutex);
        pthread_mutex_destroy(&_queried_mutex);
                
        _table_desc_list.clear();
        _table_man_list.clear();     

        print_total_tpcc_stats();
    }


    // DB INTERFACE

    virtual const int set(envVarMap* vars) { return(0); /* do nothing */ };
    virtual const int open() { return(0); /* do nothing */ };
    virtual const int start() { return(0); /* do nothing */ };
    virtual const int stop() { return(0); /* do nothing */ };
    virtual const int pause() { return(0); /* do nothing */ };
    virtual const int resume() { return(0); /* do nothing */ };    
    virtual const int newrun() { return(0); /* do nothing */ };

    virtual const int post_init();
    virtual const int load_schema();

private:
    w_rc_t _post_init_impl();

public:


    // --- statistics --- //
    void print_total_tpcc_stats() const { 
        _total_tpcc_stats.print_trx_stats(); 
        _env_stats.print_env_stats(); 
    }

    tpcc_stats_t* get_total_tpcc_stats() {
        return (&_total_tpcc_stats);
    }

    void print_session_tpcc_stats() const {
        _session_tpcc_stats.print_trx_stats();
    }

    tpcc_stats_t* get_session_tpcc_stats() {
        return (&_session_tpcc_stats);
    }

    void reset_session_tpcc_stats() {
        _session_tpcc_stats.reset();
    }
    

    // --- scaling and querying factor --- //
    void print_sf(void);
    void set_qf(const int aQF);
    inline int get_qf() { return (_queried_factor); }
    void set_sf(const int aSF);
    inline int get_sf() { return (_scaling_factor); }

    inline tpcc_table_desc_list* table_desc_list() { return (&_table_desc_list); }
    inline table_man_list_t*  table_man_list() { return (&_table_man_list); }
    const int dump();


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





    // *** DEPRECATED ***//
    /* --- kit staged trxs --- */
    /* staged payment */
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

    /* staged new order */
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



    // update statistics

    // !!! unsafe - may cause deadlock if non-other trxs are updating stats also
    int _inc_other_com() { 
        CRITICAL_SECTION(com_other_cs, _total_tpcc_stats._other_lock);
        ++_total_tpcc_stats._other_att;
        ++_total_tpcc_stats._other_com;
        ++_session_tpcc_stats._other_att;
        ++_session_tpcc_stats._other_com;
        ++_env_stats._ntrx_att;
        ++_env_stats._ntrx_com;        
        return (0); 
    }
    int _inc_other_att() { 
        CRITICAL_SECTION(com_other_cs, _total_tpcc_stats._other_lock);
        ++_total_tpcc_stats._other_att;
        ++_session_tpcc_stats._other_att;
        ++_env_stats._ntrx_att;
        return (0); 
    }

}; // EOF ShoreTPCCEnv
   


EXIT_NAMESPACE(tpcc);


#endif /* __SHORE_TPCC_ENV_H */

