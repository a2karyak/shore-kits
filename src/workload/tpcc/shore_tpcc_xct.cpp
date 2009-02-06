/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_env.cpp
 *
 *  @brief:  Implementation of the Baseline Shore TPC-C transactions
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#include "workload/tpcc/shore_tpcc_env.h"
#include "stages/tpcc/common/tpcc_random.h"

#include <vector>
#include <numeric>
#include <algorithm>

using namespace shore;


ENTER_NAMESPACE(tpcc);


/******************************************************************** 
 *
 * TPC-C TRXS
 *
 * (1) The run_XXX functions are wrappers to the real transactions
 * (2) The xct_XXX functions are the implementation of the transactions
 *
 ********************************************************************/


/********************************************************************* 
 *
 *  @fn:    run_one_xct
 *
 *  @brief: Baseline client - Entry point for running one trx 
 *
 *  @note:  The execution of this trx will not be stopped even if the
 *          measure internal has expired.
 *
 *********************************************************************/

w_rc_t ShoreTPCCEnv::run_one_xct(int xct_type, const int xctid, 
                                 const int whid, trx_result_tuple_t& trt)
{
    // if BASELINE TPC-C MIX
    if (xct_type == XCT_MIX) {        
        xct_type = random_xct_type(smthread_t::me()->rand()%100);
    }
    
    switch (xct_type) {
        
        // TPC-C BASELINE
    case XCT_NEW_ORDER:
        return (run_new_order(xctid,trt,whid));
    case XCT_PAYMENT:
        return (run_payment(xctid,trt,whid));;
    case XCT_ORDER_STATUS:
        return (run_order_status(xctid,trt,whid));
    case XCT_DELIVERY:
        return (run_delivery(xctid,trt,whid));
    case XCT_STOCK_LEVEL:
        //return (run_payment(xctid,trt,whid));;
        return (run_stock_level(xctid,trt,whid));

        // MBENCH BASELINE
    case XCT_MBENCH_WH:
        return (run_mbench_wh(xctid,trt,whid));
    case XCT_MBENCH_CUST:
        return (run_mbench_cust(xctid,trt,whid)); 
    default:
        assert (0); // UNKNOWN TRX-ID
    }
    return (RCOK);
}



/******************************************************************** 
 *
 * TPC-C TRXs Wrappers
 *
 * @brief: They are wrappers to the functions that execute the transaction
 *         body. Their responsibility is to:
 *
 *         1. Prepare the corresponding input
 *         2. Check the return of the trx function and abort the trx,
 *            if something went wrong
 *         3. Update the tpcc db environment statistics
 *
 ********************************************************************/

struct xct_count {
    int nord;
    int stock;
    int delivery;
    int payment;
    int status;
    int other;
    xct_count &operator+=(xct_count const& rhs) {
	nord += rhs.nord;
	stock += rhs.stock;
	delivery += rhs.delivery;
	payment += rhs.payment;
	status += rhs.status;
        other += rhs.other;
	return *this;
    }
    xct_count &operator-=(xct_count const& rhs) {
	nord -= rhs.nord;
	stock -= rhs.stock;
	delivery -= rhs.delivery;
	payment -= rhs.payment;
	status -= rhs.status;
        other -= rhs.other;
	return *this;
    }
};
struct xct_stats {
    xct_count attempted;
    xct_count failed;
    xct_stats &operator+=(xct_stats const &other) {
	attempted += other.attempted;
	failed += other.failed;
	return *this;
    };
    xct_stats &operator-=(xct_stats const &other) {
	attempted -= other.attempted;
	failed -= other.failed;
	return *this;
    };
};

static __thread xct_stats my_stats;
typedef std::map<pthread_t, xct_stats*> statmap_t;
static statmap_t statmap;
static pthread_mutex_t statmap_lock = PTHREAD_MUTEX_INITIALIZER;

// hooks for worker_t::_work_ACTIVE_impl
extern void worker_thread_init() {
    CRITICAL_SECTION(cs, statmap_lock);
    statmap[pthread_self()] = &my_stats;
}
extern void worker_thread_fini() {
    CRITICAL_SECTION(cs, statmap_lock);
    statmap.erase(pthread_self());
}

// hook for the shell to get totals
extern xct_stats shell_get_xct_stats() {
    CRITICAL_SECTION(cs, statmap_lock);
    xct_stats rval;
    rval -= rval; // dirty hack to set all zeros
    for(statmap_t::iterator it=statmap.begin(); it != statmap.end(); ++it) 
	rval += *it->second;

    return rval;
}


void ShoreTPCCEnv::_inc_other_att() {
    ++my_stats.attempted.other;
}

void ShoreTPCCEnv::_inc_other_failed() {
    ++my_stats.failed.other;
}

void ShoreTPCCEnv::_inc_nord_att() {
    ++my_stats.attempted.nord;
}

void ShoreTPCCEnv::_inc_nord_failed() {
    ++my_stats.failed.nord;
}

void ShoreTPCCEnv::_inc_pay_att() {
    ++my_stats.attempted.payment;
}

void ShoreTPCCEnv::_inc_pay_failed() {
    ++my_stats.failed.payment;
}

void ShoreTPCCEnv::_inc_ordst_att() {
    ++my_stats.attempted.status;
}

void ShoreTPCCEnv::_inc_ordst_failed() {
    ++my_stats.failed.status;
}

void ShoreTPCCEnv::_inc_deliv_att() {
    ++my_stats.attempted.delivery;
}

void ShoreTPCCEnv::_inc_deliv_failed() {
    ++my_stats.failed.delivery;
}

void ShoreTPCCEnv::_inc_stock_att() {
    ++my_stats.attempted.stock;
}

void ShoreTPCCEnv::_inc_stock_failed() {
    ++my_stats.failed.stock;
}


/* --- with input specified --- */

w_rc_t ShoreTPCCEnv::run_new_order(const int xct_id, 
                                   new_order_input_t& anoin,
                                   trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. NEW-ORDER...\n", xct_id);     
    ++my_stats.attempted.nord;
    w_rc_t e = xct_new_order(&anoin, xct_id, atrt);
    if (e.is_error()) {
	++my_stats.failed.nord;
        TRACE( TRACE_TRX_FLOW, "Xct (%d) NewOrder aborted [0x%x]\n", 
               xct_id, e.err_num());
        
	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) NewOrder abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

	// signal cond var
	if (atrt.get_notify())
	    atrt.get_notify()->signal();

        if (_measure!=MST_MEASURE) {
            return (e);
        }

//        _total_tpcc_stats.inc_no_att();
//        _session_tpcc_stats.inc_no_att();
        _env_stats.inc_trx_att();
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) NewOrder completed\n", xct_id);

    // signal cond var
    if (atrt.get_notify())
        atrt.get_notify()->signal();

    if (_measure!=MST_MEASURE) {
        return (RCOK);
    }

//    _total_tpcc_stats.inc_no_com();
//    _session_tpcc_stats.inc_no_com();
    _env_stats.inc_trx_com();
    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::run_payment(const int xct_id, 
                                 payment_input_t& apin,
                                 trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. PAYMENT...\n", xct_id);     
    
    ++my_stats.attempted.payment;
    w_rc_t e = xct_payment(&apin, xct_id, atrt);
    if (e.is_error()) {
	++my_stats.failed.payment;
        TRACE( TRACE_TRX_FLOW, "Xct (%d) Payment aborted [0x%x]\n", 
               xct_id, e.err_num());

//         stringstream os;
//         os << e << ends;
//         string str = os.str();
//         TRACE( TRACE_ALWAYS, "\n%s\n", str.c_str());

	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) Payment abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

	// signal cond var
	if(atrt.get_notify())
	    atrt.get_notify()->signal();

        if (_measure!=MST_MEASURE) {
            return (e);
        }

//        _total_tpcc_stats.inc_pay_att();
//        _session_tpcc_stats.inc_pay_att();
        _env_stats.inc_trx_att();
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) Payment completed\n", xct_id);

    // signal cond var
    if(atrt.get_notify())
        atrt.get_notify()->signal();

    if (_measure!=MST_MEASURE) {
        return (RCOK); 
    }

//    _total_tpcc_stats.inc_pay_com();
//    _session_tpcc_stats.inc_pay_com();
    _env_stats.inc_trx_com();
    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::run_order_status(const int xct_id, 
                                      order_status_input_t& aordstin,
                                      trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. ORDER-STATUS...\n", xct_id);     

    ++my_stats.attempted.status;
    w_rc_t e = xct_order_status(&aordstin, xct_id, atrt);
    if (e.is_error()) {
	++my_stats.failed.status;
        TRACE( TRACE_TRX_FLOW, "Xct (%d) OrderStatus aborted [0x%x]\n", 
               xct_id, e.err_num());

        w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) OrderStatus abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

	// signal cond var
	if(atrt.get_notify())
	    atrt.get_notify()->signal();

        if (_measure!=MST_MEASURE) {
            return (e);
        }

//        _total_tpcc_stats.inc_ord_att();
//        _session_tpcc_stats.inc_ord_att();
        _env_stats.inc_trx_att();
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) OrderStatus completed\n", xct_id);

    // signal cond var
    if(atrt.get_notify())
        atrt.get_notify()->signal();

    if (_measure!=MST_MEASURE) {
        return (RCOK);
    }

