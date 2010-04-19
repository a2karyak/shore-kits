/* -*- mode:C++; c-basic-offset:4 -*-
   Shore-kits -- Benchmark implementations for Shore-MT

   Copyright (c) 2007-2009
   Data Intensive Applications and Systems Labaratory (DIAS)
   Ecole Polytechnique Federale de Lausanne

   All Rights Reserved.

   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.

   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpch_xct.cpp
 *
 *  @brief:  Implementation of the Baseline Shore TPC-H transactions
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#include "workload/tpch/shore_tpch_env.h"
#include "workload/tpch/tpch_random.h"

#include <vector>
#include <map>
#include <numeric>
#include <algorithm>

#include "workload/tpch/dbgen/dss.h"
#include "workload/tpch/dbgen/dsstypes.h"

// #include "qp/shore/qp_util.h"
// #include "qp/shore/ahhjoin.h"

#include "sm_base.h"


using namespace shore;
using namespace dbgentpch;
//using namespace qp;


ENTER_NAMESPACE(tpch);



/******************************************************************** 
 *
 * Thread-local TPC-H TRXS Stats
 *
 ********************************************************************/


static __thread ShoreTPCHTrxStats my_stats;

void 
ShoreTPCHEnv::env_thread_init()
{
    CRITICAL_SECTION(stat_mutex_cs, _statmap_mutex);
    _statmap[pthread_self()] = &my_stats;
}

void 
ShoreTPCHEnv::env_thread_fini()
{
    CRITICAL_SECTION(stat_mutex_cs, _statmap_mutex);
    _statmap.erase(pthread_self());
}


/******************************************************************** 
 *
 *  @fn:    _get_stats
 *
 *  @brief: Returns a structure with the currently stats
 *
 ********************************************************************/

const ShoreTPCHTrxStats 
ShoreTPCHEnv::_get_stats()
{
    CRITICAL_SECTION(cs, _statmap_mutex);
    ShoreTPCHTrxStats rval;
    rval -= rval; // dirty hack to set all zeros
    for (statmap_t::iterator it=_statmap.begin(); it != _statmap.end(); ++it) 
        rval += *it->second;
    return (rval);
}


/******************************************************************** 
 *
 *  @fn:    reset_stats
 *
 *  @brief: Updates the last gathered statistics
 *
 ********************************************************************/

void 
ShoreTPCHEnv::reset_stats()
{
    CRITICAL_SECTION(last_stats_cs, _last_stats_mutex);
    _last_stats = _get_stats();
}


/******************************************************************** 
 *
 *  @fn:    print_throughput
 *
 *  @brief: Prints the throughput given a measurement delay
 *
 ********************************************************************/

void ShoreTPCHEnv::print_throughput(const double iQueriedSF, 
                                    const int iSpread, 
                                    const int iNumOfThreads, 
                                    const double delay,
                                    const ulong_t mioch,
                                    const double avgcpuusage)
{
    CRITICAL_SECTION(last_stats_cs, _last_stats_mutex);
    
    // get the current statistics
    ShoreTPCHTrxStats current_stats = _get_stats();
	   
    // now calculate the diff
    current_stats -= _last_stats;
	       
    int trxs_att  = current_stats.attempted.total();
    int trxs_abt  = current_stats.failed.total();
    int trxs_dld  = current_stats.deadlocked.total();    

    TRACE( TRACE_ALWAYS, "*******\n"             \
           "QueriedSF: (%.1f)\n"                 \
           "Spread:    (%s)\n"                   \
           "Threads:   (%d)\n"                   \
           "Trxs Att:  (%d)\n"                   \
           "Trxs Abt:  (%d)\n"                   \
           "Trxs Dld:  (%d)\n"                   \
           "Secs:      (%.2f)\n"                 \
           "IOChars:   (%.2fM/s)\n"              \
           "AvgCPUs:   (%.1f) (%.1f%%)\n"        \
           "TPS:       (%.2f)\n",
           iQueriedSF, 
           (iSpread ? "Yes" : "No"),
           iNumOfThreads, trxs_att, trxs_abt, trxs_dld,
           delay, mioch/delay, avgcpuusage, 100*avgcpuusage/64,
           (trxs_att-trxs_abt-trxs_dld)/delay);
}




/******************************************************************** 
 *
 * TPC-H Parallel Loading
 *
 ********************************************************************/

