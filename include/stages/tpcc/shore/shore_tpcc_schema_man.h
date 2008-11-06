/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_schema_man.h
 *
 *  @brief:  Declaration of the TPC-C table managers
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_TPCC_SCHEMA_MANAGER_H
#define __SHORE_TPCC_SCHEMA_MANAGER_H

#include "stages/tpcc/shore/shore_tpcc_schema.h"


using namespace shore;


ENTER_NAMESPACE(tpcc);



/* ------------------------------------------------------------------ */
/* --- The managers of all the tables used in the TPC-C benchmark --- */
/* ------------------------------------------------------------------ */


class warehouse_man_impl : public table_man_impl<warehouse_t>
{
    typedef row_impl<warehouse_t> warehouse_tuple;

public:

    warehouse_man_impl(warehouse_t* aWarehouseDesc)
        : table_man_impl<warehouse_t>(aWarehouseDesc)
    { }

    ~warehouse_man_impl() { }

    /* --- access specific tuples  --- */
    w_rc_t wh_index_probe(ss_m* db, 
                          warehouse_tuple* ptuple, 
                          const int w_id);
    
    w_rc_t wh_index_probe_forupdate(ss_m* db, 
                                    warehouse_tuple* ptuple, 
                                    const int w_id);
    
    w_rc_t wh_index_probe_nl(ss_m* db, 
                             warehouse_tuple* ptuple, 
                             const int w_id);

    /* --- update a retrieved tuple --- */
    w_rc_t wh_update_ytd(ss_m* db,
                         warehouse_tuple* ptuple,
                         const double h_amount,
                         lock_mode_t lm = EX);

    w_rc_t wh_update_ytd_nl(ss_m* db,
                            warehouse_tuple* ptuple,
                            const double h_amount);

    /** deprecated */
    
    w_rc_t bulkload(ss_m* db, int w_num);
    
}; // EOF: warehouse_man_impl



class district_man_impl : public table_man_impl<district_t>
{
    typedef row_impl<district_t> district_tuple;

public:

    district_man_impl(district_t* aDistrictDesc)
        : table_man_impl<district_t>(aDistrictDesc)
    { }

    ~district_man_impl() { }

    /* --- access specific tuples --- */
    w_rc_t dist_index_probe(ss_m* db,
                            district_tuple* ptuple,
                            const int d_id,
                            const int w_id);

    w_rc_t dist_index_probe_forupdate(ss_m* db,
                                      district_tuple* ptuple,
                                      const int d_id,
                                      const int w_id);

    w_rc_t dist_index_probe_nl(ss_m* db,
                               district_tuple* ptuple,
                               const int d_id,
                               const int w_id);

    /* --- update a retrieved tuple --- */
    w_rc_t dist_update_ytd(ss_m* db,
                           district_tuple* ptuple,
                           const double h_amount,
                           lock_mode_t lm = EX);

    w_rc_t dist_update_ytd_nl(ss_m* db,
                              district_tuple* ptuple,
                              const double h_amount);


    w_rc_t dist_update_next_o_id(ss_m* db,
                                 district_tuple* ptuple,
                                 const int  next_o_id,
                                 lock_mode_t lm = EX);

    w_rc_t dist_update_next_o_id_nl(ss_m* db,
                                    district_tuple* ptuple,
                                    const int  next_o_id);

    /** deprecated */

    w_rc_t bulkload(ss_m* db, int w_num);

}; // EOF: district_man_impl



class customer_man_impl : public table_man_impl<customer_t>
{
    typedef row_impl<customer_t> customer_tuple;
    typedef table_scan_iter_impl<customer_t> customer_table_iter;
    typedef index_scan_iter_impl<customer_t> customer_index_iter;

public:

    customer_man_impl(customer_t* aCustomerDesc)
        : table_man_impl<customer_t>(aCustomerDesc)
    { }

    ~customer_man_impl() { }

    /* --- access specific tuples --- */
    w_rc_t cust_get_iter_by_index(ss_m* db,
                                  customer_index_iter* &iter,
                                  customer_tuple* ptuple,
                                  rep_row_t &replow,
                                  rep_row_t &rephigh,
                                  const int w_id,
                                  const int d_id,
                                  const char* c_last);

    w_rc_t cust_index_probe(ss_m* db,
                            customer_tuple* ptuple,
                            const int c_id,
                            const int w_id,
                            const int d_id);
    
    w_rc_t cust_index_probe_by_name(ss_m* db,
                                    const char* idx_name,
                                    customer_tuple* ptuple,
                                    const int c_id,
                                    const int w_id,
                                    const int d_id);