//    _total_tpcc_stats.inc_ord_com();
//    _session_tpcc_stats.inc_ord_com();
    _env_stats.inc_trx_com();
    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::run_delivery(const int xct_id, 
                                  delivery_input_t& adelin,
                                  trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. DELIVERY...\n", xct_id);     

    ++my_stats.attempted.delivery;
    w_rc_t e = xct_delivery(&adelin, xct_id, atrt);
    if (e.is_error()) {
	++my_stats.failed.delivery;
        TRACE( TRACE_TRX_FLOW, "Xct (%d) Delivery aborted [0x%x]\n", 
               xct_id, e.err_num());

	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) Delivery abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

	// signal cond var
	if(atrt.get_notify())
	    atrt.get_notify()->signal();

        if (_measure!=MST_MEASURE) {        
            return (e);
        }

//        _total_tpcc_stats.inc_del_att();
//        _session_tpcc_stats.inc_del_att();
        _env_stats.inc_trx_att();
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) Delivery completed\n", xct_id);

    // signal cond var
    if(atrt.get_notify())
        atrt.get_notify()->signal();

    if (_measure!=MST_MEASURE) {    
        return (RCOK); 
    }

//    _total_tpcc_stats.inc_del_com();
//    _session_tpcc_stats.inc_del_com();
    _env_stats.inc_trx_com();
    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::run_stock_level(const int xct_id, 
                                     stock_level_input_t& astoin,
                                     trx_result_tuple_t& atrt)
{
    TRACE( TRACE_TRX_FLOW, "%d. STOCK-LEVEL...\n", xct_id);     

    ++my_stats.attempted.stock;
    w_rc_t e = xct_stock_level(&astoin, xct_id, atrt);
    if (e.is_error()) {
	++my_stats.failed.stock;
        TRACE( TRACE_TRX_FLOW, "Xct (%d) StockLevel aborted [0x%x]\n", 
               xct_id, e.err_num());

	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) StockLevel abort failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

	// signal cond var
	if(atrt.get_notify())
	    atrt.get_notify()->signal();

        if (_measure!=MST_MEASURE) {
            return (e);
        }

//        _total_tpcc_stats.inc_sto_att();
//        _session_tpcc_stats.inc_sto_att();
        _env_stats.inc_trx_att();
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) StockLevel completed\n", xct_id);

    // signal cond var
    if(atrt.get_notify())
        atrt.get_notify()->signal();

    if (_measure!=MST_MEASURE) {
        return (RCOK);
    }

//    _total_tpcc_stats.inc_sto_com();
//    _session_tpcc_stats.inc_sto_com();
    _env_stats.inc_trx_com();
    return (RCOK); 
}



/* --- without input specified --- */

w_rc_t ShoreTPCCEnv::run_new_order(const int xct_id, 
                                   trx_result_tuple_t& atrt,
                                   int specificWH)
{
    new_order_input_t noin = create_no_input(_queried_factor, specificWH);
    return (run_new_order(xct_id, noin, atrt));
}


w_rc_t ShoreTPCCEnv::run_payment(const int xct_id, 
                                 trx_result_tuple_t& atrt,
                                 int specificWH)
{
    payment_input_t pin = create_payment_input(_queried_factor, specificWH);
    return (run_payment(xct_id, pin, atrt));
}


w_rc_t ShoreTPCCEnv::run_order_status(const int xct_id, 
                                      trx_result_tuple_t& atrt,
                                      int specificWH)
{
    order_status_input_t ordin = create_order_status_input(_queried_factor, specificWH);
    return (run_order_status(xct_id, ordin, atrt));
}


w_rc_t ShoreTPCCEnv::run_delivery(const int xct_id, 
                                  trx_result_tuple_t& atrt,
                                  int specificWH)
{
    delivery_input_t delin = create_delivery_input(_queried_factor, specificWH);
    return (run_delivery(xct_id, delin, atrt));
}


w_rc_t ShoreTPCCEnv::run_stock_level(const int xct_id, 
                                     trx_result_tuple_t& atrt,
                                     int specificWH)
{
    stock_level_input_t slin = create_stock_level_input(_queried_factor, specificWH);
    return (run_stock_level(xct_id, slin, atrt));
 }



/******************************************************************** 
 *
 * @note: The functions below are private, the corresponding run_XXX are
 *        their public wrappers. The run_XXX are required because they
 *        do the trx abort in case something goes wrong inside the body
 *        of each of the transactions.
 *
 ********************************************************************/


// uncomment the line below if want to dump (part of) the trx results
//#define PRINT_TRX_RESULTS