/*
  DATABASE POPULATION TRANSACTIONS

  The TPC-H database has 8 tables. Out of those:
  3 are based on the Customers (CUSTOMER,ORDER,LINEITEM)
  2 are based on the Parts (PART,PARTSUPP)
  2 are static (NATION,REGION)
  1 depends on the SF but not Customers or Parts (SUPPLIER)

  Regular cardinalities:

  Supplier :                10K*SF     (2MB)
  Nation   :                25         (<1MB)
  Region   :                5          (<1MB)

  Part     :                0.2M*SF    (30MB)
  PartSupp : 4*Part     =   0.8M*SF    (110MB)

  Customer :                0.15M*SF   (25MB)
  Order    : 10*Cust    =   1.5M*SF    (150MB)
  Lineitem : [1,7]*Cust =   6M*SF      (650MB)

  
  The table creator:
  1) Creates all the tables 
  2) Loads completely the first 3 tables (Supplier,Nation,Region)
  3) Loads #ParLoaders*DIVISOR Part units (Part,PartSupp)
  4) Loads #ParLoaders*DIVISOR Customer units (Customer,Order,Lineitem)


  The sizes of the records:
  NATION:   192
  REGION:   56
  SUPPLIER: 208
  PART:     176
  PARTSUPP: 224
  CUSTOMER: 240
  ORDERS:   152
  LINEITEM: 152

*/


/******************************************************************** 
 *
 * Those functions populate records for the TPC-H database. They do not
 * commit thought. So, they need to be invoked inside a transaction
 * and the caller needs to commit at the end. 
 *
 ********************************************************************/

#undef  DO_TPCH_LOAD
#define DO_TPCH_LOAD

#undef  DO_PRINT_TPCH_RECS
//#define DO_PRINT_TPCH_RECS

// Populates one nation
w_rc_t 
ShoreTPCHEnv::_gen_one_nation(const int id, 
                              rep_row_t& areprow)
{    
    w_rc_t e = RCOK;
    row_impl<nation_t>*   prna = _pnation_man->get_tuple();
    assert (prna);
    prna->_rep = &areprow;

    code_t ac;
    mk_nation(id, &ac);

#ifdef DO_PRINT_TPCH_RECS
    TRACE( TRACE_ALWAYS, "%ld,%s,%ld,%s,%d\n", 
           ac.code, ac.text, ac.join, ac.comment, ac.clen);
#endif

    prna->set_value(0, (int)ac.code);
    prna->set_value(1, ac.text);
    prna->set_value(2, (int)ac.join);
    prna->set_value(3, ac.comment);
        
    e = _pnation_man->add_tuple(_pssm, prna);

    _pnation_man->give_tuple(prna);
    return (e);
}


// Populates one region
w_rc_t 
ShoreTPCHEnv::_gen_one_region(const int id, 
                              rep_row_t& areprow)
{    
    w_rc_t e = RCOK;
    row_impl<region_t>*   prre = _pregion_man->get_tuple();
    assert (prre);
    prre->_rep = &areprow;

    code_t ac;
    mk_region(id, &ac);

#ifdef DO_PRINT_TPCH_RECS
    TRACE( TRACE_ALWAYS, "%ld,%s,%s,%d\n", 
           ac.code, ac.text, ac.comment, ac.clen);
#endif

    prre->set_value(0, (int)ac.code);
    prre->set_value(1, ac.text);
    prre->set_value(2, ac.comment);
        
    e = _pregion_man->add_tuple(_pssm, prre);

    _pregion_man->give_tuple(prre);
    return (e);
}



// Populates one supplier
w_rc_t 
ShoreTPCHEnv::_gen_one_supplier(const int id, 
                                rep_row_t& areprow)
{    
    w_rc_t e = RCOK;
    row_impl<supplier_t>* prsu = _psupplier_man->get_tuple();
    assert (prsu);
    prsu->_rep = &areprow;

    dbgentpch::supplier_t as;
    mk_supp(id, &as);

#ifdef DO_PRINT_TPCH_RECS
    if (id%100==0) {
        TRACE( TRACE_ALWAYS, "%ld,%s,%s,%d,%ld,%s,%ld,%s,%d\n", 
               as.suppkey,as.name,as.address,as.alen,as.nation_code,
               as.phone,as.acctbal,as.comment,as.clen); 
    }
#endif

    prsu->set_value(0, (int)as.suppkey);
    prsu->set_value(1, as.name);
    prsu->set_value(2, as.address);
    prsu->set_value(3, (int)as.nation_code);
    prsu->set_value(4, as.phone);
    prsu->set_value(5, (double)as.acctbal);
    prsu->set_value(6, as.comment);
        
    e = _psupplier_man->add_tuple(_pssm, prsu);

    _psupplier_man->give_tuple(prsu);
    return (e);
}