    w_rc_t cust_index_probe_forupdate(ss_m* db,
                                      customer_tuple* ptuple,
                                      const int c_id,
                                      const int w_id,
                                      const int d_id);

    w_rc_t cust_index_probe_nl(ss_m* db,
                               customer_tuple* ptuple,
                               const int c_id,
                               const int w_id,
                               const int d_id);


    /* --- update a retrieved tuple --- */
    w_rc_t cust_update_tuple(ss_m* db,
                             customer_tuple* ptuple,
                             const tpcc_customer_tuple& acustomer,
                             const char* adata1 = NULL,
                             const char* adata2 = NULL,
                             lock_mode_t lm = EX);

    w_rc_t cust_update_tuple_nl(ss_m* db,
                                customer_tuple* ptuple,
                                const tpcc_customer_tuple& acustomer,
                                const char* adata1 = NULL,
                                const char* adata2 = NULL);

    
    w_rc_t cust_update_discount_balance(ss_m* db,
                                        customer_tuple* ptuple,
                                        const decimal discount,
                                        const decimal balance,
                                        lock_mode_t lm = EX);
    
    w_rc_t cust_update_discount_balance_nl(ss_m* db,
                                           customer_tuple* ptuple,
                                           const decimal discount,
                                           const decimal balance);

    /** deprecated */

    w_rc_t bulkload(ss_m* db, int w_num);

}; // EOF: customer_man_impl




class history_man_impl : public table_man_impl<history_t>
{
    typedef row_impl<history_t> history_tuple;

public:

    history_man_impl(history_t* aHistoryDesc)
        : table_man_impl<history_t>(aHistoryDesc)
    { }

    ~history_man_impl() { }

    /** deprecated */

    w_rc_t bulkload(ss_m* db, int w_num);

}; // EOF: history_man_impl


class new_order_man_impl : public table_man_impl<new_order_t>
{
    typedef row_impl<new_order_t> new_order_tuple;
    typedef table_scan_iter_impl<new_order_t> new_order_table_iter;
    typedef index_scan_iter_impl<new_order_t> new_order_index_iter;

public:

    new_order_man_impl(new_order_t* aNewOrderDesc)
        : table_man_impl<new_order_t>(aNewOrderDesc)
    {
    }

    ~new_order_man_impl()
    {
    }

    /* --- access specific tuples --- */     
    w_rc_t no_get_iter_by_index(ss_m* db,
                                new_order_index_iter* &iter,
                                new_order_tuple* ptuple,
                                rep_row_t &replow,
                                rep_row_t &rephigh,
                                const int w_id,
                                const int d_id);

    /* --- update a retrieved tuple --- */
    w_rc_t no_delete_by_index(ss_m* db,
                              new_order_tuple* ptuple,
                              const int w_id,
                              const int d_id,
                              const int o_id);

    w_rc_t no_delete_by_index_nl(ss_m* db,
                                 new_order_tuple* ptuple,
                                 const int w_id,
                                 const int d_id,
                                 const int o_id);
    
    /** deprecated */

    w_rc_t bulkload(ss_m* db, int w_num);
     
}; // EOF: new_order_man_impl


class order_man_impl : public table_man_impl<order_t>
{
    typedef row_impl<order_t> order_tuple;
    typedef table_scan_iter_impl<order_t> order_table_iter;
    typedef index_scan_iter_impl<order_t> order_index_iter;

private:
    int* _pcnt_array;
    pthread_mutex_t* _pcnt_array_mutex;
    w_rc_t bulkload(ss_m* db, int w_num, int* apcnt_array);

public:

    order_man_impl(order_t* aOrderDesc)
        : table_man_impl<order_t>(aOrderDesc)
    {
    }

    ~order_man_impl()
    {
    }

    /* --- access specific tuples --- */         
    w_rc_t ord_get_iter_by_index(ss_m* db,
                                 order_index_iter* &iter,
                                 order_tuple* ptuple,
                                 rep_row_t &replow,
                                 rep_row_t &rephigh,
                                 const int w_id,
                                 const int d_id,
                                 const int c_id);

    /* --- update a retrieved tuple --- */
    w_rc_t ord_update_carrier_by_index(ss_m* db,
                                       order_tuple* ptuple,
                                       const int carrier_id);

    w_rc_t ord_update_carrier_by_index_nl(ss_m* db,
                                          order_tuple* ptuple,
                                          const int carrier_id);
    
    /** deprecated */

    /* cnt_array */
    void produce_cnt_array(int w_num, pthread_mutex_t* parray_mutex);
    inline void free_cnt_array() {
        if (_pcnt_array)
            free(_pcnt_array);
        _pcnt_array = NULL;
    }
    inline int* get_cnt_array() { return (_pcnt_array); }
    inline pthread_mutex_t* get_cnt_mutex() { return (_pcnt_array_mutex); }

