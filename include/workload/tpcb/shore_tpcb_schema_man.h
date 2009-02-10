/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcb_schema_man.h
 *
 *  @brief:  Declaration of the TPC-C table managers
 *
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_TPCB_SCHEMA_MANAGER_H
#define __SHORE_TPCB_SCHEMA_MANAGER_H

#include "workload/tpcb/shore_tpcb_schema.h"


using namespace shore;


ENTER_NAMESPACE(tpcb);



/* ------------------------------------------------------------------ */
/* --- The managers of all the tables used in the TPC-C benchmark --- */
/* ------------------------------------------------------------------ */


class branch_man_impl : public table_man_impl<branch_t>
{
    typedef row_impl<branch_t> branch_tuple;

public:

    branch_man_impl(branch_t* aBranchDesc)
        : table_man_impl<branch_t>(aBranchDesc)
    { }

    ~branch_man_impl() { }

    /* --- access specific tuples  --- */
    w_rc_t b_index_probe_forupdate(ss_m* db, 
				   branch_tuple* ptuple, 
				   const int b_id);
    

}; // EOF: branch_man_impl



class teller_man_impl : public table_man_impl<teller_t>
{
    typedef row_impl<teller_t> teller_tuple;

public:

    teller_man_impl(teller_t* aTellerDesc)
        : table_man_impl<teller_t>(aTellerDesc)
    { }

    ~teller_man_impl() { }

    /* --- access specific tuples  --- */
    w_rc_t t_index_probe_forupdate(ss_m* db, 
				   teller_tuple* ptuple, 
				   const int t_id);
    


}; // EOF: teller_man_impl



class account_man_impl : public table_man_impl<account_t>
{
    typedef row_impl<account_t> account_tuple;
    typedef table_scan_iter_impl<account_t> account_table_iter;
    typedef index_scan_iter_impl<account_t> account_index_iter;

public:

    account_man_impl(account_t* aAccountDesc)
        : table_man_impl<account_t>(aAccountDesc)
    { }

    ~account_man_impl() { }

    /* --- access specific tuples  --- */
    w_rc_t a_index_probe_forupdate(ss_m* db, 
				   account_tuple* ptuple, 
				   const int a_id);
    


}; // EOF: account_man_impl




class history_man_impl : public table_man_impl<history_t>
{
    typedef row_impl<history_t> history_tuple;

public:

    history_man_impl(history_t* aHistoryDesc)
        : table_man_impl<history_t>(aHistoryDesc)
    { }

    ~history_man_impl() { }

}; // EOF: history_man_impl



EXIT_NAMESPACE(tpcb);

#endif /* __SHORE_TPCB_SCHEMA_MANAGER_H */