/******************************************************************** 
 *
 * TPC-C NEW_ORDER
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::xct_new_order(new_order_input_t* pnoin, 
                                   const int xct_id, 
                                   trx_result_tuple_t& trt)
{
    // ensure a valid environment
    assert (pnoin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // new_order trx touches 8 tables:
    // warehouse, district, customer, neworder, order, item, stock, orderline
    row_impl<warehouse_t>* prwh = _pwarehouse_man->get_tuple();
    assert (prwh);

    row_impl<district_t>* prdist = _pdistrict_man->get_tuple();
    assert (prdist);

    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

    row_impl<new_order_t>* prno = _pnew_order_man->get_tuple();
    assert (prno);

    row_impl<order_t>* prord = _porder_man->get_tuple();
    assert (prord);

    row_impl<item_t>* pritem = _pitem_man->get_tuple();
    assert (pritem);

    row_impl<stock_t>* prst = _pstock_man->get_tuple();
    assert (prst);

    row_impl<order_line_t>* prol = _porder_line_man->get_tuple();
    assert (prol);

    w_rc_t e = RCOK;

    trt.set_id(xct_id);
    trt.set_state(UNSUBMITTED);
    rep_row_t areprow(_pcustomer_man->ts());

    // allocate space for the biggest of the 8 table representations
    areprow.set(_pcustomer_desc->maxsize()); 

    prwh->_rep = &areprow;
    prdist->_rep = &areprow;

    prcust->_rep = &areprow;
    prno->_rep = &areprow;
    prord->_rep = &areprow;
    pritem->_rep = &areprow;
    prst->_rep = &areprow;
    prol->_rep = &areprow;


    /* SELECT c_discount, c_last, c_credit, w_tax
     * FROM customer, warehouse
     * WHERE w_id = :w_id AND c_w_id = w_id AND c_d_id = :d_id AND c_id = :c_id
     *
     * plan: index probe on "W_INDEX", index probe on "C_INDEX"
     */

    { // make gotos safe

        /* 1. retrieve warehouse (read-only) */
        TRACE( TRACE_TRX_FLOW, 
               "App: %d NO:wh-idx-probe (%d)\n", 
               xct_id, pnoin->_wh_id);
        e = _pwarehouse_man->wh_index_probe(_pssm, prwh, pnoin->_wh_id);
        if (e.is_error()) { goto done; }

        tpcc_warehouse_tuple awh;
        prwh->get_value(7, awh.W_TAX);


        /* SELECT d_tax, d_next_o_id
         * FROM district
         * WHERE d_id = :d_id AND d_w_id = :w_id
         *
         * plan: index probe on "D_INDEX"
         */

        /* 2. retrieve district for update */
        TRACE( TRACE_TRX_FLOW, 
               "App: %d NO:dist-idx-upd (%d) (%d)\n", 
               xct_id, pnoin->_wh_id, pnoin->_d_id);
        e = _pdistrict_man->dist_index_probe_forupdate(_pssm, prdist, 
                                                       pnoin->_wh_id, pnoin->_d_id);
        if (e.is_error()) { goto done; }

        tpcc_district_tuple adist;
        prdist->get_value(8, adist.D_TAX);
        prdist->get_value(10, adist.D_NEXT_O_ID);
        adist.D_NEXT_O_ID++;


        /* 3. retrieve customer */
        TRACE( TRACE_TRX_FLOW, 
               "App: %d NO:cust-idx-probe (%d) (%d) (%d)\n", 
               xct_id, pnoin->_wh_id, pnoin->_d_id, pnoin->_c_id);
        e = _pcustomer_man->cust_index_probe(_pssm, prcust, 
                                             pnoin->_wh_id, 
                                             pnoin->_d_id, 
                                             pnoin->_c_id);
        if (e.is_error()) { goto done; }

        tpcc_customer_tuple  acust;
        prcust->get_value(15, acust.C_DISCOUNT);
        prcust->get_value(13, acust.C_CREDIT, 3);
        prcust->get_value(5, acust.C_LAST, 17);


        /* UPDATE district
         * SET d_next_o_id = :next_o_id+1
         * WHERE CURRENT OF dist_cur
         */

        TRACE( TRACE_TRX_FLOW, 
               "App: %d NO:dist-upd-next-o-id (%d)\n", 
               xct_id, adist.D_NEXT_O_ID);
        e = _pdistrict_man->dist_update_next_o_id(_pssm, prdist, adist.D_NEXT_O_ID);
        if (e.is_error()) { goto done; }


        double total_amount = 0;

        for (int item_cnt=0; item_cnt<pnoin->_ol_cnt; item_cnt++) {

            /* 4. for all items read item, and update stock, and order line */
            register int ol_i_id = pnoin->items[item_cnt]._ol_i_id;
            register int ol_supply_w_id = pnoin->items[item_cnt]._ol_supply_wh_id;


            /* SELECT i_price, i_name, i_data
             * FROM item
             * WHERE i_id = :ol_i_id
             *
             * plan: index probe on "I_INDEX"
             */

            tpcc_item_tuple aitem;
            TRACE( TRACE_TRX_FLOW, "App: %d NO:item-idx-probe (%d)\n", 
                   xct_id, ol_i_id);
            e = _pitem_man->it_index_probe(_pssm, pritem, ol_i_id);
            if (e.is_error()) { goto done; }

            pritem->get_value(4, aitem.I_DATA, 51);
            pritem->get_value(3, aitem.I_PRICE);
            pritem->get_value(2, aitem.I_NAME, 25);

            int item_amount = aitem.I_PRICE * pnoin->items[item_cnt]._ol_quantity; 
            total_amount += item_amount;
            //	info->items[item_cnt].ol_amount = amount;

            /* SELECT s_quantity, s_remote_cnt, s_data, s_dist0, s_dist1, s_dist2, ...
             * FROM stock
             * WHERE s_i_id = :ol_i_id AND s_w_id = :ol_supply_w_id
             *
             * plan: index probe on "S_INDEX"
             */

            tpcc_stock_tuple astock;
            TRACE( TRACE_TRX_FLOW, "App: %d NO:stock-idx-upd (%d) (%d)\n", 
                   xct_id, ol_supply_w_id, ol_i_id);

            e = _pstock_man->st_index_probe_forupdate(_pssm, prst, 
                                                      ol_supply_w_id, ol_i_id);
            if (e.is_error()) { goto done; }

            prst->get_value(0, astock.S_I_ID);
            prst->get_value(1, astock.S_W_ID);
            prst->get_value(5, astock.S_YTD);
            astock.S_YTD += pnoin->items[item_cnt]._ol_quantity;
            prst->get_value(2, astock.S_REMOTE_CNT);        
            prst->get_value(3, astock.S_QUANTITY);
            astock.S_QUANTITY -= pnoin->items[item_cnt]._ol_quantity;
            if (astock.S_QUANTITY < 10) astock.S_QUANTITY += 91;
            prst->get_value(6+pnoin->_d_id, astock.S_DIST[6+pnoin->_d_id], 25);
            prst->get_value(16, astock.S_DATA, 51);

            char c_s_brand_generic;
            if (strstr(aitem.I_DATA, "ORIGINAL") != NULL && 
                strstr(astock.S_DATA, "ORIGINAL") != NULL)
                c_s_brand_generic = 'B';
            else c_s_brand_generic = 'G';

            prst->get_value(4, astock.S_ORDER_CNT);
            astock.S_ORDER_CNT++;

            if (pnoin->_wh_id != ol_supply_w_id) {
                astock.S_REMOTE_CNT++;
            }


            /* UPDATE stock
             * SET s_quantity = :s_quantity, s_order_cnt = :s_order_cnt
             * WHERE s_w_id = :w_id AND s_i_id = :ol_i_id;
             */

            TRACE( TRACE_TRX_FLOW, "App: %d NO:stock-upd-tuple (%d) (%d)\n", 
                   xct_id, astock.S_W_ID, astock.S_I_ID);
            e = _pstock_man->st_update_tuple(_pssm, prst, &astock);
            if (e.is_error()) { goto done; }


            /* INSERT INTO order_line
             * VALUES (o_id, d_id, w_id, ol_ln, ol_i_id, supply_w_id,
             *        '0001-01-01-00.00.01.000000', ol_quantity, iol_amount, dist)
             */

            prol->set_value(0, adist.D_NEXT_O_ID);
            prol->set_value(1, pnoin->_d_id);
            prol->set_value(2, pnoin->_wh_id);
            prol->set_value(3, item_cnt+1);
            prol->set_value(4, ol_i_id);
            prol->set_value(5, ol_supply_w_id);
            prol->set_value(6, pnoin->_tstamp);
            prol->set_value(7, pnoin->items[item_cnt]._ol_quantity);
            prol->set_value(8, item_amount);
            prol->set_value(9, astock.S_DIST[6+pnoin->_d_id]);

            TRACE( TRACE_TRX_FLOW, 
                   "App: %d NO:orderline-add-tuple (%d) (%d) (%d) (%d)\n", 
                   xct_id, pnoin->_wh_id, pnoin->_d_id, adist.D_NEXT_O_ID, item_cnt+1);
            e = _porder_line_man->add_tuple(_pssm, prol);
            if (e.is_error()) { goto done; }

        } /* end for loop */


        /* 5. insert row to orders and new_order */

        /* INSERT INTO orders
         * VALUES (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_ol_cnt, o_all_local)
         */

        prord->set_value(0, adist.D_NEXT_O_ID);
        prord->set_value(1, pnoin->_c_id);
        prord->set_value(2, pnoin->_d_id);
        prord->set_value(3, pnoin->_wh_id);
        prord->set_value(4, pnoin->_tstamp);
        prord->set_value(5, 0);
        prord->set_value(6, pnoin->_ol_cnt);
        prord->set_value(7, pnoin->_all_local);

        TRACE( TRACE_TRX_FLOW, "App: %d NO:ord-add-tuple (%d)\n", 
               xct_id, adist.D_NEXT_O_ID);
        e = _porder_man->add_tuple(_pssm, prord);
        if (e.is_error()) { goto done; }

        /* INSERT INTO new_order VALUES (o_id, d_id, w_id)
         */

        prno->set_value(0, adist.D_NEXT_O_ID);
        prno->set_value(1, pnoin->_d_id);
        prno->set_value(2, pnoin->_wh_id);

        TRACE( TRACE_TRX_FLOW, "App: %d NO:nord-add-tuple (%d) (%d) (%d)\n", 
               xct_id, pnoin->_wh_id, pnoin->_d_id, adist.D_NEXT_O_ID);
    
        e = _pnew_order_man->add_tuple(_pssm, prno);
        if (e.is_error()) { goto done; }

        /* 6. finalize trx */
        e = _pssm->commit_xct();
        if (e.is_error()) { goto done; }

    } // goto

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);


#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prwh->print_tuple();
    prdist->print_tuple();
    prcust->print_tuple();
    prno->print_tuple();
    prord->print_tuple();
    pritem->print_tuple();
    prst->print_tuple();
    prol->print_tuple();
#endif


done:    
    // return the tuples to the cache
    _pwarehouse_man->give_tuple(prwh);
    _pdistrict_man->give_tuple(prdist);
    _pcustomer_man->give_tuple(prcust);
    _pnew_order_man->give_tuple(prno);
    _porder_man->give_tuple(prord);
    _pitem_man->give_tuple(pritem);
    _pstock_man->give_tuple(prst);
    _porder_line_man->give_tuple(prol);
    return (e);

} // EOF: NEW_ORDER




