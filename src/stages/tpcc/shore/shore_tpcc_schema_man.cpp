/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_schema.h
 *
 *  @brief:  Implementation of the workload-specific access methods 
 *           on TPC-C tables
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "stages/tpcc/shore/shore_tpcc_schema_man.h"


using namespace shore;
using namespace tpcc;


/*********************************************************************
 *
 * Workload-specific access methods on tables
 *
 *********************************************************************/


/* ----------------- */
/* --- WAREHOUSE --- */
/* ----------------- */


w_rc_t warehouse_man_impl::index_probe(ss_m* db,
                                       warehouse_tuple* ptuple,
                                       const int w_id)
{
    ptuple->set_value(0, w_id);
    return (index_probe_by_name(db, "W_INDEX", ptuple));
}

w_rc_t warehouse_man_impl::index_probe_forupdate(ss_m* db,
                                                 warehouse_tuple* ptuple,
                                                 const int w_id)
{
    ptuple->set_value(0, w_id);
    return (index_probe_forupdate_by_name(db, "W_INDEX", ptuple));
}

w_rc_t warehouse_man_impl::update_ytd(ss_m* db,
                                      warehouse_tuple* ptuple,
                                      const int w_id,
                                      const double amount)
{
    // 1. idx probes for update the warehouse
    // 2. increases the ytd of the warehouse and updates the table

    ptuple->set_value(0, w_id);
    W_DO(index_probe_forupdate_by_name(db, "W_INDEX", ptuple));

    double ytd;
    ptuple->get_value(8, ytd);
    ytd += amount;
    ptuple->set_value(8, ytd);
    W_DO(update_tuple(db, ptuple));
    return (RCOK);
}



/* ---------------- */
/* --- DISTRICT --- */
/* ---------------- */


w_rc_t district_man_impl::index_probe(ss_m* db,
                                      district_tuple* ptuple,
                                      const int d_id,
                                      const int w_id)
{
    ptuple->set_value(0, d_id);
    ptuple->set_value(1, w_id);
    return (index_probe_by_name(db, "D_INDEX", ptuple));
}

w_rc_t district_man_impl::index_probe_forupdate(ss_m* db,
                                                district_tuple* ptuple,
                                                const int d_id,
                                                const int w_id)
{
    ptuple->set_value(0, d_id);
    ptuple->set_value(1, w_id);
    return (index_probe_forupdate_by_name(db, "D_INDEX", ptuple));
}

w_rc_t district_man_impl::update_ytd(ss_m* db,
                                     district_tuple* ptuple,
                                     const int d_id,
                                     const int w_id,
                                     const double amount)
{
    // 1. idx probes for update the district
    // 2. increases the ytd of the district and updates the table

    ptuple->set_value(0, d_id);
    ptuple->set_value(1, w_id);
    W_DO(index_probe_forupdate_by_name(db, "D_INDEX", ptuple));

    double d_ytd;
    ptuple->get_value(9, d_ytd);
    d_ytd += amount;
    ptuple->set_value(9, d_ytd);
    W_DO(update_tuple(db, ptuple));
    return (RCOK);
}

w_rc_t district_man_impl::update_next_o_id(ss_m* db,
                                           district_tuple* ptuple,
                                           const int next_o_id)
{
    ptuple->set_value(10, next_o_id);
    return (update_tuple(db, ptuple));
}



/* ---------------- */
/* --- CUSTOMER --- */
/* ---------------- */


w_rc_t customer_man_impl::get_iter_by_index(ss_m* db,
                                            customer_index_iter* &iter,
                                            customer_tuple* ptuple,
                                            rep_row_t &replow,
                                            rep_row_t &rephigh,
                                            const int w_id,
                                            const int d_id,
                                            const char* c_last)
{
    assert (_ptable);
    index_desc_t* pindex = _ptable->find_index("C_NAME_INDEX");
    assert (pindex);

    // C_NAME_INDEX: {2 - 1 - 5 - 3 - 0}

    // prepare the key to be probed
    ptuple->set_value(0, 0);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    ptuple->set_value(3, "");
    ptuple->set_value(5, c_last);

    int lowsz = format_key(pindex, ptuple, replow);
    assert (replow._dest);

    char   temp[2];
    temp[0] = MAX('z', 'Z')+1;
    temp[1] = '\0';
    ptuple->set_value(3, temp);

    int highsz = format_key(pindex, ptuple, rephigh);
    assert (rephigh._dest);    

    /* index only access */
    W_DO(get_iter_for_index_scan(db, pindex, iter,
				 scan_index_i::ge, vec_t(replow._dest, lowsz),
				 scan_index_i::lt, vec_t(rephigh._dest, highsz),
				 false));
    return (RCOK);
}