// Populates one part and the corresponding partsupp
w_rc_t 
ShoreTPCHEnv::_gen_one_part_based(const int id, 
                                  rep_row_t& areprow)
{    
    w_rc_t e = RCOK;
    row_impl<part_t>*     prpa = _ppart_man->get_tuple();
    assert (prpa);
    prpa->_rep = &areprow;

    row_impl<partsupp_t>* prps = _ppartsupp_man->get_tuple();
    assert (prps);
    prps->_rep = &areprow;

    {
        // 1. Part
        dbgentpch::part_t ap;
        mk_part(id, &ap);

#ifdef DO_PRINT_TPCH_RECS
        if (id%100==0) {
            TRACE( TRACE_ALWAYS, "%ld,%s,%s,%s,%s,%ld,%s,%ld,%s\n",
                   ap.partkey,ap.name,ap.mfgr,ap.brand,ap.type,
                   ap.size,ap.container,ap.retailprice,ap.comment); 
        }
#endif

        prpa->set_value(0, (int)ap.partkey);
        prpa->set_value(1, ap.name);
        prpa->set_value(2, ap.mfgr);
        prpa->set_value(3, ap.brand);
        prpa->set_value(4, ap.type);
        prpa->set_value(5, (int)ap.size);
        prpa->set_value(6, ap.container);
        prpa->set_value(7, (double)ap.retailprice);
        prpa->set_value(8, ap.comment);

        e = _ppart_man->add_tuple(_pssm, prpa);
        if (e.is_error()) { goto done; }

        for (int i=0; i< SUPP_PER_PART; ++i) {

#ifdef DO_PRINT_TPCH_RECS
            if (id%100==0) {
                TRACE( TRACE_ALWAYS, "%ld,%ld,%ld,%ld,%s\n",
                       ap.s[i].partkey,ap.s[i].suppkey,
                       ap.s[i].qty,ap.s[i].scost,
                       ap.s[i].comment); 
            }
#endif

            // 2. PartSupp
            prps->set_value(0, (int)ap.s[i].partkey);
            prps->set_value(1, (int)ap.s[i].suppkey);
            prps->set_value(2, (int)ap.s[i].qty);
            prps->set_value(3, (double)ap.s[i].scost);
            prps->set_value(4, ap.s[i].comment);
            
            e = _ppartsupp_man->add_tuple(_pssm, prps);
            if (e.is_error()) { goto done; }       
        }
    }

 done:
    _ppart_man->give_tuple(prpa);
    _ppartsupp_man->give_tuple(prps);
    return (e);
}



// Populates one customer and the corresponding orders and lineitems
w_rc_t 
ShoreTPCHEnv::_gen_one_cust_based(const int id, 
                                  rep_row_t& areprow)
{
    w_rc_t e = RCOK;    
    row_impl<customer_t>* prcu = _pcustomer_man->get_tuple();
    assert (prcu);
    prcu->_rep = &areprow;

    row_impl<orders_t>*    pror = _porders_man->get_tuple();
    assert (pror);
    pror->_rep = &areprow;

    row_impl<lineitem_t>* prli = _plineitem_man->get_tuple();
    assert (prli);
    prli->_rep = &areprow;

    {
        // 1. Customer
        dbgentpch::customer_t ac;
        mk_cust(id, &ac);

#ifdef DO_PRINT_TPCH_RECS        
        if (id%100==0) {
            TRACE( TRACE_ALWAYS, "%ld,%s,%s,%ld,%s,%ld,%s,%s\n",
                   ac.custkey,ac.name,ac.address,ac.nation_code,
                   ac.phone,ac.acctbal,ac.mktsegment,ac.comment); 
        }
#endif

        prcu->set_value(0, (int)ac.custkey);
        prcu->set_value(1, ac.name);
        prcu->set_value(2, ac.address);
        prcu->set_value(3, (int)ac.nation_code);
        prcu->set_value(4, ac.phone);
        prcu->set_value(5, (double)ac.acctbal);
        prcu->set_value(6, ac.mktsegment);
        prcu->set_value(7, ac.comment);

        _pcustomer_man->add_tuple(_pssm, prcu);
        if (e.is_error()) { goto done; }

        for (int i=0; i<ORDERS_PER_CUSTOMER; ++i) {

            // 2. Orders            
            dbgentpch::order_t ao;
            mk_order(id, &ao, 0);

#ifdef DO_PRINT_TPCH_RECS
            if (id%100==0) {
                TRACE( TRACE_ALWAYS, "%ld,%ld,%s,%ld,%s,%s,%s,%ld,%s\n",
                       ao.okey,ao.custkey,ao.orderstatus,ao.totalprice,
                       ao.odate,ao.opriority,ao.clerk,ao.spriority,ao.comment); 
            }
#endif
            
            pror->set_value(0, (int)ao.okey);            
            pror->set_value(1, (int)ao.custkey);
            pror->set_value(2, ao.orderstatus);
            pror->set_value(3, (double)ao.totalprice);
            pror->set_value(4, ao.odate);
            pror->set_value(5, ao.opriority);
            pror->set_value(6, ao.clerk);
            pror->set_value(7, (int)ao.spriority);
            pror->set_value(8, ao.comment);

            _porders_man->add_tuple(_pssm, pror);
            if (e.is_error()) { goto done; }            

            for (int j=0; j<ao.lines; ++j) {
                
                // 3. LineItem

#ifdef DO_PRINT_TPCH_RECS
                if (id%100==0) {
                    TRACE( TRACE_ALWAYS, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld," \
                           "%s,%s,%s,%s,%s,%s,%s,%s\n",
                           ao.l[j].okey,ao.l[j].partkey,ao.l[j].suppkey,
                           ao.l[j].lcnt,ao.l[j].quantity,ao.l[j].eprice,
                           ao.l[j].discount,ao.l[j].tax,
                           ao.l[j].rflag,ao.l[j].lstatus,
                           ao.l[j].cdate,ao.l[j].sdate,ao.l[j].rdate,
                           ao.l[j].shipinstruct,ao.l[j].shipmode,ao.l[j].comment); 
                }
#endif

                prli->set_value(0, (int)ao.l[j].okey);
                prli->set_value(1, (int)ao.l[j].partkey);
                prli->set_value(2, (int)ao.l[j].suppkey);
                prli->set_value(3, (int)ao.l[j].lcnt);
                prli->set_value(4, (double)ao.l[j].quantity);
                prli->set_value(5, (double)ao.l[j].eprice);
                prli->set_value(6, (double)ao.l[j].discount);
                prli->set_value(7, (double)ao.l[j].tax);
                prli->set_value(8, ao.l[j].rflag);
                prli->set_value(9, ao.l[j].lstatus);
                prli->set_value(10, ao.l[j].cdate);
                prli->set_value(11, ao.l[j].sdate);
                prli->set_value(12, ao.l[j].rdate);
                prli->set_value(13, ao.l[j].shipinstruct);
                prli->set_value(14, ao.l[j].shipmode);
                prli->set_value(15, ao.l[j].comment);

                _plineitem_man->add_tuple(_pssm, prli);
                if (e.is_error()) { goto done; }                            
            }
        }
    }

 done:
    _pcustomer_man->give_tuple(prcu);
    _porders_man->give_tuple(pror);
    _plineitem_man->give_tuple(prli);   
    return (e);
}