/******************************************************************** 
 *
 * TPC-C PAYMENT
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::xct_payment(payment_input_t* ppin, 
                                 const int xct_id, 
                                 trx_result_tuple_t& trt)
{
    // ensure a valid environment    
    assert (ppin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // payment trx touches 4 tables: 
    // warehouse, district, customer, and history

    // get table tuples from the caches
    row_impl<warehouse_t>* prwh = _pwarehouse_man->get_tuple();
    assert (prwh);

    row_impl<district_t>* prdist = _pdistrict_man->get_tuple();
    assert (prdist);

    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

    row_impl<history_t>* prhist = _phistory_man->get_tuple();
    assert (prhist);

    w_rc_t e = RCOK;

    trt.set_id(xct_id);
    trt.set_state(UNSUBMITTED);
    rep_row_t areprow(_pcustomer_man->ts());

    // allocate space for the biggest of the 4 table representations
    areprow.set(_pcustomer_desc->maxsize()); 

    prwh->_rep = &areprow;
    prdist->_rep = &areprow;
    prcust->_rep = &areprow;
    prhist->_rep = &areprow;

//     /* 0. initiate transaction */
//     W_DO(_pssm->begin_xct());

    { // make gotos safe

        /* 1. retrieve warehouse for update */
        TRACE( TRACE_TRX_FLOW, 
               "App: %d PAY:wh-idx-upd (%d)\n", 
               xct_id, ppin->_home_wh_id);

        e = _pwarehouse_man->wh_index_probe_forupdate(_pssm, prwh, 
                                                      ppin->_home_wh_id);
        if (e.is_error()) { goto done; }


        /* 2. retrieve district for update */
        TRACE( TRACE_TRX_FLOW, 
               "App: %d PAY:dist-idx-upd (%d) (%d)\n", 
               xct_id, ppin->_home_wh_id, ppin->_home_d_id);

        e = _pdistrict_man->dist_index_probe_forupdate(_pssm, prdist,
                                                       ppin->_home_wh_id, ppin->_home_d_id);
        if (e.is_error()) { goto done; }

        /* 3. retrieve customer for update */

        // find the customer wh and d
        int c_w = ( ppin->_v_cust_wh_selection > 85 ? ppin->_home_wh_id : ppin->_remote_wh_id);
        int c_d = ( ppin->_v_cust_wh_selection > 85 ? ppin->_home_d_id : ppin->_remote_d_id);

        if (ppin->_v_cust_ident_selection <= 60) {

            // if (ppin->_c_id == 0) {

            /* 3a. if no customer selected already use the index on the customer name */

            /* SELECT  c_id, c_first
             * FROM customer
             * WHERE c_last = :c_last AND c_w_id = :c_w_id AND c_d_id = :c_d_id
             * ORDER BY c_first
             *
             * plan: index only scan on "C_NAME_INDEX"
             */

            rep_row_t lowrep(_pcustomer_man->ts());
            rep_row_t highrep(_pcustomer_man->ts());

            guard< index_scan_iter_impl<customer_t> > c_iter;
	    {
		index_scan_iter_impl<customer_t>* tmp_c_iter;
		TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-iter-by-name-idx (%s)\n", 
		       xct_id, ppin->_c_last);
		e = _pcustomer_man->cust_get_iter_by_index(_pssm, tmp_c_iter, prcust, 
							   lowrep, highrep,
							   c_w, c_d, ppin->_c_last);
		c_iter = tmp_c_iter;
		if (e.is_error()) { goto done; }
	    }

            vector<int> v_c_id;
            int a_c_id = 0;
            int count = 0;
            bool eof;

            e = c_iter->next(_pssm, eof, *prcust);
            if (e.is_error()) { goto done; }
            while (!eof) {
                // push the retrieved customer id to the vector
                ++count;
                prcust->get_value(0, a_c_id);
                v_c_id.push_back(a_c_id);

                TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-iter-next (%d)\n", 
                       xct_id, a_c_id);
                e = c_iter->next(_pssm, eof, *prcust);
                if (e.is_error()) { goto done; }
            }
            assert (count);

            // 3b. find the customer id in the middle of the list
            ppin->_c_id = v_c_id[(count+1)/2-1];
        }
        assert (ppin->_c_id>0);


        /* SELECT c_first, c_middle, c_last, c_street_1, c_street_2, c_city, 
         * c_state, c_zip, c_phone, c_since, c_credit, c_credit_lim, 
         * c_discount, c_balance, c_ytd_payment, c_payment_cnt 
         * FROM customer 
         * WHERE c_id = :c_id AND c_w_id = :c_w_id AND c_d_id = :c_d_id 
         * FOR UPDATE OF c_balance, c_ytd_payment, c_payment_cnt
         *
         * plan: index probe on "C_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, 
               "App: %d PAY:cust-idx-upd (%d) (%d) (%d)\n", 
               xct_id, c_w, c_d, ppin->_c_id);

        e = _pcustomer_man->cust_index_probe_forupdate(_pssm, prcust, 
                                                       c_w, c_d, ppin->_c_id);
        if (e.is_error()) { goto done; }

        double c_balance, c_ytd_payment;
        int    c_payment_cnt;
        tpcc_customer_tuple acust;

        // retrieve customer
        prcust->get_value(3,  acust.C_FIRST, 17);
        prcust->get_value(4,  acust.C_MIDDLE, 3);
        prcust->get_value(5,  acust.C_LAST, 17);
        prcust->get_value(6,  acust.C_STREET_1, 21);
        prcust->get_value(7,  acust.C_STREET_2, 21);
        prcust->get_value(8,  acust.C_CITY, 21);
        prcust->get_value(9,  acust.C_STATE, 3);
        prcust->get_value(10, acust.C_ZIP, 10);
        prcust->get_value(11, acust.C_PHONE, 17);
        prcust->get_value(12, acust.C_SINCE);
        prcust->get_value(13, acust.C_CREDIT, 3);
        prcust->get_value(14, acust.C_CREDIT_LIM);
        prcust->get_value(15, acust.C_DISCOUNT);
        prcust->get_value(16, acust.C_BALANCE);
        prcust->get_value(17, acust.C_YTD_PAYMENT);
        prcust->get_value(18, acust.C_LAST_PAYMENT);
        prcust->get_value(19, acust.C_PAYMENT_CNT);
        prcust->get_value(20, acust.C_DATA_1, 251);
        prcust->get_value(21, acust.C_DATA_2, 251);

        // update customer fields
        acust.C_BALANCE -= ppin->_h_amount;
        acust.C_YTD_PAYMENT += ppin->_h_amount;
        acust.C_PAYMENT_CNT++;

        // if bad customer
        if (acust.C_CREDIT[0] == 'B' && acust.C_CREDIT[1] == 'C') { 
            /* 10% of customers */

            /* SELECT c_data
             * FROM customer 
             * WHERE c_id = :c_id AND c_w_id = :c_w_id AND c_d_id = :c_d_id
             * FOR UPDATE OF c_balance, c_ytd_payment, c_payment_cnt, c_data
             *
             * plan: index probe on "C_INDEX"
             */

            // update the data
            char c_new_data_1[251];
            char c_new_data_2[251];
            sprintf(c_new_data_1, "%d,%d,%d,%d,%d,%1.2f",
                    ppin->_c_id, c_d, c_w, ppin->_home_d_id, 
                    ppin->_home_wh_id, ppin->_h_amount);

            int len = strlen(c_new_data_1);
            strncat(c_new_data_1, acust.C_DATA_1, 250-len);
            strncpy(c_new_data_2, &acust.C_DATA_1[250-len], len);
            strncpy(c_new_data_2, acust.C_DATA_2, 250-len);

            TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-upd-tuple\n", xct_id);
            e = _pcustomer_man->cust_update_tuple(_pssm, prcust, acust, 
                                                  c_new_data_1, c_new_data_2);
            if (e.is_error()) { goto done; }
        }
        else { /* good customer */
            TRACE( TRACE_TRX_FLOW, "App: %d PAY:cust-upd-tuple\n", xct_id);
            e = _pcustomer_man->cust_update_tuple(_pssm, prcust, acust, 
                                                  NULL, NULL);
            if (e.is_error()) { goto done; }
        }

        /* UPDATE district SET d_ytd = d_ytd + :h_amount
         * WHERE d_id = :d_id AND d_w_id = :w_id
         *
         * SELECT d_street_1, d_street_2, d_city, d_state, d_zip, d_name
         * FROM district
         * WHERE d_id = :d_id AND d_w_id = :w_id
         *
         * plan: index probe on "D_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, "App: %d PAY:dist-upd-ytd (%d) (%d)\n", 
               xct_id, ppin->_home_wh_id, ppin->_home_d_id);
        e = _pdistrict_man->dist_update_ytd(_pssm, prdist, ppin->_h_amount);
        if (e.is_error()) { goto done; }

        tpcc_district_tuple adistr;
        prdist->get_value(2, adistr.D_NAME, 11);
        prdist->get_value(3, adistr.D_STREET_1, 21);
        prdist->get_value(4, adistr.D_STREET_2, 21);
        prdist->get_value(5, adistr.D_CITY, 21);
        prdist->get_value(6, adistr.D_STATE, 3);
        prdist->get_value(7, adistr.D_ZIP, 10);
    

        /* UPDATE warehouse SET w_ytd = wytd + :h_amount
         * WHERE w_id = :w_id
         *
         * SELECT w_name, w_street_1, w_street_2, w_city, w_state, w_zip
         * FROM warehouse
         * WHERE w_id = :w_id
         *
         * plan: index probe on "W_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, "App: %d PAY:wh-update-ytd (%d)\n", 
               xct_id, ppin->_home_wh_id);

        e = _pwarehouse_man->wh_update_ytd(_pssm, prwh, ppin->_h_amount);
        if (e.is_error()) { goto done; }


        tpcc_warehouse_tuple awh;
        prwh->get_value(1, awh.W_NAME, 11);
        prwh->get_value(2, awh.W_STREET_1, 21);
        prwh->get_value(3, awh.W_STREET_2, 21);
        prwh->get_value(4, awh.W_CITY, 21);
        prwh->get_value(5, awh.W_STATE, 3);
        prwh->get_value(6, awh.W_ZIP, 10);


        /* INSERT INTO history
         * VALUES (:c_id, :c_d_id, :c_w_id, :d_id, :w_id, 
         *         :curr_tmstmp, :ih_amount, :h_data)
         */

        tpcc_history_tuple ahist;
        sprintf(ahist.H_DATA, "%s   %s", awh.W_NAME, adistr.D_NAME);
        ahist.H_DATE = time(NULL);

        prhist->set_value(0, ppin->_c_id);
        prhist->set_value(1, c_d);
        prhist->set_value(2, c_w);
        prhist->set_value(3, ppin->_home_d_id);
        prhist->set_value(4, ppin->_home_wh_id);
        prhist->set_value(5, ahist.H_DATE);
        prhist->set_value(6, ppin->_h_amount * 100.0);
        prhist->set_value(7, ahist.H_DATA);

        TRACE( TRACE_TRX_FLOW, "App: %d PAY:hist-add-tuple\n", xct_id);
        e = _phistory_man->add_tuple(_pssm, prhist);
        if (e.is_error()) { goto done; }


        /* 4. commit */
        e = _pssm->commit_xct();
        if (e.is_error()) { goto done; }

    } // goto

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);


