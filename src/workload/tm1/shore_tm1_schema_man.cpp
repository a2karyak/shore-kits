/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tm1_schema.h
 *
 *  @brief:  Implementation of the workload-specific access methods 
 *           on TM1 tables
 *
 *  @author: Ippokratis Pandis, Feb 2009
 *
 */

#include "workload/tm1/shore_tm1_schema_man.h"


using namespace shore;
using namespace tm1;


/*********************************************************************
 *
 * Workload-specific access methods on tables
 *
 *********************************************************************/


/* ------------------- */
/* --- SUBSCRIBERS --- */
/* ------------------- */


w_rc_t sub_man_impl::sub_idx_probe(ss_m* db,
                                   sub_tuple* ptuple,
                                   const int s_id)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    return (index_probe_by_name(db, "S_IDX", ptuple));
}

w_rc_t sub_man_impl::sub_idx_upd(ss_m* db,
                                 sub_tuple* ptuple,
                                 const int s_id)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    return (index_probe_forupdate_by_name(db, "S_IDX", ptuple));
}

w_rc_t sub_man_impl::sub_idx_nl(ss_m* db,
                                sub_tuple* ptuple,
                                const int s_id)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    return (index_probe_nl_by_name(db, "S_IDX_NL", ptuple));
}



w_rc_t sub_man_impl::sub_nbr_idx_probe(ss_m* db,
                                       sub_tuple* ptuple,
                                       const char* s_nbr)
{
    w_assert3 (ptuple);    
    ptuple->set_value(1, s_nbr);
    return (index_probe_by_name(db, "SUB_NBR_IDX", ptuple));
}

w_rc_t sub_man_impl::sub_nbr_idx_upd(ss_m* db,
                                     sub_tuple* ptuple,
                                     const char* s_nbr)
{
    w_assert3 (ptuple);    
    ptuple->set_value(1, s_nbr);
    return (index_probe_forupdate_by_name(db, "SUB_NBR_IDX", ptuple));
}

w_rc_t sub_man_impl::sub_nbr_idx_nl(ss_m* db,
                                    sub_tuple* ptuple,
                                    const char* s_nbr)
{
    w_assert3 (ptuple);    
    ptuple->set_value(1, s_nbr);
    return (index_probe_nl_by_name(db, "SUB_NBR_IDX_NL", ptuple));
}



/* ------------------- */
/* --- ACCESS_INFO --- */
/* ------------------- */


w_rc_t ai_man_impl::ai_idx_probe(ss_m* db,
                                 ai_tuple* ptuple,
                                 const int s_id, const short ai_type)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    ptuple->set_value(1, ai_type);
    return (index_probe_by_name(db, "AI_IDX", ptuple));
}

w_rc_t ai_man_impl::ai_idx_upd(ss_m* db,
                               ai_tuple* ptuple,
                               const int s_id, const short ai_type)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    ptuple->set_value(1, ai_type);
    return (index_probe_forupdate_by_name(db, "AI_IDX", ptuple));
}

w_rc_t ai_man_impl::ai_idx_nl(ss_m* db,
                              ai_tuple* ptuple,
                              const int s_id, const short ai_type)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    ptuple->set_value(1, ai_type);
    return (index_probe_nl_by_name(db, "AI_IDX_NL", ptuple));
}



/* ------------------------ */
/* --- SPECIAL_FACILITY --- */
/* ------------------------ */


w_rc_t sf_man_impl::sf_idx_probe(ss_m* db,
                                 sf_tuple* ptuple,
                                 const int s_id, const short sf_type)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    ptuple->set_value(1, sf_type);
    return (index_probe_by_name(db, "SF_IDX", ptuple));
}

w_rc_t sf_man_impl::sf_idx_upd(ss_m* db,
                               sf_tuple* ptuple,
                               const int s_id, const short sf_type)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    ptuple->set_value(1, sf_type);
    return (index_probe_forupdate_by_name(db, "SF_IDX", ptuple));
}

w_rc_t sf_man_impl::sf_idx_nl(ss_m* db,
                              sf_tuple* ptuple,
                              const int s_id, const short sf_type)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    ptuple->set_value(1, sf_type);
    return (index_probe_nl_by_name(db, "SF_IDX_NL", ptuple));
}