/******************************************************************** 
 *
 * TPC-H Loading: Population transactions
 *
 ********************************************************************/

w_rc_t 
ShoreTPCHEnv::xct_populate_baseline(const int xct_id, 
                                    populate_baseline_input_t& in)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);

    // The Customer is the biggest (240) of all the tables
    rep_row_t areprow(_pcustomer_man->ts());
    areprow.set(_pcustomer_desc->maxsize());

    w_rc_t e = RCOK;

#ifdef DO_TPCH_LOAD
    // 2. Build the small tables
    TRACE( TRACE_ALWAYS, "Building NATION !!!\n");

    for (int i=0; i<NO_NATIONS; ++i) {
        e = _gen_one_nation(i, areprow);
    }
    if(e.is_error()) { return (e); }


    TRACE( TRACE_ALWAYS, "Building REGION !!!\n");

    for (int i=0; i<NO_REGIONS; ++i) {
        e = _gen_one_region(i, areprow);
    }
    if(e.is_error()) { return (e); }


    TRACE( TRACE_ALWAYS, "Building SUPPLIER !!!\n");

    for (int i=0; i<in._sf*SUPPLIER_PER_SF; ++i) {
        e = _gen_one_supplier(i, areprow);
    }
    if(e.is_error()) { return (e); }
#endif


    TRACE( TRACE_ALWAYS, "Starting PARTS !!!\n");

    // 3. Insert first rows in the Part-based tables
    for (int i=0; i < in._loader_count; ++i) {
        long start = i*in._parts_per_thread;
        long end = start + in._divisor - 1;
        TRACE( TRACE_ALWAYS, "Parts %d .. %d\n", start, end);
            
        for (int j=start; j<end; ++j) {
#ifdef DO_TPCH_LOAD
            e = _gen_one_part_based(j,areprow);
#endif
            if(e.is_error()) { return (e); }
        }            
    }


    TRACE( TRACE_ALWAYS, "Starting CUSTS !!!\n");

    // 4. Insert first rows in the Customer-based tables
    for (int i=0; i < in._loader_count; ++i) {
        long start = i*in._custs_per_thread;
        long end = start + in._divisor - 1;
        TRACE( TRACE_ALWAYS, "Custs %d .. %d\n", start, end);
            
        for (int j=start; j<end; ++j) {
#ifdef DO_TPCH_LOAD
            e = _gen_one_cust_based(j,areprow);
#endif
            if(e.is_error()) { return (e); }
        }
    }

    e = _pssm->commit_xct();
    if(e.is_error()) { return (e); }

    return (e);
}


w_rc_t 
ShoreTPCHEnv::xct_populate_one_part(const int xct_id, 
                                    populate_one_part_input_t& in)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);

    // The Partsupp is the biggest (224) of all the 2 tables
    rep_row_t areprow(_ppartsupp_man->ts());
    areprow.set(_ppartsupp_desc->maxsize());

    w_rc_t e = RCOK;

    int id = in._partid;