#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prwh->print_tuple();
    prdist->print_tuple();
    prcust->print_tuple();
    prhist->print_tuple();
#endif


done:
    // return the tuples to the cache
    _pwarehouse_man->give_tuple(prwh);
    _pdistrict_man->give_tuple(prdist);
    _pcustomer_man->give_tuple(prcust);
    _phistory_man->give_tuple(prhist);  
    return (e);

} // EOF: PAYMENT



/******************************************************************** 
 *
 * TPC-C ORDER STATUS
 *
 * Input: w_id, d_id, c_id (use c_last if set to null), c_last
 *
 * @note: Read-Only trx
 *
 ********************************************************************/


w_rc_t ShoreTPCCEnv::xct_order_status(order_status_input_t* pstin, 
                                      const int xct_id, 
                                      trx_result_tuple_t& trt)
{
    // ensure a valid environment    
    assert (pstin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    register int w_id = pstin->_wh_id;
    register int d_id = pstin->_d_id;

    // order_status trx touches 3 tables: 
    // customer, order and orderline

    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

    row_impl<order_t>* prord = _porder_man->get_tuple();
    assert (prord);

    row_impl<order_line_t>* prol = _porder_line_man->get_tuple();
    assert (prol);

    w_rc_t e = RCOK;

    trt.set_id(xct_id);
    trt.set_state(UNSUBMITTED);
    rep_row_t areprow(_pcustomer_man->ts());

    // allocate space for the biggest of the 3 table representations
    areprow.set(_pcustomer_desc->maxsize()); 

    prcust->_rep = &areprow;
    prord->_rep = &areprow;
    prol->_rep = &areprow;


    rep_row_t lowrep(_pcustomer_man->ts());
    rep_row_t highrep(_pcustomer_man->ts());

    tpcc_orderline_tuple* porderlines = NULL;

    // allocate space for the biggest of the (customer) and (order)
    // table representations
    lowrep.set(_pcustomer_desc->maxsize()); 
    highrep.set(_pcustomer_desc->maxsize()); 

    { // make gotos safe

        /* 1a. select customer based on name */
        if (pstin->_c_id == 0) {
            /* SELECT  c_id, c_first
             * FROM customer
             * WHERE c_last = :c_last AND c_w_id = :w_id AND c_d_id = :d_id
             * ORDER BY c_first
             *
             * plan: index only scan on "C_NAME_INDEX"
             */

            assert (pstin->_c_select <= 60);
            assert (pstin->_c_last);

            guard<index_scan_iter_impl<customer_t> > c_iter;
	    {
		index_scan_iter_impl<customer_t>* tmp_c_iter;
		TRACE( TRACE_TRX_FLOW, "App: %d ORDST:cust-iter-by-name-idx\n", xct_id);
		e = _pcustomer_man->cust_get_iter_by_index(_pssm, tmp_c_iter, prcust,
							   lowrep, highrep,
							   w_id, d_id, pstin->_c_last);
		c_iter = tmp_c_iter;
		if (e.is_error()) { goto done; }
	    }

            int  c_id_list[17];
            int  count = 0;
            bool eof;

            e = c_iter->next(_pssm, eof, *prcust);
            if (e.is_error()) { goto done; }
            while (!eof) {

                // put the retrieved customer id to the list
                prcust->get_value(0, c_id_list[count++]);            
                TRACE( TRACE_TRX_FLOW, "App: %d ORDST:cust-iter-next\n", xct_id);
                e = c_iter->next(_pssm, eof, *prcust);
                if (e.is_error()) { goto done; }
            }

            /* find the customer id in the middle of the list */
            pstin->_c_id = c_id_list[(count+1)/2-1];
        }
        assert (pstin->_c_id>0);


        /* 1. probe the customer */

        /* SELECT c_first, c_middle, c_last, c_balance
         * FROM customer
         * WHERE c_id = :c_id AND c_w_id = :w_id AND c_d_id = :d_id
         *
         * plan: index probe on "C_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, 
               "App: %d ORDST:cust-idx-probe (%d) (%d) (%d)\n", 
               xct_id, w_id, d_id, pstin->_c_id);
        e = _pcustomer_man->cust_index_probe(_pssm, prcust, 
                                             w_id, d_id, pstin->_c_id);
        if (e.is_error()) { goto done; }
            
        tpcc_customer_tuple acust;
        prcust->get_value(3,  acust.C_FIRST, 17);
        prcust->get_value(4,  acust.C_MIDDLE, 3);
        prcust->get_value(5,  acust.C_LAST, 17);
        prcust->get_value(16, acust.C_BALANCE);


        /* 2. retrieve the last order of this customer */

        /* SELECT o_id, o_entry_d, o_carrier_id
         * FROM orders
         * WHERE o_w_id = :w_id AND o_d_id = :d_id AND o_c_id = :o_c_id
         * ORDER BY o_id DESC
         *
         * plan: index scan on "O_CUST_INDEX"
         */
     
        guard<index_scan_iter_impl<order_t> > o_iter;
	{
	    index_scan_iter_impl<order_t>* tmp_o_iter;
	    TRACE( TRACE_TRX_FLOW, "App: %d ORDST:ord-iter-by-idx\n", xct_id);
	    e = _porder_man->ord_get_iter_by_index(_pssm, tmp_o_iter, prord,
						   lowrep, highrep,
						   w_id, d_id, pstin->_c_id);
	    o_iter = tmp_o_iter;
	    if (e.is_error()) { goto done; }
	}

        tpcc_order_tuple aorder;
        bool eof;

        e = o_iter->next(_pssm, eof, *prord);
        if (e.is_error()) { goto done; }
        while (!eof) {
            prord->get_value(0, aorder.O_ID);
            prord->get_value(4, aorder.O_ENTRY_D);
            prord->get_value(5, aorder.O_CARRIER_ID);
            prord->get_value(6, aorder.O_OL_CNT);

            //        rord.print_tuple();

            e = o_iter->next(_pssm, eof, *prord);
            if (e.is_error()) { goto done; }
        }
     
        // we should have retrieved a valid id and ol_cnt for the order               
        assert (aorder.O_ID);
        assert (aorder.O_OL_CNT);
     
        /* 3. retrieve all the orderlines that correspond to the last order */
     
        /* SELECT ol_i_id, ol_supply_w_id, ol_quantity, ol_amount, ol_delivery_d 
         * FROM order_line 
         * WHERE ol_w_id = :H00003 AND ol_d_id = :H00004 AND ol_o_id = :H00016 
         *
         * plan: index scan on "OL_INDEX"
         */

        guard<index_scan_iter_impl<order_line_t> > ol_iter;
	{
	    index_scan_iter_impl<order_line_t>* tmp_ol_iter;
	    TRACE( TRACE_TRX_FLOW, "App: %d ORDST:ol-iter-by-idx\n", xct_id);
	    e = _porder_line_man->ol_get_probe_iter_by_index(_pssm, tmp_ol_iter, prol,
							     lowrep, highrep,
							     w_id, d_id, aorder.O_ID);
	    ol_iter = tmp_ol_iter;
	    if (e.is_error()) { goto done; }
	}
        
        porderlines = new tpcc_orderline_tuple[aorder.O_OL_CNT];
        int i=0;

        e = ol_iter->next(_pssm, eof, *prol);
        if (e.is_error()) { goto done; }
        while (!eof) {
            prol->get_value(4, porderlines[i].OL_I_ID);
            prol->get_value(5, porderlines[i].OL_SUPPLY_W_ID);
            prol->get_value(6, porderlines[i].OL_DELIVERY_D);
            prol->get_value(7, porderlines[i].OL_QUANTITY);
            prol->get_value(8, porderlines[i].OL_AMOUNT);
            i++;

            e = ol_iter->next(_pssm, eof, *prol);
            if (e.is_error()) { goto done; }
        }

        /* 4. commit */
        e = _pssm->commit_xct();
        if (e.is_error()) { goto done; }

    } // goto

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);


#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    rcust.print_tuple();
    rord.print_tuple();
    rordline.print_tuple();
#endif

done:
    // return the tuples to the cache
    _pcustomer_man->give_tuple(prcust);
    _porder_man->give_tuple(prord);  
    _porder_line_man->give_tuple(prol);
    
    if (porderlines) delete [] porderlines;

    return (e);

}; // EOF: ORDER-STATUS