    w_rc_t bulkload(ss_m* db, int w_num);

}; // EOF: order_man_impl



class order_line_man_impl : public table_man_impl<order_line_t>
{
    typedef row_impl<order_line_t> order_line_tuple;
    typedef table_scan_iter_impl<order_line_t> order_line_table_iter;
    typedef index_scan_iter_impl<order_line_t> order_line_index_iter;

private:
    int* _pcnt_array;
    pthread_mutex_t* _pcnt_array_mutex;
    w_rc_t bulkload(ss_m* db, int w_num, int* apcnt_array);

public:

    order_line_man_impl(order_line_t* aOrderLineDesc)
        : table_man_impl<order_line_t>(aOrderLineDesc)
    {
    }

    ~order_line_man_impl()
    {
    }

    /* --- access the table --- */
    w_rc_t ol_get_iter_by_index(ss_m* db,
                                order_line_index_iter* &iter,
                                order_line_tuple* ptuple,
                                rep_row_t &replow,
                                rep_row_t &rephigh,
                                const int w_id,
                                const int d_id,
                                const int low_o_id,
                                const int high_o_id);
    
    w_rc_t ol_get_iter_by_index(ss_m* db,
                                order_line_index_iter* &iter,
                                order_line_tuple* ptuple,
                                rep_row_t &replow,
                                rep_row_t &rephigh,
                                const int w_id,
                                const int d_id,
                                const int o_id);

    /** deprecated */

    /* cnt_array */

    /** @note: the cnt_array should be created by order_t */
    inline void set_cnt_array(int* apcnt_array, pthread_mutex_t* pmutex) { 
        _pcnt_array_mutex = pmutex; assert(_pcnt_array_mutex);
        _pcnt_array = apcnt_array; assert(_pcnt_array); 
    }
    inline int* get_cnt_array() { return (_pcnt_array); }
    inline pthread_mutex_t* get_cnt_mutex() { return (_pcnt_array_mutex); }

    w_rc_t bulkload(ss_m* db, int w_num);

}; // EOF: order_line_man_impl



class item_man_impl : public table_man_impl<item_t>
{
    typedef row_impl<item_t> item_tuple;
    typedef table_scan_iter_impl<item_t> item_table_iter;
    typedef index_scan_iter_impl<item_t> item_index_iter;

public:

    item_man_impl(item_t* aItemDesc)
        : table_man_impl<item_t>(aItemDesc)
    {
    }

    ~item_man_impl()
    {
    }

    /* --- access the table --- */
    w_rc_t it_index_probe(ss_m* ddb, 
                          item_tuple* ptuple,
                          const int i_id);

    w_rc_t it_index_probe_forupdate(ss_m* db, 
                                    item_tuple* ptuple,
                                    const int i_id);

    w_rc_t it_index_probe_nl(ss_m* db, 
                             item_tuple* ptuple,
                             const int i_id);

    /** deprecated */

    w_rc_t bulkload(ss_m* db, int w_num);

}; // EOF: item_man_impl



class stock_man_impl : public table_man_impl<stock_t>
{
    typedef row_impl<stock_t> stock_tuple;
    typedef table_scan_iter_impl<stock_t> stock_table_iter;
    typedef index_scan_iter_impl<stock_t> stock_index_iter;

public:

    stock_man_impl(stock_t* aStockDesc)
        : table_man_impl<stock_t>(aStockDesc)
    {
    }

    ~stock_man_impl()
    {
    }

    /* --- access the table --- */
    w_rc_t st_index_probe(ss_m* db,
                          stock_tuple* ptuple,
                          const int i_id,
                          const int w_id);

    w_rc_t st_index_probe_forupdate(ss_m* db,
                                    stock_tuple* ptuple,
                                    const int i_id,
                                    const int w_id);

    w_rc_t st_index_probe_nl(ss_m* db,
                             stock_tuple* ptuple,
                             const int i_id,
                             const int w_id);

    /* --- update a retrieved tuple --- */
    w_rc_t st_update_tuple(ss_m* db,
                           stock_tuple* ptuple,
                           const tpcc_stock_tuple* pstock,
                           lock_mode_t lm = EX);

    w_rc_t st_update_tuple_nl(ss_m* db,
                              stock_tuple* ptuple,
                              const tpcc_stock_tuple* pstock);

    /** deprecated */

    w_rc_t bulkload(ss_m* db, int w_num);

}; // EOF: stock_man_impl


EXIT_NAMESPACE(tpcc);

#endif /* __SHORE_TPCC_SCHEMA_MANAGER_H */