#ifdef DO_TPCH_LOAD
    e = _gen_one_part_based(id, areprow);
#endif

    if(e.is_error()) { return (e); }

    e = _pssm->commit_xct();
    if(e.is_error()) { return (e); }

    return (e);
}


w_rc_t 
ShoreTPCHEnv::xct_populate_one_cust(const int xct_id, 
                                    populate_one_cust_input_t& in)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);

    // The Customer is the biggest (240) of the 3 tables
    rep_row_t areprow(_pcustomer_man->ts());
    areprow.set(_pcustomer_desc->maxsize()); 

    w_rc_t e = RCOK;

    int id = in._custid;

#ifdef DO_TPCH_LOAD
    e = _gen_one_cust_based(id, areprow);
#endif
    if(e.is_error()) { return (e); }

    e = _pssm->commit_xct();
    if(e.is_error()) { return (e); }
        
    return e;
} 



/******************************************************************** 
 *
 * TPC-H QUERIES (TRXS)
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

w_rc_t ShoreTPCHEnv::run_one_xct(Request* prequest)
{
    // if BASELINE TPC-H MIX
    if (prequest->type() == XCT_TPCH_MIX) {
        prequest->set_type(random_xct_type(smthread_t::me()->rand()%100));
    }

    switch (prequest->type()) {

        // TPC-H BASELINE
    case XCT_TPCH_Q1:
        return (run_q1(prequest));
    case XCT_TPCH_Q6:
        return (run_q6(prequest));;
    case XCT_TPCH_Q12:
        return (run_q12(prequest));;
    case XCT_TPCH_Q14:
        return (run_q14(prequest));;
    default:
        assert (0); // UNKNOWN TRX-ID
    }
    return (RCOK);
}


/******************************************************************** 
 *
 * TPC-H TRXs Wrappers
 *
 * @brief: They are wrappers to the functions that execute the transaction
 *         body. Their responsibility is to:
 *
 *         1. Prepare the corresponding input
 *         2. Check the return of the trx function and abort the trx,
 *            if something went wrong
 *         3. Update the tpch db environment statistics
 *
 ********************************************************************/


// --- without input specified --- //

DEFINE_TRX(ShoreTPCHEnv,q1);
DEFINE_TRX(ShoreTPCHEnv,q6);
DEFINE_TRX(ShoreTPCHEnv,q12);
DEFINE_TRX(ShoreTPCHEnv,q14);


// uncomment the line below if want to dump (part of) the trx results
//#define PRINT_TRX_RESULTS


/******************************************************************** 
 *
 * TPC-H Q1
 *
 ********************************************************************/

class q1_group_by_key_t 
{
public:
    char return_flag[2];
    char linestatus[2];

    q1_group_by_key_t(char * flag, char *status)
    {
        memcpy(return_flag, flag, 2);
        memcpy(linestatus, status, 2);
    };

    q1_group_by_key_t(const q1_group_by_key_t& rhs)
    {
        memcpy(return_flag, rhs.return_flag, 2);
        memcpy(linestatus, rhs.linestatus, 2);
    };

    q1_group_by_key_t& operator=(const q1_group_by_key_t& rhs)
    {
        memcpy(return_flag, rhs.return_flag, 2);
        memcpy(linestatus, rhs.linestatus, 2);

        return (*this);
    };
};


class q1_group_by_comp {
public:
    bool operator() (
                     const q1_group_by_key_t& lhs, 
                     const q1_group_by_key_t& rhs) const
    {
        return ((lhs.return_flag[0] < rhs.return_flag[0]) ||
                ((lhs.return_flag[0] == rhs.return_flag[0]) &&
                 (lhs.linestatus[0] < rhs.linestatus[0]))
		);
    }
};


class q1_group_by_value_t {
public:
    int sum_qty;
    int sum_base_price;
    decimal sum_disc_price;
    decimal sum_charge;
    decimal sum_discount;
    int count;

    q1_group_by_value_t() {
        sum_qty = 0;
        sum_base_price = 0;
        sum_disc_price = 0;
        sum_charge = 0;
        sum_discount = 0;
        count = 0;
    }

    q1_group_by_value_t(const q1_group_by_value_t& rhs) {
        sum_qty = rhs.sum_qty;
        sum_base_price = rhs.sum_base_price;
        sum_disc_price = rhs.sum_disc_price;;
        sum_charge = rhs.sum_charge;
        sum_discount = rhs.sum_discount;
        count = rhs.count;
    }

    q1_group_by_value_t& operator+=(const q1_group_by_value_t& rhs)
    {
        sum_qty += rhs.sum_qty;
        sum_base_price += rhs.sum_base_price;
        sum_disc_price += rhs.sum_disc_price;;
        sum_charge += rhs.sum_charge;
        sum_discount += rhs.sum_discount;
        count += rhs.count;

        return(*this);
    }
};