/******************************************************************** 
 *
 * TPC-C DELIVERY
 *
 * Input data: w_id, carrier_id
 *
 * @note: Delivers one new_order (undelivered order) from each district
 *
 ********************************************************************/
unsigned int delivery_abort_ctr = 0;

w_rc_t ShoreTPCCEnv::xct_delivery(delivery_input_t* pdin, 
                                  const int xct_id, 
                                  trx_result_tuple_t& trt)
{
    // ensure a valid environment    
    assert (pdin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    static bool const SPLIT_TRX = false;
    
    register int w_id       = pdin->_wh_id;
    register int carrier_id = pdin->_carrier_id; 
    time_t ts_start = time(NULL);

    // delivery trx touches 4 tables: 
    // new_order, order, orderline, and customer
    row_impl<new_order_t>* prno = _pnew_order_man->get_tuple();
    assert (prno);

    row_impl<order_t>* prord = _porder_man->get_tuple();
    assert (prord);

    row_impl<order_line_t>* prol = _porder_line_man->get_tuple();
    assert (prol);    

    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

    w_rc_t e = RCOK;

    trt.set_id(xct_id);
    trt.set_state(UNSUBMITTED);
    rep_row_t areprow(_pcustomer_man->ts());

    // allocate space for the biggest of the 4 table representations
    areprow.set(_pcustomer_desc->maxsize()); 

    prno->_rep = &areprow;
    prord->_rep = &areprow;
    prol->_rep = &areprow;
    prcust->_rep = &areprow;

    rep_row_t lowrep(_porder_line_man->ts());
    rep_row_t highrep(_porder_line_man->ts());

    // allocate space for the biggest of the (new_order) and (orderline)
    // table representations
    lowrep.set(_porder_line_desc->maxsize()); 
    highrep.set(_porder_line_desc->maxsize()); 

    std::vector<int> dlist(DISTRICTS_PER_WAREHOUSE);
    std::iota(dlist.begin(), dlist.end(), 1);
    if(SPLIT_TRX)
	std::random_shuffle(dlist.begin(), dlist.end());

    int d_id;
    
 again:
    { // make gotos safe

        /* process each district separately */
	while(dlist.size()) {
	    d_id = dlist.back();
	    dlist.pop_back();	    

            /* 1. Get the new_order of the district, with the min value */

            /* SELECT MIN(no_o_id) INTO :no_o_id:no_o_id_i
             * FROM new_order
             * WHERE no_d_id = :d_id AND no_w_id = :w_id
             *
             * plan: index scan on "NO_INDEX"
             */
            TRACE( TRACE_TRX_FLOW, "App: %d DEL:nord-iter-by-idx (%d) (%d)\n", 
                   xct_id, w_id, d_id);
    
	    guard<index_scan_iter_impl<new_order_t> > no_iter;
	    {
		index_scan_iter_impl<new_order_t>* tmp_no_iter;
		e = _pnew_order_man->no_get_iter_by_index(_pssm, tmp_no_iter, prno, 
							  lowrep, highrep,
							  w_id, d_id,
							  EX, false);
		no_iter = tmp_no_iter;
		if (e.is_error()) { goto done; }
	    }

            bool eof;

            // iterate over all new_orders and load their no_o_ids to the sort buffer
            e = no_iter->next(_pssm, eof, *prno);
            if (e.is_error()) { goto done; }

            if (eof) continue; // skip this district

            int no_o_id;
            prno->get_value(0, no_o_id);
            assert (no_o_id);
	    

            /* 2. Delete the retrieved new order */        

            /* DELETE FROM new_order
             * WHERE no_w_id = :w_id AND no_d_id = :d_id AND no_o_id = :no_o_id
             *
             * plan: index scan on "NO_INDEX"
             */

            TRACE( TRACE_TRX_FLOW, "App: %d DEL:nord-delete-by-index (%d) (%d) (%d)\n", 
                   xct_id, w_id, d_id, no_o_id);

            e = _pnew_order_man->no_delete_by_index(_pssm, prno, 
                                                    w_id, d_id, no_o_id);
            if (e.is_error()) { goto done; }

            /* 3a. Update the carrier for the delivered order (in the orders table) */
            /* 3b. Get the customer id of the updated order */

            /* UPDATE orders SET o_carrier_id = :o_carrier_id
             * SELECT o_c_id INTO :o_c_id FROM orders
             * WHERE o_id = :no_o_id AND o_w_id = :w_id AND o_d_id = :d_id;
             *
             * plan: index probe on "O_INDEX"
             */

            TRACE( TRACE_TRX_FLOW, "App: %d DEL:ord-idx-probe-upd (%d) (%d) (%d)\n", 
                   xct_id, w_id, d_id, no_o_id);

            prord->set_value(0, no_o_id);
            prord->set_value(2, d_id);
            prord->set_value(3, w_id);

            e = _porder_man->ord_update_carrier_by_index(_pssm, prord, carrier_id);
            if (e.is_error()) { goto done; }

            int  c_id;
            prord->get_value(1, c_id);

        
            /* 4a. Calculate the total amount of the orders from orderlines */
            /* 4b. Update all the orderlines with the current timestamp */
           
            /* SELECT SUM(ol_amount) INTO :total_amount FROM order_line
             * UPDATE ORDER_LINE SET ol_delivery_d = :curr_tmstmp
             * WHERE ol_w_id = :w_id AND ol_d_id = :no_d_id AND ol_o_id = :no_o_id;
             *
             * plan: index scan on "OL_INDEX"
             */


            TRACE( TRACE_TRX_FLOW, 
                   "App: %d DEL:ol-iter-probe-by-idx (%d) (%d) (%d)\n", 
                   xct_id, w_id, d_id, no_o_id);

            int total_amount = 0;
            guard<index_scan_iter_impl<order_line_t> > ol_iter;
	    {
		index_scan_iter_impl<order_line_t>* tmp_ol_iter;
		e = _porder_line_man->ol_get_probe_iter_by_index(_pssm, tmp_ol_iter, prol, 
								 lowrep, highrep,
								 w_id, d_id, no_o_id);
		ol_iter = tmp_ol_iter;
		if (e.is_error()) { goto done; }
	    }
	    
            // iterate over all the orderlines for the particular order
            e = ol_iter->next(_pssm, eof, *prol);
            if (e.is_error()) { goto done; }
            while (!eof) {
                // update the total amount
                int current_amount;
                prol->get_value(8, current_amount);
                total_amount += current_amount;

                // update orderline
                prol->set_value(6, ts_start);
                e = _porder_line_man->update_tuple(_pssm, prol);
                if (e.is_error()) { goto done; }

                // go to the next orderline
                e = ol_iter->next(_pssm, eof, *prol);
                if (e.is_error()) { goto done; }
            }


            /* 5. Update balance of the customer of the order */

            /* UPDATE customer
             * SET c_balance = c_balance + :total_amount, c_delivery_cnt = c_delivery_cnt + 1
             * WHERE c_id = :c_id AND c_w_id = :w_id AND c_d_id = :no_d_id;
             *
             * plan: index probe on "C_INDEX"
             */

            TRACE( TRACE_TRX_FLOW, "App: %d DEL:cust-idx-probe-upd (%d) (%d) (%d)\n", 
                   xct_id, w_id, d_id, c_id);

            e = _pcustomer_man->cust_index_probe_forupdate(_pssm, prcust, 
                                                           w_id, d_id, c_id);
            if (e.is_error()) { goto done; }

            double   balance;
            prcust->get_value(16, balance);
            prcust->set_value(16, balance+total_amount);

            e = _pcustomer_man->update_tuple(_pssm, prcust);
            if (e.is_error()) { goto done; }

	    if(SPLIT_TRX && dlist.size()) {
		e = _pssm->commit_xct();
		if (e.is_error()) { goto done; }
		e = _pssm->begin_xct();
		if (e.is_error()) { goto done; }
	    }
        }

        /* 4. commit */
        e = _pssm->commit_xct();
        if (e.is_error()) { goto done; }

    } // goto

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);


#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prno->print_tuple();
    prord->print_tuple();
    prordline->print_tuple();
    prcust->print_tuple();
#endif

done:
    if(SPLIT_TRX && e.err_num() == smlevel_0::eDEADLOCK) {
	W_COERCE(_pssm->abort_xct());
	W_DO(_pssm->begin_xct());
	atomic_inc_32(&delivery_abort_ctr);
	dlist.push_back(d_id); // retry the failed trx
	goto again;
    }
    // return the tuples to the cache
    _pcustomer_man->give_tuple(prcust);
    _pnew_order_man->give_tuple(prno);
    _porder_man->give_tuple(prord);
    _porder_line_man->give_tuple(prol);
    return (e);

}; // EOF: DELIVERY




