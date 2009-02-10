/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcb_schema.h
 *
 *  @brief:  Implementation of the workload-specific access methods 
 *           on TPC-C tables
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "workload/tpcb/shore_tpcb_schema_man.h"


using namespace shore;
using namespace tpcb;


/*********************************************************************
 *
 * Workload-specific access methods on tables
 *
 *********************************************************************/


/* ----------------- */
/* --- BRANCH --- */
/* ----------------- */


w_rc_t branch_man_impl::b_index_probe_forupdate(ss_m* db,
						branch_tuple* ptuple,
						const int b_id)
{
    assert (ptuple);    
    ptuple->set_value(0, b_id);
    return (index_probe_forupdate_by_name(db, "B_INDEX", ptuple));
}

/* ---------------- */
/* --- TELLER --- */
/* ---------------- */


w_rc_t teller_man_impl::t_index_probe_forupdate(ss_m* db,
						teller_tuple* ptuple,
						const int t_id)
{
    assert (ptuple);    
    ptuple->set_value(0, t_id);
    return (index_probe_forupdate_by_name(db, "T_INDEX", ptuple));
}



/* ---------------- */
/* --- ACCOUNT --- */
/* ---------------- */

w_rc_t account_man_impl::a_index_probe_forupdate(ss_m* db,
						account_tuple* ptuple,
						const int a_id)
{
    assert (ptuple);    
    ptuple->set_value(0, a_id);
    return (index_probe_forupdate_by_name(db, "A_INDEX", ptuple));
}

