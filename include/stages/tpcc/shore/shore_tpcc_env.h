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

#include "stages/tpcc/common/tpcc_scaling_factor.h"
#include "stages/tpcc/common/tpcc_const.h"
#include "stages/tpcc/common/tpcc_struct.h"
#include "stages/tpcc/common/tpcc_input.h"
#include "stages/tpcc/common/trx_packet.h"

#include "sm/shore/shore_env.h"

#include "stages/tpcc/shore/shore_tpcc_schema.h"

#include <map>


using namespace qpipe;
using namespace shore;



ENTER_NAMESPACE(tpcc);


using std::map;


/******** Exported variables ********/

class ShoreTPCCEnv;
extern ShoreTPCCEnv* shore_env;



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
    warehouse_t        _warehouse;
    district_t         _district;
    customer_t         _customer;
    history_t          _history;
    new_order_t        _new_order;
    order_t            _order;
    order_line_t       _order_line;
    item_t             _item;
    stock_t            _stock;

    tpcc_table_list_t  _table_list;

    /** scaling factors */
    int             _scaling_factor; /* scaling factor - SF=1 -> 100MB database */
    pthread_mutex_t _scaling_mutex;
    int             _queried_factor; /* queried factor - how many of the WHs queried */
    pthread_mutex_t _queried_mutex;

    /** some stats */
    tpcc_stats_t _tpcc_stats; 


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
        : ShoreEnv(confname), _scaling_factor(aSF), _queried_factor(aSF)
    {
        assert (aSF > 0);
        assert (aQF > 0);
        assert (aSF >= aQF);

        pthread_mutex_init(&_scaling_mutex, NULL);
        pthread_mutex_init(&_queried_mutex, NULL);
        
        /* add the tables to the list */
        _table_list.push_back(&_warehouse);
        _table_list.push_back(&_district);
        _table_list.push_back(&_customer);
        _table_list.push_back(&_history);
        _table_list.push_back(&_new_order);
        _table_list.push_back(&_order);
        _table_list.push_back(&_order_line);
        _table_list.push_back(&_item);
        _table_list.push_back(&_stock);

        assert (_table_list.size() == SHORE_TPCC_TABLES);
    }

    ~ShoreTPCCEnv() 
    {
        pthread_mutex_destroy(&_scaling_mutex);
        pthread_mutex_destroy(&_queried_mutex);
                
        _table_list.clear();
    }

    /** Public methods */    

    /* --- operations over tables --- */
    w_rc_t loaddata();  
    w_rc_t check_consistency();


    /* --- access to the tables --- */
    warehouse_t*  warehouse() { return (&_warehouse); }
    district_t*   district()  { return (&_district); }
    customer_t*   customer()  { return (&_customer); }
    history_t*    history()   { return (&_history); }
    new_order_t*  new_order() { return (&_new_order); }
    order_t*      order()     { return (&_order); }
    order_line_t* orderline() { return (&_order_line); }
    item_t*       item()      { return (&_item); }
    stock_t*      stock()     { return (&_stock); }


    /* --- kit baseline trxs --- */
    w_rc_t run_new_order(const int xct_id, trx_result_tuple_t& atrt);
    w_rc_t run_payment(const int xct_id, trx_result_tuple_t& atrt);
    w_rc_t run_order_status(const int xct_id, trx_result_tuple_t& atrt);
    w_rc_t run_delivery(const int xct_id, trx_result_tuple_t& atrt);
    w_rc_t run_stock_level(const int xct_id, trx_result_tuple_t& atrt);

    /* --- access methods --- */
    void set_qf(const int aQF);
    inline int get_qf() { return (_queried_factor); }
    inline int get_sf() { return (_scaling_factor); }
    inline tpcc_table_list_t* table_list() { return (&_table_list); }

}; // EOF ShoreTPCCEnv
    
    


/****************************************************************** 
 *
 *  @class tpcc_loading_smt_t
 *
 *  @brief An smthread inherited class that it is used for spawning
 *         multiple tpcc loading threads. 
 *
 ******************************************************************/

class tpcc_loading_smt_t : public smthread_t 
{
private:
    ss_m*         _pssm;
    tpcc_table_t* _ptable;
    const int     _sf;
    const int     _cnt;
    int           _rv;

public:
    
    tpcc_loading_smt_t(ss_m* assm, tpcc_table_t* atable, 
                       const int asf, const int aid) 
	: smthread_t(t_regular), _pssm(assm), _ptable(atable), 
          _sf(asf), _cnt(aid)
    {
        assert (_pssm);
        assert (_ptable);
        assert (_sf);
    }

    ~tpcc_loading_smt_t() { }

    // thread entrance
    void run();

    inline rv() { return (_rv); }

}; // EOF: tpcc_loading_smt_t



/****************************************************************** 
 *
 *  @class tpcc_checking_smt_t
 *
 *  @brief An smthread inherited class that it is used for spawning
 *         multiple tpcc checking consistency threads. 
 *
 ******************************************************************/

class tpcc_checking_smt_t : public smthread_t 
{
private:
    ss_m*         _pssm;
    tpcc_table_t* _ptable;
    const int     _cnt;

public:
    
    tpcc_checking_smt_t(ss_m* assm, tpcc_table_t* atable, const int aid) 
	: smthread_t(t_regular), _pssm(assm), _ptable(atable), _cnt(aid)
    {
        assert (_pssm);
        assert (_ptable);
    }

    ~tpcc_checking_smt_t() { }

    // thread entrance
    void run();

}; // EOF: tpcc_checking_smt_t


EXIT_NAMESPACE(tpcc);


#endif /* __SHORE_TPCC_ENV_H */