struct q1_output_ele_t 
{
    char l_returnflag[2];
    char l_linestatus[2];
    int sum_qty;
    int sum_base_price;
    decimal sum_disc_price;
    decimal sum_charge;
    decimal avg_qty;
    decimal avg_price;
    decimal avg_disc;
    int count_order;
};


w_rc_t ShoreTPCHEnv::xct_q1(const int xct_id, 
                            q1_input_t& pq1in)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // q1 trx touches 1 tables:
    // lineitem
    row_impl<lineitem_t>* prlineitem = _plineitem_man->get_tuple();
    assert (prlineitem);

    w_rc_t e = RCOK;

    // allocate space for lineitem representation
    rep_row_t areprow(_plineitem_man->ts());
    areprow.set(_plineitem_desc->maxsize()); 

    prlineitem->_rep = &areprow;

    /*
      select
      l_returnflag,
      l_linestatus,
      sum(l_quantity) as sum_qty,
      sum(l_extendedprice) as sum_base_price,
      sum(l_extendedprice*(1-l_discount)) as sum_disc_price,
      sum(l_extendedprice*(1-l_discount)*(1+l_tax)) as sum_charge,
      avg(l_quantity) as avg_qty,
      avg(l_extendedprice) as avg_price,
      avg(l_discount) as avg_disc,
      count(*) as count_order
      from
      lineitem
      where
      l_shipdate <= date '1998-12-01' - interval '[DELTA]' day (3)
      group by
      l_returnflag,
      l_linestatus
      order by
      l_returnflag,
      l_linestatus;
    */

    /* table scan lineitem */

    {
        guard< table_scan_iter_impl<lineitem_t> > l_iter;
        {
            table_scan_iter_impl<lineitem_t>* tmp_l_iter;
            e = _plineitem_man->get_iter_for_file_scan(_pssm, tmp_l_iter);
            l_iter = tmp_l_iter;
            if (e.is_error()) { goto done; }
        }

        bool eof;

        tpch_lineitem_tuple aline;

        map<q1_group_by_key_t, q1_group_by_value_t, q1_group_by_comp> q1_result;
        map<q1_group_by_key_t, q1_group_by_value_t>::iterator it;
        vector<q1_output_ele_t> q1_output;

        /*
          l_returnflag = 8 l_linestatus = 9 l_quantity = 4
          l_extendedprice = 5 l_discount = 6 l_tax = 7
          l_shipdate = 10
        */
        /*
          int sum_qty;
          int sum_base_price;
          decimal sum_disc_price;
          decimal sum_charge;
          decimal sum_discount;
          int count;
        */

        e = l_iter->next(_pssm, eof, *prlineitem);
        q1_group_by_value_t value;

        if (e.is_error()) { goto done; }
        while (!eof) {
            prlineitem->get_value(4, aline.L_QUANTITY);
            prlineitem->get_value(5, aline.L_EXTENDEDPRICE);
            prlineitem->get_value(6, aline.L_DISCOUNT);
            prlineitem->get_value(7, aline.L_TAX);
            prlineitem->get_value(8, aline.L_RETURNFLAG, 2);
            prlineitem->get_value(9, aline.L_LINESTATUS, 2);
            prlineitem->get_value(10, aline.L_SHIPDATE, 15);

            // IP: Needs to convert the string of SHIPDATE to time_t
            time_t the_shipdate = str_to_timet(aline.L_SHIPDATE);

            if (the_shipdate <= pq1in.l_shipdate) {
                q1_group_by_key_t key(aline.L_RETURNFLAG, aline.L_LINESTATUS);

                value.sum_qty = aline.L_QUANTITY;
                value.sum_base_price = aline.L_EXTENDEDPRICE;
                value.sum_disc_price = (aline.L_EXTENDEDPRICE *
                                        (1-aline.L_DISCOUNT));
                value.sum_charge = (aline.L_EXTENDEDPRICE *
                                    (1-aline.L_DISCOUNT) * 
                                    (1+aline.L_TAX));
                value.sum_discount = (aline.L_DISCOUNT);
                value.count = 1;

                it = q1_result.find(key);
                if (it != q1_result.end()) {
                    // exists, update 
                    (*it).second += value;
                } else {
                    q1_result.insert(pair<q1_group_by_key_t, q1_group_by_value_t>(key, value));
                }
            }

            e = l_iter->next(_pssm, eof, *prlineitem);
            if (e.is_error()) {goto done;}
        }

        q1_output_ele_t q1_output_ele;
        for (it = q1_result.begin(); it != q1_result.end(); it ++) {
            memcpy(q1_output_ele.l_returnflag, (*it).first.return_flag, 2);
            memcpy(q1_output_ele.l_linestatus, (*it).first.linestatus, 2);
            q1_output_ele.sum_qty = (*it).second.sum_qty;
            q1_output_ele.sum_base_price = (*it).second.sum_base_price;
            q1_output_ele.sum_disc_price = (*it).second.sum_disc_price;
            q1_output_ele.sum_charge = (*it).second.sum_charge;
            q1_output_ele.avg_qty = (*it).second.sum_qty / (*it).second.count;
            q1_output_ele.avg_price = (*it).second.sum_base_price / (*it).second.count;
            q1_output_ele.avg_disc = (*it).second.sum_discount / (*it).second.count;
            q1_output_ele.count_order = (*it).second.count;

            q1_output.push_back(q1_output_ele);
        }

        e = _pssm->commit_xct();
        if (e.is_error()) {goto done;}
    } // goto

 done:    
    // return the tuples to the cache
    _plineitem_man->give_tuple(prlineitem);
    return (e);

};// EOF: Q1 