w_rc_t sf_man_impl::sf_get_idx_iter(ss_m* db,
                                    sf_idx_iter* &iter,
                                    sf_tuple* ptuple,
                                    rep_row_t &replow,
                                    rep_row_t &rephigh,
                                    const int sub_id,
                                    lock_mode_t alm,
                                    bool need_tuple)
{
    w_assert3 (ptuple);

    /* find the index */
    w_assert3 (_ptable);
    index_desc_t* pindex = NULL;
    if (alm == NL) 
        pindex = _ptable->find_index("SF_IDX_NL");
    else
        pindex = _ptable->find_index("SF_IDX");        
    w_assert3 (pindex);

    // CF_IDX: { 0 - 1 }

    // prepare the key to be probed
    ptuple->set_value(0, sub_id);
    ptuple->set_value(1, (short)1); // smallest SF_TYPE (1-4)

    int lowsz = format_key(pindex, ptuple, replow);
    w_assert3 (replow._dest);


    ptuple->set_value(1, (short)4); // largest SF_TYPE (1-4)

    int highsz = format_key(pindex, ptuple, rephigh);
    w_assert3 (rephigh._dest);    

    /* index only access */
    W_DO(get_iter_for_index_scan(db, pindex, iter,
                                 alm, need_tuple,
				 scan_index_i::ge, vec_t(replow._dest, lowsz),
				 scan_index_i::lt, vec_t(rephigh._dest, highsz)));
    return (RCOK);
}


w_rc_t sf_man_impl::sf_get_idx_iter_nl(ss_m* db,
                                       sf_idx_iter* &iter,
                                       sf_tuple* ptuple,
                                       rep_row_t &replow,
                                       rep_row_t &rephigh,
                                       const int sub_id,
                                       bool need_tuple)
{
    return (sf_get_idx_iter(db,iter,ptuple,replow,rephigh,sub_id,NL,need_tuple));
}




/* ----------------------- */
/* --- CALL_FORWARDING --- */
/* ----------------------- */


w_rc_t cf_man_impl::cf_idx_probe(ss_m* db,
                                 cf_tuple* ptuple,
                                 const int s_id, const short sf_type, 
                                 const short stime)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    ptuple->set_value(1, sf_type);
    ptuple->set_value(2, stime);
    return (index_probe_by_name(db, "CF_IDX", ptuple));
}

w_rc_t cf_man_impl::cf_idx_upd(ss_m* db,
                               cf_tuple* ptuple,
                               const int s_id, const short sf_type, 
                               const short stime)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    ptuple->set_value(1, sf_type);
    ptuple->set_value(2, stime);
    return (index_probe_forupdate_by_name(db, "CF_IDX", ptuple));
}

w_rc_t cf_man_impl::cf_idx_nl(ss_m* db,
                              cf_tuple* ptuple,
                              const int s_id, const short sf_type, 
                              const short stime)
{
    w_assert3 (ptuple);    
    ptuple->set_value(0, s_id);
    ptuple->set_value(1, sf_type);
    ptuple->set_value(2, stime);
    return (index_probe_nl_by_name(db, "CF_IDX_NL", ptuple));
}




w_rc_t cf_man_impl::cf_get_idx_iter(ss_m* db,
                                    cf_idx_iter* &iter,
                                    cf_tuple* ptuple,
                                    rep_row_t &replow,
                                    rep_row_t &rephigh,
                                    const int sub_id,
                                    const short sf_type,
                                    const short s_time,
                                    lock_mode_t alm,
                                    bool need_tuple)
{
    w_assert3 (ptuple);

    /* find the index */
    w_assert3 (_ptable);
    index_desc_t* pindex = NULL;
    if (alm == NL)
        pindex = _ptable->find_index("CF_IDX_NL");
    else
        pindex = _ptable->find_index("CF_IDX");
    w_assert3 (pindex);

    // CF_IDX: {0 - 1 - 2}

    // prepare the key to be probed
    ptuple->set_value(0, sub_id);
    ptuple->set_value(1, sf_type);
    ptuple->set_value(2, s_time);

    int lowsz = format_key(pindex, ptuple, replow);
    w_assert3 (replow._dest);


    ptuple->set_value(2, (short)24); // largest S_TIME

    int highsz = format_key(pindex, ptuple, rephigh);
    w_assert3 (rephigh._dest);    

    /* index only access */
    W_DO(get_iter_for_index_scan(db, pindex, iter,
                                 alm, need_tuple,
				 scan_index_i::ge, vec_t(replow._dest, lowsz),
				 scan_index_i::lt, vec_t(rephigh._dest, highsz)));
    return (RCOK);
}


w_rc_t cf_man_impl::cf_get_idx_iter_nl(ss_m* db,
                                       cf_idx_iter* &iter,
                                       cf_tuple* ptuple,
                                       rep_row_t &replow,
                                       rep_row_t &rephigh,
                                       const int sub_id,
                                       const short sf_type,
                                       const short s_time,
                                       bool need_tuple)
{
    return (cf_get_idx_iter(db,iter,ptuple,replow,rephigh,sub_id,sf_type,s_time,NL,need_tuple));
}