/******************************************************************** 
 *
 * TPC-C STOCK LEVEL
 *
 * Input data: w_id, d_id, threshold
 *
 * @note: Read-only transaction
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::xct_stock_level(stock_level_input_t* pslin, 
                                     const int xct_id, 
                                     trx_result_tuple_t& trt)
{
    // ensure a valid environment    
    assert (pslin);
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // stock level trx touches 3 tables: 
    // district, orderline, and stock
    row_impl<district_t>* prdist = _pdistrict_man->get_tuple();
    assert (prdist);

    row_impl<order_line_t>* prol = _porder_line_man->get_tuple();
    assert (prol);

    row_impl<stock_t>* prst = _pstock_man->get_tuple();
    assert (prst);

    w_rc_t e = RCOK;

    trt.set_id(xct_id);
    trt.set_state(UNSUBMITTED);
    rep_row_t areprow(_pdistrict_man->ts());

    // allocate space for the biggest of the 3 table representations
    areprow.set(_pdistrict_desc->maxsize()); 

    prdist->_rep = &areprow;
    prol->_rep = &areprow;
    prst->_rep = &areprow;

    { // make gotos safe
    
        /* 1. get next_o_id from the district */

        /* SELECT d_next_o_id INTO :o_id
         * FROM district
         * WHERE d_w_id = :w_id AND d_id = :d_id
         *
         * (index scan on D_INDEX)
         */

        TRACE( TRACE_TRX_FLOW, "App: %d STO:dist-idx-probe (%d) (%d)\n", 
               xct_id, pslin->_wh_id, pslin->_d_id);

        e = _pdistrict_man->dist_index_probe(_pssm, prdist, 
                                             pslin->_wh_id, pslin->_d_id);
        if (e.is_error()) { goto done; }

        int next_o_id = 0;
        prdist->get_value(10, next_o_id);


        /*
         *   SELECT COUNT(DISTRICT(s_i_id)) INTO :stock_count
         *   FROM order_line, stock
         *   WHERE ol_w_id = :w_id AND ol_d_id = :d_id
         *       AND ol_o_id < :o_id AND ol_o_id >= :o_id-20
         *       AND s_w_id = :w_id AND s_i_id = ol_i_id
         *       AND s_quantity < :threshold;
         *
         *   Plan: 1. index scan on OL_INDEX 
         *         2. sort ol tuples in the order of i_id from 1
         *         3. index scan on S_INDEX
         *         4. fetch stock with sargable on quantity from 3
         *         5. nljoin on 2 and 4
         *         6. unique on 5
         *         7. group by on 6
         */

        /* 2a. Index scan on order_line table. */

        TRACE( TRACE_TRX_FLOW, "App: %d STO:ol-iter-by-idx (%d) (%d) (%d) (%d)\n", 
               xct_id, pslin->_wh_id, pslin->_d_id, next_o_id-20, next_o_id);
   
        rep_row_t lowrep(_porder_line_man->ts());
        rep_row_t highrep(_porder_line_man->ts());
        rep_row_t sortrep(_porder_line_man->ts());
        // allocate space for the biggest of the (new_order) and (orderline)
        // table representations
        lowrep.set(_porder_line_desc->maxsize()); 
        highrep.set(_porder_line_desc->maxsize()); 
        sortrep.set(_porder_line_desc->maxsize()); 
    

        guard<index_scan_iter_impl<order_line_t> > ol_iter;
	{
	    index_scan_iter_impl<order_line_t>* tmp_ol_iter;
	    e = _porder_line_man->ol_get_range_iter_by_index(_pssm, tmp_ol_iter, prol,
							     lowrep, highrep,
							     pslin->_wh_id, pslin->_d_id,
							     next_o_id-20, next_o_id);
	    ol_iter = tmp_ol_iter;
	    if (e.is_error()) { goto done; }
	}

        sort_buffer_t ol_list(4);
        ol_list.setup(0, SQL_INT);  /* OL_I_ID */
        ol_list.setup(1, SQL_INT);  /* OL_W_ID */
        ol_list.setup(2, SQL_INT);  /* OL_D_ID */
        ol_list.setup(3, SQL_INT);  /* OL_O_ID */
        sort_man_impl ol_sorter(&ol_list, &sortrep, 2);
        row_impl<sort_buffer_t> rsb(&ol_list);

        // iterate over all selected orderlines and add them to the sorted buffer
        bool eof;
        e = ol_iter->next(_pssm, eof, *prol);
        while (!eof) {

            if (e.is_error()) { goto done; }

            /* put the value into the sorted buffer */
            int temp_oid, temp_iid;
            int temp_wid, temp_did;        

            prol->get_value(4, temp_iid);
            prol->get_value(0, temp_oid);
            prol->get_value(2, temp_wid);
            prol->get_value(1, temp_did);

            rsb.set_value(0, temp_iid);
            rsb.set_value(1, temp_wid);
            rsb.set_value(2, temp_did);
            rsb.set_value(3, temp_oid);

            ol_sorter.add_tuple(rsb);

            //         TRACE( TRACE_TRX_FLOW, "App: %d STO:ol-iter-next (%d) (%d) (%d) (%d)\n", 
            //                xct_id, temp_wid, temp_did, temp_oid, temp_iid);
  
            e = ol_iter->next(_pssm, eof, *prol);
        }
        assert (ol_sorter.count());

        /* 2b. Sort orderline tuples on i_id */
        sort_iter_impl ol_list_sort_iter(_pssm, &ol_list, &ol_sorter);
        int last_i_id = -1;
        int count = 0;

        /* 2c. Nested loop join order_line with stock */
        e = ol_list_sort_iter.next(_pssm, eof, rsb);
        if (e.is_error()) { goto done; }
        while (!eof) {

            /* use the index to find the corresponding stock tuple */
            int i_id;
            int w_id;

            rsb.get_value(0, i_id);
            rsb.get_value(1, w_id);

            //         TRACE( TRACE_TRX_FLOW, "App: %d STO:st-idx-probe (%d) (%d)\n", 
            //                xct_id, w_id, i_id);

            // 2d. Index probe the Stock
            e = _pstock_man->st_index_probe(_pssm, prst, w_id, i_id);
            if (e.is_error()) { goto done; }

            // check if stock quantity below threshold 
            int quantity;
            prst->get_value(3, quantity);

            if (quantity < pslin->_threshold) {
                /* Do join on the two tuples */

                /* the work is to count the number of unique item id. We keep
                 * two pieces of information here: the last item id and the
                 * current count.  This is enough because the item id is in
                 * increasing order.
                 */
                if (last_i_id != i_id) {
                    last_i_id = i_id;
                    count++;
                }
            
                TRACE( TRACE_TRX_FLOW, "App: %d STO:found-one (%d) (%d) (%d)\n", 
                       xct_id, count, i_id, quantity);

            }

            e = ol_list_sort_iter.next(_pssm, eof, rsb);
            if (e.is_error()) { goto done; }
        }
  
        /* 3. commit */
        e = _pssm->commit_xct();
        if (e.is_error()) { goto done; }

    } // goto
 
    // if we reached this point everything went ok
    trt.set_state(COMMITTED);

#ifdef PRINT_TRX_RESULTS
        // at the end of the transaction 
        // dumps the status of all the table rows used
        rdist.print_tuple();
        rordline.print_tuple();
        rstock.print_tuple();
#endif

done:    
    // return the tuples to the cache
    _pdistrict_man->give_tuple(prdist);
    _pstock_man->give_tuple(prst);
    _porder_line_man->give_tuple(prol);
    return (e);
}



/******************************************************************** 
 *
 * MBENCHES
 *
 ********************************************************************/

w_rc_t ShoreTPCCEnv::run_mbench_cust(const int xct_id, trx_result_tuple_t& atrt, 
                                     const int whid)
{
    TRACE( TRACE_TRX_FLOW, "%d. MBENCH-CUST...\n", xct_id);     
    ++my_stats.attempted.other;
    w_rc_t e = _run_mbench_cust(xct_id, atrt, whid);
    if (e.is_error()) {
        ++my_stats.failed.other;
        TRACE( TRACE_TRX_FLOW, "Xct (%d) MBench-Cust aborted [0x%x]\n", 
               xct_id, e.err_num());
	
	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) MBench-Cust failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

	// signal cond var
	if(atrt.get_notify())
	    atrt.get_notify()->signal();

        if (_measure!=MST_MEASURE) {
            return (e);
        }

//        _total_tpcc_stats.inc_other_com();
//        _session_tpcc_stats.inc_other_com();
        _env_stats.inc_trx_att();
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) MBench-Cust completed\n", xct_id);

    // signal cond var
    if(atrt.get_notify())
        atrt.get_notify()->signal();

    if (_measure!=MST_MEASURE) {
        return (RCOK);
    }

