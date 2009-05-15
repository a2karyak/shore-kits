/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_stock_level.cpp
 *
 *  @brief:  DORA TPC-C STOCK LEVEL
 *
 *  @note:   Implementation of RVPs and Actions that synthesize 
 *           the TPC-C Stock Level trx according to DORA
 *
 *  @author: Ippokratis Pandis, Jan 2009
 */

#include "dora/tpcc/dora_stock_level.h"
#include "dora/tpcc/dora_tpcc.h"


using namespace dora;
using namespace shore;
using namespace tpcc;



ENTER_NAMESPACE(dora);


//#define PRINT_TRX_RESULTS

//
// RVPS
//
// (1) mid1_stock_rvp
// (2) mid2_stock_rvp
// (3) final_stock_rvp
//


DEFINE_DORA_FINAL_RVP_CLASS(final_stock_rvp,stock_level);



/******************************************************************** 
 *
 * STOCK LEVEL MID-1 RVP - enqueues the IT(OL) action
 *
 ********************************************************************/

w_rc_t mid1_stock_rvp::run() 
{
    // 1. Setup the next RVP
#ifndef ONLYDORA
    assert (_xct);
#endif
    mid2_stock_rvp* rvp2 = _penv->new_mid2_stock_rvp(_tid,_xct,_xct_id,_result,_in,_actions,_bWake);

    // 2. Generate and enqueue action
    r_ol_stock_action* r_ol_stock = _penv->new_r_ol_stock_action(_tid,_xct,rvp2,_in);

    TRACE( TRACE_TRX_FLOW, "Next phase (%d)\n", _tid);
    typedef range_partition_impl<int>   irpImpl; 

    {
        irpImpl* ol_part = _penv->oli()->myPart(_in._wh_id-1);

        // OLI_PART_CS
        CRITICAL_SECTION(oli_part_cs, ol_part->_enqueue_lock);
        if (ol_part->enqueue(r_ol_stock,_bWake)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing R_OL_STOCK\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
        return (RCOK);
    }
}



/******************************************************************** 
 *
 * STOCK LEVEL MID-2 RVP - enqueues the R(ST) --join-- action
 *
 ********************************************************************/

w_rc_t mid2_stock_rvp::run() 
{
    // 1. Setup the next RVP
#ifndef ONLYDORA
    assert (_xct);
#endif

    // 1. Set the final RVP
    final_stock_rvp* frvp = _penv->new_final_stock_rvp(_tid,_xct,_xct_id,_result,_actions);

    // 2. Generate the action
    r_st_stock_action* r_st = _penv->new_r_st_stock_action(_tid,_xct,frvp,_in);

    TRACE( TRACE_TRX_FLOW, "Next phase (%d)\n", _tid);
    typedef range_partition_impl<int>   irpImpl; 

    { 
        irpImpl* my_st_part = _penv->sto()->myPart(_in._wh_id-1);

        // STO_PART_CS
        CRITICAL_SECTION(sto_part_cs, my_st_part->_enqueue_lock);
        if (my_st_part->enqueue(r_st,_bWake)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing R_ST_STOCK\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }
    return (RCOK);
}


/******************************************************************** 
 *
 * STOCK LEVEL TPC-C DORA ACTIONS
 *
 * (1) READ-DISTRICT (next_o_id)
 * (2) READ-ORDERLINES
 * (3) JOIN-STOCKS
 *
 ********************************************************************/

void r_dist_stock_action::calc_keys()
{
    set_read_only();
    _down.push_back(_in._wh_id);
    _down.push_back(_in._d_id);
    _up.push_back(_in._wh_id);
    _up.push_back(_in._d_id);
}

w_rc_t r_dist_stock_action::trx_exec() 
{
    assert (_penv);

    // get table tuple from the cache
    row_impl<district_t>* prdist = _penv->district_man()->get_tuple();
    assert (prdist);

    rep_row_t areprow(_penv->district_man()->ts());
    areprow.set(_penv->district_desc()->maxsize()); 
    prdist->_rep = &areprow;

    w_rc_t e = RCOK;

    register int w_id = _in._wh_id;
    register int d_id = _in._d_id;

    { // make gotos safe
    
        /* 1. get next_o_id from the district */

        /* SELECT d_next_o_id INTO :o_id
         * FROM district
         * WHERE d_w_id = :w_id AND d_id = :d_id
         *
         * (index scan on D_INDEX)
         */

        TRACE( TRACE_TRX_FLOW, "App: %d STO:dist-idx-probe (%d) (%d)\n", 
               _tid, w_id, d_id);

#ifndef ONLYDORA
        e = _penv->district_man()->dist_index_probe_nl(_penv->db(), prdist, w_id, d_id);
#endif

        if (e.is_error()) { goto done; }

        // pass the data to the RVP
        int next_o_id = 0;
        prdist->get_value(10, next_o_id);
        
        _prvp->_in._next_o_id = next_o_id;

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the level of all the table rows used
    prdist->print_tuple();
#endif

done:
    // give back the tuple
    _penv->district_man()->give_tuple(prdist);
    return (e);
}



#warning Only 2 fields (WH,DI) determine the ORDERLINE table accesses! Not 3.

void r_ol_stock_action::calc_keys()
{
    set_read_only();
    _down.push_back(_in._wh_id);
    _down.push_back(_in._d_id);
    _up.push_back(_in._wh_id);
    _up.push_back(_in._d_id);
}

w_rc_t r_ol_stock_action::trx_exec() 
{
    assert (_penv);

    // get table tuple from the cache

    row_impl<order_line_t>* prol = _penv->order_line_man()->get_tuple();
    assert (prol);

    rep_row_t areprow(_penv->order_line_man()->ts());
    areprow.set(_penv->order_line_desc()->maxsize()); 
    prol->_rep = &areprow;

    rep_row_t lowrep(_penv->order_line_man()->ts());
    lowrep.set(_penv->order_line_desc()->maxsize()); 

    rep_row_t highrep(_penv->order_line_man()->ts());
    highrep.set(_penv->order_line_desc()->maxsize()); 

    rep_row_t sortrep(_penv->order_line_man()->ts());
    sortrep.set(_penv->order_line_desc()->maxsize()); 

    sort_buffer_t ol_list(4);
    ol_list.setup(0, SQL_INT);  /* OL_I_ID */
    ol_list.setup(1, SQL_INT);  /* OL_W_ID */
    ol_list.setup(2, SQL_INT);  /* OL_D_ID */
    ol_list.setup(3, SQL_INT);  /* OL_O_ID */
    sort_man_impl ol_sorter(&ol_list, &sortrep, 2);
    row_impl<sort_buffer_t> rsb(&ol_list);

    w_rc_t e = RCOK;
    bool eof;

    register int w_id = _in._wh_id;
    register int d_id = _in._d_id;
    register int i_id = -1;

    { // make gotos safe 

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
               _tid, w_id, d_id, _in._next_o_id-20, _in._next_o_id);
    
#ifndef ONLYDORA
        guard<index_scan_iter_impl<order_line_t> > ol_iter;
	{
	    index_scan_iter_impl<order_line_t>* tmp_ol_iter;
	    e = _penv->order_line_man()->ol_get_range_iter_by_index_nl(_penv->db(), 
                                                                          tmp_ol_iter, prol,
                                                                          lowrep, highrep,
                                                                          w_id, d_id,
                                                                          _in._next_o_id-20, _in._next_o_id);
	    ol_iter = tmp_ol_iter;
	    if (e.is_error()) { goto done; }
	}

        // iterate over all selected orderlines and add them to the sorted buffer

        e = ol_iter->next(_penv->db(), eof, *prol);
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

            TRACE( TRACE_TRX_FLOW, "App: %d STO:ol-iter-next (%d) (%d) (%d) (%d)\n", 
                   _tid, temp_wid, temp_did, temp_oid, temp_iid);
  
            e = ol_iter->next(_penv->db(), eof, *prol);
        }
        assert (ol_sorter.count());
#endif

        /* 2b. Sort orderline tuples on i_id */
        sort_iter_impl ol_list_sort_iter(_penv->db(), &ol_list, &ol_sorter);
        int last_i_id = -1;
        int count = 0;

        /* 2c. Load the vector with pairs of w_id, and i_it notify rvp */
        assert (_prvp->_in._pvwi == NULL);
        _prvp->_in._pvwi = new TwoIntVec();       
        assert (_prvp->_in._pvwi);
        _prvp->_in._pvwi->reserve(ol_sorter.count());

        e = ol_list_sort_iter.next(_penv->db(), eof, rsb);
        if (e.is_error()) { goto done; }
        
        while (!eof) {

            /* use the index to find the corresponding stock tuple */
            rsb.get_value(0, i_id);
            rsb.get_value(1, w_id);

            TRACE( TRACE_TRX_FLOW, "App: %d STO:st-idx-probe (%d) (%d)\n", 
                   _tid, w_id, i_id);
            
            // add pair to vector
            _prvp->_in._pvwi->push_back(pair<int,int>(w_id,i_id));            
            
            e = ol_list_sort_iter.next(_penv->db(), eof, rsb);
            if (e.is_error()) { goto done; }
        }

    } // goto 

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the level of all the table rows used
    prol->print_tuple();
#endif

done:
    // give back the tuple
    _penv->order_line_man()->give_tuple(prol);   
    return (e);
}


#warning Only 1 field (WH) determines the STOCK table accesses! Not 2.

void r_st_stock_action::calc_keys()
{
    set_read_only();
    _down.push_back(_in._wh_id);
    _up.push_back(_in._wh_id);
}

w_rc_t r_st_stock_action::trx_exec() 
{
    assert (_penv);

    // get table tuple from the cache
    row_impl<stock_t>* prst = _penv->stock_man()->get_tuple();
    assert (prst);

    rep_row_t areprow(_penv->stock_man()->ts());
    areprow.set(_penv->stock_desc()->maxsize()); 
    prst->_rep = &areprow;

    w_rc_t e = RCOK;

    int input_w_id = _in._wh_id;

    register int w_id = 0;
    register int d_id = 0;
    register int i_id = 0;
    register int quantity = 0;

    int last_i_id = -1;
    int count = 0;

    { // make gotos safe


        /* 2c. Nested loop join order_line with stock */
        assert (_in._pvwi);
        for (TwoIntVecIt it = _in._pvwi->begin(); it != _in._pvwi->end(); ++it) {

            /* use the index to find the corresponding stock tuple */
            w_id = (*it).first;
            i_id = (*it).second;

            // ensures that all the probed stocks belong to the same warehouse
            assert (input_w_id == w_id); 

            TRACE( TRACE_TRX_FLOW, "App: %d STO:st-idx-probe-nl (%d) (%d)\n", 
                   _tid, w_id, i_id);

            // 2d. Index probe the Stock
#ifndef ONLYDORA
            e = _penv->stock_man()->st_index_probe_nl(_penv->db(), prst, w_id, i_id);
#endif
            if (e.is_error()) { goto done; }

            // check if stock quantity below threshold 
            prst->get_value(3, quantity);

            if (quantity < _in._threshold) {
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
            
                TRACE( TRACE_TRX_FLOW, "App: %d STO:found-one (%d) (%d) (%d) (%d)\n", 
                       _tid, w_id, i_id, quantity, count);

            }
        }

    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction 
    // dumps the level of all the table rows used
    prst->print_tuple();
#endif

done:
    // delete the pvwi
    if (_in._pvwi) { delete (_in._pvwi); _in._pvwi = NULL; }

    // give back the tuple
    _penv->stock_man()->give_tuple(prst);

    return (e);
}




EXIT_NAMESPACE(dora);