/******************************************************************** 
 *
 * TPC-H Q6
 *
 ********************************************************************/

/*
 * l_extendedprice l_discount l_shipdate l_quantity
 */

w_rc_t 
ShoreTPCHEnv::xct_q6(const int xct_id, 
                     q6_input_t& pq6in)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    // q6 trx touches 1 tables:
    // lineitem
    row_impl<lineitem_t>* prlineitem = _plineitem_man->get_tuple();
    assert (prlineitem);

    w_rc_t e = RCOK;

    // allocate space for lineitem representation
    rep_row_t areprow(_plineitem_man->ts());
    areprow.set(_plineitem_desc->maxsize()); 

    prlineitem->_rep = &areprow;
    
    //       select
    //       sum(l_extendedprice*l_discount) as revenue
    //       from
    //       lineitem
    //       where
    //       l_shipdate >= date '[DATE]'
    //       and l_shipdate < date '[DATE]' + interval '1' year
    //       and l_discount between [DISCOUNT] - 0.01 and [DISCOUNT] + 0.01
    //       and l_quantity < [QUANTITY]
	
    // index scan on shipdate
    rep_row_t lowrep(_plineitem_man->ts());
    rep_row_t highrep(_plineitem_man->ts());

    lowrep.set(_plineitem_desc->maxsize());
    highrep.set(_plineitem_desc->maxsize());

    {
        struct tm date;

        if (gmtime_r(&(pq6in.l_shipdate), &date) == NULL) {
            goto done;
        }

        date.tm_year ++;

        time_t last_shipdate = mktime(&date);

        guard< index_scan_iter_impl<lineitem_t> > l_iter;
        {
            index_scan_iter_impl<lineitem_t>* tmp_l_iter;
            e = _plineitem_man->l_get_range_iter_by_index(_pssm, tmp_l_iter,
                                                          prlineitem, lowrep, highrep,
                                                          pq6in.l_shipdate,
                                                          last_shipdate);
            l_iter = tmp_l_iter;
            if (e.is_error()) { goto done; }

        }

        bool eof;

        tpch_lineitem_tuple aline;

        double q6_result = 0;

        e = l_iter->next(_pssm, eof, *prlineitem);
        if (e.is_error()) { goto done; }
        while (!eof) {
            prlineitem->get_value(4, aline.L_QUANTITY);
            prlineitem->get_value(5, aline.L_EXTENDEDPRICE);
            prlineitem->get_value(6, aline.L_DISCOUNT);
            prlineitem->get_value(10, aline.L_SHIPDATE, 15);

            //	if ((aline.L_SHIPDATE >= pq6in.l_shipdate) &&
            //	    (aline.L_SHIPDATE <= last_shipdate) && 
            //		  (aline.L_DISCOUNT > pq6in.l_discount - 0.01) &&
            //		  (aline.L_DISCOUNT < pq6in.l_discount + 0.01) &&
            //		  (aline.L_QUANTITY < pq6in.l_quantity)) {

            if ((aline.L_DISCOUNT > pq6in.l_discount - 0.01) &&
                (aline.L_DISCOUNT < pq6in.l_discount + 0.01) &&
                (aline.L_QUANTITY < pq6in.l_quantity)) {

                q6_result += (aline.L_EXTENDEDPRICE *
                              aline.L_DISCOUNT);

            }

            e = l_iter->next(_pssm, eof, *prlineitem);
            if (e.is_error()) {goto done;}
        }

        e = _pssm->commit_xct();
        if (e.is_error()) {goto done;}
    } // goto

 done:    
    // return the tuples to the cache
    _plineitem_man->give_tuple(prlineitem);
    return (e);

}; // EOF: Q6 




/******************************************************************** 
 *
 * TPC-H Q12
 *
 ********************************************************************/