//    _total_tpcc_stats.inc_other_com();
//    _session_tpcc_stats.inc_other_com();
    _env_stats.inc_trx_com();
    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::_run_mbench_cust(const int xct_id, trx_result_tuple_t& trt, 
                                      int whid)
{
    // ensure a valid environment    
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // pick a valid wh id
    if (whid==0) 
        whid = URand(1,_scaling_factor); 

    // generates the input
    int did = URand(1,10);
    int cid = NURand(1023,1,3000);
    double amount = (double)URand(1,1000);
    // mbench trx touches 1 table: 
    // customer

    row_impl<customer_t>* prcust = _pcustomer_man->get_tuple();
    assert (prcust);
    
    w_rc_t e = RCOK;

    trt.set_id(xct_id);
    trt.set_state(UNSUBMITTED);
    rep_row_t areprow(_pcustomer_man->ts());

    // allocate space for the table representation
    areprow.set(_pcustomer_desc->maxsize()); 
    prcust->_rep = &areprow;

    { // make gotos safe

        /* 1. retrieve customer for update */

        /* SELECT c_first, c_middle, c_last, c_street_1, c_street_2, c_city, 
         * c_state, c_zip, c_phone, c_since, c_credit, c_credit_lim, 
         * c_discount, c_balance, c_ytd_payment, c_payment_cnt 
         * FROM customer 
         * WHERE c_id = :c_id AND c_w_id = :c_w_id AND c_d_id = :c_d_id 
         * FOR UPDATE OF c_balance, c_ytd_payment, c_payment_cnt
         *
         * plan: index probe on "C_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, 
               "App: %d PAY:cust-idx-upd (%d) (%d) (%d)\n", 
               xct_id, whid, did, cid);

        e = _pcustomer_man->cust_index_probe_forupdate(_pssm, prcust, 
                                                       whid, did, cid);
        if (e.is_error()) { goto done; }


        double c_balance, c_ytd_payment;
        int    c_payment_cnt;
        tpcc_customer_tuple acust;

        // retrieve customer
        prcust->get_value(3,  acust.C_FIRST, 17);
        prcust->get_value(4,  acust.C_MIDDLE, 3);
        prcust->get_value(5,  acust.C_LAST, 17);
        prcust->get_value(6,  acust.C_STREET_1, 21);
        prcust->get_value(7,  acust.C_STREET_2, 21);
        prcust->get_value(8,  acust.C_CITY, 21);
        prcust->get_value(9,  acust.C_STATE, 3);
        prcust->get_value(10, acust.C_ZIP, 10);
        prcust->get_value(11, acust.C_PHONE, 17);
        prcust->get_value(12, acust.C_SINCE);
        prcust->get_value(13, acust.C_CREDIT, 3);
        prcust->get_value(14, acust.C_CREDIT_LIM);
        prcust->get_value(15, acust.C_DISCOUNT);
        prcust->get_value(16, acust.C_BALANCE);
        prcust->get_value(17, acust.C_YTD_PAYMENT);
        prcust->get_value(18, acust.C_LAST_PAYMENT);
        prcust->get_value(19, acust.C_PAYMENT_CNT);
        prcust->get_value(20, acust.C_DATA_1, 251);
        prcust->get_value(21, acust.C_DATA_2, 251);

        // update customer fields
        acust.C_BALANCE -= amount;
        acust.C_YTD_PAYMENT += amount;
        acust.C_PAYMENT_CNT++;

        // if bad customer
        if (acust.C_CREDIT[0] == 'B' && acust.C_CREDIT[1] == 'C') { 
            /* 10% of customers */

            /* SELECT c_data
             * FROM customer 
             * WHERE c_id = :c_id AND c_w_id = :c_w_id AND c_d_id = :c_d_id
             * FOR UPDATE OF c_balance, c_ytd_payment, c_payment_cnt, c_data
             *
             * plan: index probe on "C_INDEX"
             */

            // update the data
            char c_new_data_1[251];
            char c_new_data_2[251];
            sprintf(c_new_data_1, "%d,%d,%d,%d,%d,%1.2f",
                    cid, did, whid, did, whid, amount);

            int len = strlen(c_new_data_1);
            strncat(c_new_data_1, acust.C_DATA_1, 250-len);
            strncpy(c_new_data_2, &acust.C_DATA_1[250-len], len);
            strncpy(c_new_data_2, acust.C_DATA_2, 250-len);

            TRACE( TRACE_TRX_FLOW, "App: %d PAY:bad-cust-upd-tuple\n", xct_id);
            e = _pcustomer_man->cust_update_tuple(_pssm, prcust, acust, 
                                                  c_new_data_1, c_new_data_2);
            if (e.is_error()) { goto done; }
        }
        else { /* good customer */
            TRACE( TRACE_TRX_FLOW, "App: %d PAY:good-cust-upd-tuple\n", xct_id);
            e = _pcustomer_man->cust_update_tuple(_pssm, prcust, acust, 
                                                  NULL, NULL);
            if (e.is_error()) { goto done; }
        }

        /* 4. commit */
        e = _pssm->commit_xct();
        if (e.is_error()) { goto done; }

    } // goto

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);


#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prcust->print_tuple();
#endif


done:
    // give back the tuples
    _pcustomer_man->give_tuple(prcust);
    return (e);

} // EOF: MBENCH-CUST


w_rc_t ShoreTPCCEnv::run_mbench_wh(const int xct_id, trx_result_tuple_t& atrt, 
                                   const int whid)
{
    TRACE( TRACE_TRX_FLOW, "%d. MBENCH-WH...\n", xct_id);     

    ++my_stats.attempted.other;
    w_rc_t e = _run_mbench_wh(xct_id, atrt, whid);
    if (e.is_error()) {
        ++my_stats.failed.other;
        TRACE( TRACE_TRX_FLOW, "Xct (%d) MBench-Wh aborted [0x%x]\n", 
               xct_id, e.err_num());
	
	w_rc_t e2 = _pssm->abort_xct();
	if(e2.is_error()) {
	    TRACE( TRACE_ALWAYS, "Xct (%d) MBench-Wh failed [0x%x]\n", 
		   xct_id, e2.err_num());
	}

	// signal cond var
	if(atrt.get_notify())
	    atrt.get_notify()->signal();

        if (_measure!=MST_MEASURE) {
            return (e);
        }

//        _total_tpcc_stats.inc_other_com();
//        _session_tpcc_stats.inc_other_com();
        _env_stats.inc_trx_att();
        return (e);
    }

    TRACE( TRACE_TRX_FLOW, "Xct (%d) MBench-Wh completed\n", xct_id);

    // signal cond var
    if(atrt.get_notify())
        atrt.get_notify()->signal();

    if (_measure!=MST_MEASURE) {
        return (RCOK);
    }

//    _total_tpcc_stats.inc_other_com();
//    _session_tpcc_stats.inc_other_com();
    _env_stats.inc_trx_com();    
    return (RCOK); 
}

w_rc_t ShoreTPCCEnv::_run_mbench_wh(const int xct_id, trx_result_tuple_t& trt, 
                                    int whid)
{
    // ensure a valid environment    
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // pick a valid wh id
    if (whid==0) 
        whid = URand(1,_scaling_factor); 

    // generate the input
    double amount = (double)URand(1,1000);

    // mbench trx touches 1 table: 
    // warehouse

    // get table tuples from the caches
    row_impl<warehouse_t>* prwh = _pwarehouse_man->get_tuple();
    assert (prwh);

    w_rc_t e = RCOK;

    trt.set_id(xct_id);
    trt.set_state(UNSUBMITTED);
    rep_row_t areprow(_pwarehouse_man->ts());

    // allocate space for the biggest of the 4 table representations
    areprow.set(_pwarehouse_desc->maxsize()); 
    prwh->_rep = &areprow;

    { // make gotos safe

        /* 1. retrieve warehouse for update */
        TRACE( TRACE_TRX_FLOW, 
               "App: %d PAY:wh-idx-upd (%d)\n", 
               xct_id, whid);

        e = _pwarehouse_man->wh_index_probe_forupdate(_pssm, prwh, whid);
        if (e.is_error()) { goto done; }


        /* UPDATE warehouse SET w_ytd = wytd + :h_amount
         * WHERE w_id = :w_id
         *
         * SELECT w_name, w_street_1, w_street_2, w_city, w_state, w_zip
         * FROM warehouse
         * WHERE w_id = :w_id
         *
         * plan: index probe on "W_INDEX"
         */

        TRACE( TRACE_TRX_FLOW, "App: %d PAY:wh-update-ytd (%d)\n", xct_id, whid);
        e = _pwarehouse_man->wh_update_ytd(_pssm, prwh, amount);
        if (e.is_error()) { goto done; }

        tpcc_warehouse_tuple awh;
        prwh->get_value(1, awh.W_NAME, 11);
        prwh->get_value(2, awh.W_STREET_1, 21);
        prwh->get_value(3, awh.W_STREET_2, 21);
        prwh->get_value(4, awh.W_CITY, 21);
        prwh->get_value(5, awh.W_STATE, 3);
        prwh->get_value(6, awh.W_ZIP, 10);

        /* 4. commit */
        e = _pssm->commit_xct();
        if (e.is_error()) { goto done; }

    } // goto

    // if we reached this point everything went ok
    trt.set_state(COMMITTED);

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the status of all the table rows used
    prwh->print_tuple();
#endif

done:
    // give back the tuples
    _pwarehouse_man->give_tuple(prwh);
    return (e);

} // EOF: MBENCH-WH



EXIT_NAMESPACE(tpcc);