w_rc_t customer_man_impl::index_probe(ss_m* db,
                                      customer_tuple* ptuple,
                                      const int c_id,
                                      const int w_id,
                                      const int d_id)
{
    ptuple->set_value(0, c_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    return (index_probe_by_name(db, "C_INDEX", ptuple));
}

w_rc_t customer_man_impl::index_probe_forupdate(ss_m * db,
                                                customer_tuple* ptuple,
                                                const int c_id,
                                                const int w_id,
                                                const int d_id)
{
    ptuple->set_value(0, c_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    return (index_probe_forupdate_by_name(db, "C_INDEX", ptuple));
}

w_rc_t customer_man_impl::update_tuple(ss_m* db,
                                       customer_tuple* ptuple,
                                       const tpcc_customer_tuple acustomer,
                                       const char* adata1,
                                       const char* adata2)
{
    ptuple->set_value(16, acustomer.C_BALANCE);
    ptuple->set_value(17, acustomer.C_YTD_PAYMENT);
    ptuple->set_value(19, acustomer.C_PAYMENT_CNT);

    if (adata1)
	ptuple->set_value(20, adata1);

    if (adata2)
	ptuple->set_value(21, adata2);

    return (table_man_impl<customer_t>::update_tuple(db, ptuple));
}



/* ----------------- */
/* --- NEW_ORDER --- */
/* ----------------- */

				    
w_rc_t new_order_man_impl::get_iter_by_index(ss_m* db,
                                             new_order_index_iter* &iter,
                                             new_order_tuple* ptuple,
                                             rep_row_t &replow,
                                             rep_row_t &rephigh,
                                             const int w_id,
                                             const int d_id)
{
    /* find the index structure */
    assert (_ptable);
    index_desc_t* pindex = _ptable->find_index("NO_INDEX");
    assert (pindex);

    /* get the lowest key value */
    ptuple->set_value(0, 0);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);

    int lowsz = format_key(pindex, ptuple, replow);
    assert (replow._dest);

    /* get the highest key value */
    ptuple->set_value(1, d_id+1);

    int highsz = format_key(pindex, ptuple, rephigh);
    assert (rephigh._dest);

    /* get the tuple iterator (index only scan) */
    W_DO(get_iter_for_index_scan(db, pindex, iter,
				 scan_index_i::ge, vec_t(replow._dest, lowsz),
				 scan_index_i::lt, vec_t(rephigh._dest, highsz)));
    return (RCOK);
}

w_rc_t new_order_man_impl::delete_by_index(ss_m* db,
                                           new_order_tuple* ptuple,
                                           const int w_id,
                                           const int d_id,
                                           const int o_id)
{
    // 1. idx probe for new_order
    // 2. deletes the found new_order

    ptuple->set_value(0, o_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    W_DO(index_probe_by_name(db, "NO_INDEX", ptuple));
    W_DO(delete_tuple(db, ptuple));

    return (RCOK);
}



/* ------------- */
/* --- ORDER --- */
/* ------------- */


w_rc_t order_man_impl::update_carrier_by_index(ss_m* db,
                                        order_tuple* ptuple,
                                        const int carrier_id)
{
    // 1. idx probe for update the order
    // 2. update carrier_id and update table

    W_DO(index_probe_forupdate_by_name(db, "O_INDEX", ptuple));

    ptuple->set_value(5, carrier_id);
    W_DO(update_tuple(db, ptuple));

    return (RCOK);
}

w_rc_t order_man_impl::get_iter_by_index(ss_m* db,
                                         order_index_iter* &iter,
                                         order_tuple* ptuple,
                                         rep_row_t &replow,
                                         rep_row_t &rephigh,
                                         const int w_id,
                                         const int d_id,
                                         const int c_id)
{
    assert (_ptable);
    index_desc_t* pindex = _ptable->find_index("O_CUST_INDEX");
    assert (pindex);

    /* get the lowest key value */
    ptuple->set_value(0, 0);
    ptuple->set_value(1, c_id);
    ptuple->set_value(2, d_id);
    ptuple->set_value(3, w_id);

    int lowsz = format_key(pindex, ptuple, replow);
    assert (replow._dest);

    /* get the highest key value */
    ptuple->set_value(1, c_id+1);

    int highsz  = format_key(pindex, ptuple, rephigh);
    assert (rephigh._dest);

    W_DO(get_iter_for_index_scan(db, pindex, iter,
				 scan_index_i::ge, vec_t(replow._dest, lowsz),
				 scan_index_i::lt, vec_t(rephigh._dest, highsz),
				 true));
    return (RCOK);
}



/* ----------------- */
/* --- ORDERLINE --- */
/* ----------------- */


w_rc_t order_line_man_impl::get_iter_by_index(ss_m* db,
                                              order_line_index_iter* &iter,
                                              order_line_tuple* ptuple,
                                              rep_row_t &replow,
                                              rep_row_t &rephigh,
                                              const int w_id,
                                              const int d_id,
                                              const int low_o_id,
                                              const int high_o_id)
{
    /* pointer to the index */
    assert (_ptable);
    index_desc_t* pindex = _ptable->find_index("OL_INDEX");
    assert (pindex);

    /* get the lowest key value */
    ptuple->set_value(0, low_o_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    ptuple->set_value(3, (int)0);  /* assuming that ol_number starts from 1 */

    int lowsz = format_key(pindex, ptuple, replow);
    assert (replow._dest);

    /* get the highest key value */
    ptuple->set_value(0, high_o_id+1);

    int highsz = format_key(pindex, ptuple, rephigh);
    assert (rephigh._dest);
    
    /* get the tuple iterator (not index only scan) */
    W_DO(get_iter_for_index_scan(db, pindex, iter,
				 scan_index_i::ge, vec_t(replow._dest, lowsz),
				 scan_index_i::lt, vec_t(rephigh._dest, highsz),
				 true));
    return (RCOK);
}


w_rc_t order_line_man_impl::get_iter_by_index(ss_m* db,
                                              order_line_index_iter* &iter,
                                              order_line_tuple* ptuple,
                                              rep_row_t &replow,
                                              rep_row_t &rephigh,
                                              const int w_id,
                                              const int d_id,
                                              const int o_id)
{
    assert (_ptable);
    index_desc_t* pindex = _ptable->find_index("OL_INDEX");
    assert (pindex);

    ptuple->set_value(0, o_id);
    ptuple->set_value(1, d_id);
    ptuple->set_value(2, w_id);
    ptuple->set_value(3, 0);

    int lowsz = format_key(pindex, ptuple, replow);
    assert (replow._dest);

    /* get the highest key value */
    ptuple->set_value(0, o_id+1);

    int highsz = format_key(pindex, ptuple, rephigh);
    assert (rephigh._dest);

    W_DO(get_iter_for_index_scan(db, pindex, iter,
				 scan_index_i::ge,
				 vec_t(replow._dest, lowsz),
				 scan_index_i::lt,
				 vec_t(rephigh._dest, highsz),
				 true));
    return (RCOK);
}



/* ------------ */
/* --- ITEM --- */
/* ------------ */


w_rc_t item_man_impl::index_probe(ss_m* db, 
                                  item_tuple* ptuple,
                                  const int i_id)
{
    assert (_ptable);
    index_desc_t* pindex = _ptable->find_index("I_INDEX");
    assert (pindex);
    ptuple->set_value(0, i_id);
    return (table_man_impl<item_t>::index_probe(db, pindex, ptuple));
}

w_rc_t item_man_impl::index_probe_forupdate(ss_m* db, 
                                            item_tuple* ptuple,
                                            const int i_id)
{
    assert (_ptable);
    index_desc_t* pindex = _ptable->find_index("I_INDEX");
    assert (pindex);
    ptuple->set_value(0, i_id);
    return (table_man_impl<item_t>::index_probe_forupdate(db, pindex, ptuple));
}



/* ------------- */
/* --- STOCK --- */
/* ------------- */


w_rc_t stock_man_impl::index_probe(ss_m* db,
                                   stock_tuple* ptuple,
                                   const int i_id,
                                   const int w_id)
{
    assert (_ptable);
    index_desc_t* pindex = _ptable->find_index("S_INDEX");
    assert (pindex);

    ptuple->set_value(0, i_id);
    ptuple->set_value(1, w_id);
    return (table_man_impl<stock_t>::index_probe(db, pindex, ptuple));
}

w_rc_t stock_man_impl::index_probe_forupdate(ss_m* db,
                                             stock_tuple* ptuple,
                                             const int i_id,
                                             const int w_id)
{
    assert (_ptable);
    index_desc_t* pindex = _ptable->find_index("S_INDEX");
    assert (pindex);

    ptuple->set_value(0, i_id);
    ptuple->set_value(1, w_id);
    return (table_man_impl<stock_t>::index_probe_forupdate(db, pindex, ptuple));
}

w_rc_t  stock_man_impl::update_tuple(ss_m* db,
                                     stock_tuple* ptuple,
                                     const tpcc_stock_tuple* pstock)
{
    ptuple->set_value(2, pstock->S_REMOTE_CNT);
    ptuple->set_value(3, pstock->S_QUANTITY);
    ptuple->set_value(4, pstock->S_ORDER_CNT);
    ptuple->set_value(5, pstock->S_YTD);
    return (table_man_impl<stock_t>::update_tuple(db, ptuple));
}