//  * Query 12
//  * 
//  * select l_shipmode, sum(case
//  * 	when o_orderpriority = '1-URGENT' or o_orderpriority = '2-HIGH'
//  * 	then 1
//  * 	else 0 end) as high_line_count, 
//  *    sum(case
//  *      when o_orderpriority <> '1-URGENT' and o_orderpriority <> '2-HIGH'
//  *      then 1
//  *      else 0 end) as low_line_count
//  * from tpcd.orders, tpcd.lineitem
//  * where o_orderkey = l_orderkey and
//  *       (l_shipmode = "SHIP" or l_shipmode = "RAIL") and
//  *       l_commitdate < l_receiptdate and
//  *       l_shipdate < l_commitdate and
//  *       l_receiptdate >= "1994-01-01" and
//  *       l_receiptdate < "1995-01-01"
//  * group by l_shipmode
//  * order by l_shipmode


// bool 
// q12_l_pred_nsm(const void* tuple, 
//                const file_info_t& finfo)
// {
//     char* frec = (char*)tuple;
//     char* shipmode = FIELD_START(finfo, frec, L_SHIPMODE);
//     char* commitdate = FIELD_START(finfo, frec, L_COMMITDATE);
//     char* receiptdate = FIELD_START(finfo, frec, L_RECEIPTDATE);
//     char* shipdate = FIELD_START(finfo, frec, L_SHIPDATE);
//     if ((strcmp(shipmode, "SHIP")==0 || 
//          strcmp(shipmode, "RAIL")==0) &&
//     	COMPARE_DATE(commitdate, receiptdate) < 0 &&
// 	COMPARE_DATE(shipdate, commitdate) < 0 &&
// 	COMPARE_DATE(receiptdate, "1994-01-01") >= 0 &&
// 	COMPARE_DATE(receiptdate, "1995-01-01") < 0)
//         return true;
//     else return false;
// }


// void 
// q12_final_touch_nsm(void *query, 
//                     const void* tuple1, 
//                     const void* tuple2)
// {
//     tpch_query_12 *q = (tpch_query_12 *)query;

//     char* left_frec = (char*)tuple1;
//     char* right_frec = (char*)tuple2;

//     w_assert3(strcmp(left_frec+42, "RAIL")==0 || 
//               strcmp(left_frec+42,"SHIP")==0);

//     if (strcmp(left_frec+42, "RAIL")==0)
//     	if (strcmp(right_frec, "1-URGENT")==0 ||
// 	    strcmp(right_frec, "2-HIGH")==0)
//             q->r_high++;
//     	else 
//             {
//                 w_assert3(strcmp(right_frec+4, "1-URGENT")!=0 &&
//                           strcmp(right_frec, "2-HIGH")!=0);
//                 q->r_low++;
//             }
//     else 
//         {
//             w_assert3(strcmp(left_frec+42, "SHIP")==0);
//             if (strcmp(right_frec, "1-URGENT")==0 ||
//                 strcmp(right_frec, "2-HIGH")==0)
// 	        q->s_high++;
//             else
//                 {
//                     w_assert3(strcmp(right_frec, "1-URGENT")!=0 &&
//                               strcmp(right_frec, "2-HIGH")!=0);
//                     q->s_low++;
//                 }
//         }
// }


w_rc_t ShoreTPCHEnv::xct_q12(const int xct_id, 
                             q12_input_t& in)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    TRACE( TRACE_ALWAYS, "%s %s %d\n", 
           in.l_shipmode1, in.l_shipmode2, in.l_receiptdate);


//     /// NAT:
//     w_rc_t rc;

//     flid_t ifi_lineitem[5];
//     ifi_lineitem[0] = L_ORDERKEY;
//     ifi_lineitem[1] = L_SHIPDATE;
//     ifi_lineitem[2] = L_COMMITDATE;
//     ifi_lineitem[3] = L_RECEIPTDATE;
//     ifi_lineitem[4] = L_SHIPMODE;

//     flid_t ifi_order[2];
//     ifi_order[0] = O_ORDERPRIORITY;
//     ifi_order[1] = O_ORDERKEY;

//     ahhjoin_q<lineitem_t,orders_t> q12(table_name, L_ORDERKEY, O_ORDERKEY, 5, 2, 
//                                        ifi_lineitem, ifi_order, q12_l_pred_nsm, NULL, 
//                                        (query_c*)this, q12_final_touch_nsm);
//     W_DO(q12.run());


//     fprintf(output_file, 
//             "QUERY RESULT: \n\tSHIPMODE\tHIGH\tLOW\n\tRAIL\t\t%6.0f\t%6.0f\n\tSHIP\t\t%6.0f\t%6.0f\n", 
//             (double)r_high, (double)r_low, (double)s_high, (double)s_low);


    return (RCOK);
}



/******************************************************************** 
 *
 * TPC-H Q14
 *
 ********************************************************************/

w_rc_t ShoreTPCHEnv::xct_q14(const int xct_id, 
                             q14_input_t& in)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    TRACE( TRACE_ALWAYS, "%d\n", 
           in.l_shipdate);

    return (RC(smlevel_0::eNOTIMPLEMENTED));
}




EXIT_NAMESPACE(tpch);
