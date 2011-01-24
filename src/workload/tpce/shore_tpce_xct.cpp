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

/** @file:   shore_tpce_xct.cpp
 *
 *  @brief:  Implementation of the Baseline Shore TPC-E transactions
 *
 *  @author: Cansu Kaynak
 *  @author: Djordje Jevdjic
 */

#include "workload/tpce/shore_tpce_env.h"
#include "workload/tpce/tpce_const.h"
#include "workload/tpce/tpce_input.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include "workload/tpce/egen/CE.h"
#include "workload/tpce/egen/TxnHarnessStructs.h"
#include "workload/tpce/shore_tpce_egen.h"

using namespace shore;
using namespace TPCE;

//#define TRACE_TRX_FLOW TRACE_ALWAYS

ENTER_NAMESPACE(tpce);

#ifdef COMPILE_FLAT_FILE_LOAD 
extern FILE * fssec;
extern FILE* fshs;
#endif

const int loadUnit = 10000;

int testCnt = 0;

TIdent lastTradeId = -1;
mcs_lock _trade_order_lock;

//buffers for Egen data
AccountPermissionBuffer accountPermissionBuffer (3015);
CustomerBuffer customerBuffer (1005);
CustomerAccountBuffer customerAccountBuffer (1005);
CustomerTaxrateBuffer  customerTaxrateBuffer (2010);
HoldingBuffer holdingBuffer(10000);
HoldingHistoryBuffer holdingHistoryBuffer(2*loadUnit);
HoldingSummaryBuffer holdingSummaryBuffer(6000);
WatchItemBuffer watchItemBuffer (iMaxItemsInWL*1020+5000);
WatchListBuffer watchListBuffer (1020);

BrokerBuffer brokerBuffer(100);
CashTransactionBuffer cashTransactionBuffer(loadUnit);
ChargeBuffer chargeBuffer(20);
CommissionRateBuffer commissionRateBuffer (245);
SettlementBuffer settlementBuffer(loadUnit);
TradeBuffer tradeBuffer(loadUnit);
TradeHistoryBuffer tradeHistoryBuffer(3*loadUnit);
TradeTypeBuffer tradeTypeBuffer (10);


CompanyBuffer companyBuffer (1000);
CompanyCompetitorBuffer companyCompetitorBuffer(3000);
DailyMarketBuffer dailyMarketBuffer(3000);
ExchangeBuffer exchangeBuffer(9);
FinancialBuffer financialBuffer (1500);
IndustryBuffer industryBuffer(107);
LastTradeBuffer lastTradeBuffer (1005);
NewsItemBuffer newsItemBuffer(200); 
NewsXRefBuffer newsXRefBuffer(200);//big
SectorBuffer sectorBuffer(17);
SecurityBuffer securityBuffer(1005);


AddressBuffer addressBuffer(1005);
StatusTypeBuffer statusTypeBuffer (10);
TaxrateBuffer taxrateBuffer (325);
ZipCodeBuffer zipCodeBuffer (14850);

/******************************************************************** 
 *
 * Thread-local TPC-E TRXS Stats
 *
 ********************************************************************/

static __thread ShoreTPCETrxStats my_stats;

void ShoreTPCEEnv::env_thread_init()
{
    CRITICAL_SECTION(stat_mutex_cs, _statmap_mutex);
    _statmap[pthread_self()] = &my_stats;
}

void ShoreTPCEEnv::env_thread_fini()
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

ShoreTPCETrxStats ShoreTPCEEnv::_get_stats()
{
    CRITICAL_SECTION(cs, _statmap_mutex);
    ShoreTPCETrxStats rval;
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

void ShoreTPCEEnv::reset_stats()
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

void ShoreTPCEEnv::print_throughput(const double iQueriedSF, 
                                    const int iSpread,
                                    const int iNumOfThreads,
                                    const double delay,
                                    const ulong_t mioch,
                                    const double avgcpuusage)
{
    CRITICAL_SECTION(last_stats_cs, _last_stats_mutex);

    // get the current statistics
    ShoreTPCETrxStats current_stats = _get_stats();

    // now calculate the diff
    current_stats -= _last_stats;

    int trxs_att  = current_stats.attempted.total();
    int trxs_abt  = current_stats.failed.total();
    int trxs_dld  = current_stats.deadlocked.total();

    TRACE( TRACE_ALWAYS, "*******\n"                \
           "Spread:    (%s)\n"                      \
           "Threads:   (%d)\n"                      \
           "Trxs Att:  (%d)\n"                      \
           "Trxs Abt:  (%d)\n"                      \
           "Trxs Dld:  (%d)\n"                      \
           "Secs:      (%.2f)\n"                    \
           "IOChars:   (%.2fM/s)\n"                 \
           "AvgCPUs:   (%.1f) (%.1f%%)\n"           \
           "TPS:       (%.2f)\n",
           (iSpread ? "Yes" : "No"),
           iNumOfThreads, trxs_att, trxs_abt, trxs_dld,
           delay, mioch/delay, avgcpuusage, 100*avgcpuusage/64,
           (trxs_att-trxs_abt-trxs_dld)/delay);
}

/******************************************************************** 
 *
 * TPC-E TRXS
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

w_rc_t ShoreTPCEEnv::run_one_xct(Request* prequest)
{
    //check if there is ready transaction initiated by market 
    if(!MarketFeedInputBuffer->isEmpty())  prequest->set_type(XCT_TPCE_MARKET_FEED);
    else if(!TradeResultInputBuffer->isEmpty()) prequest->set_type(XCT_TPCE_TRADE_RESULT);
    else
	if (prequest->type() == XCT_TPCE_MIX) {
            double rndm =  (1.0*(smthread_t::me()->rand()%10000))/100.0;
            if (rndm<0) rndm*=-1.0;
            prequest->set_type(random_xct_type(rndm));
	}

    switch (prequest->type()) {

    case XCT_TPCE_BROKER_VOLUME:
        return run_broker_volume(prequest);

    case XCT_TPCE_CUSTOMER_POSITION:
        return run_customer_position(prequest);

    case XCT_TPCE_MARKET_FEED:
        return run_market_feed(prequest);

    case XCT_TPCE_MARKET_WATCH:
        return run_market_watch(prequest);

    case XCT_TPCE_SECURITY_DETAIL:
        return run_security_detail(prequest);

    case XCT_TPCE_TRADE_LOOKUP:
        return run_trade_lookup(prequest);

    case XCT_TPCE_TRADE_ORDER:
        return run_trade_order(prequest);

    case XCT_TPCE_TRADE_RESULT:
        return run_trade_result(prequest);

    case XCT_TPCE_TRADE_STATUS:
        return run_trade_status(prequest);

    case XCT_TPCE_TRADE_UPDATE:
        return run_trade_update(prequest);

    case XCT_TPCE_DATA_MAINTENANCE:
  //      assert(false);
        return run_data_maintenance(prequest);

    case XCT_TPCE_TRADE_CLEANUP:
        return run_trade_cleanup(prequest);
    default:
        printf("************type %d**********\n ", prequest->type());
        assert (0); // UNKNOWN TRX-ID
    }
    return (RCOK);
}



/******************************************************************** 
 *
 * TPC-E Database Loading
 *
 ********************************************************************/

/*
  DATABASE POPULATION TRANSACTIONS

  The TPC-E database has 33 tables. Out of those:
  9 are fixed with following cardinalities:
  CHARGE		 		15  
  COMMISSION_RATE		  	240
  EXCHANGE				4 
  INDUSTRY				102 
  SECTOR				12 
  STATUS_TYPE				5 
  TAXRATE				320 
  TRADE_TYPE				5
  ZIP_CODE				14741 

  16 are scaling (size is proportional to the number of customers):
  CUSTOMER				1*customer_count
  CUSTOMER_TAXRATE      2*customer_count
  CUSTOMER_ACCOUNT      5*customer_count
  ACCOUNT_PERMISSION   ~7.1* customer_count 
  ADDRESS	        1.5*customer_count + 4  
  BROKER		0.01*customer_count
  COMPANY		0.5 * customer_count
  COMPANY_COMPETITOR    1.5* customer_count
  DAILY_MARKET		4,469,625 securities * 1,305 
  FINANCIAL		10*customer_count 
  LAST_TRADE		0.685*customer_count 
  NEWS_ITEM		1*customer_count
  NEWS_XREF		1*customer_count 
  SECURITY              0.685*customer_count
  WATCH_LIST		1*customer_count 
  WATCH_ITEM           ~100*customer_count

  8 growing tables, proportional to the following expression: (customer_count*initial_trading_days)/scaling_factor ratio!
  CASH_TRANSACTION		
  HOLDING 
  HOLDING_HISTORY 
  HOLDING_SUMMARY 
  SETTLEMENT 
  TRADE 
  TRADE_HISTORY 
  TRADE_REQUEST

*/



/******************************************************************** 
 *
 * These functions populate records for the TPC-E database. They do not
 * commit though. So, they need to be invoked inside a transaction
 * and the caller needs to commit at the end. 
 *
 ********************************************************************/


// Populates one SECTOR
w_rc_t ShoreTPCEEnv::_load_one_sector(rep_row_t& areprow,  PSECTOR_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _psector_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->SC_ID);
    pr->set_value(1, record->SC_NAME);

    e = _psector_man->add_tuple(_pssm, pr);

    _psector_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_sector()
{
    bool isLast= pGenerateAndLoad->isLastSector();
    while (!isLast){
        PSECTOR_ROW record = pGenerateAndLoad->getSectorRow();
        sectorBuffer.append(record);
        isLast= pGenerateAndLoad->isLastSector();
    }
    sectorBuffer.setMoreToRead(false);
}


// Populates one CHARGE
w_rc_t ShoreTPCEEnv::_load_one_charge(rep_row_t& areprow,  PCHARGE_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pcharge_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->CH_TT_ID);
    pr->set_value(1, (short)record->CH_C_TIER);
    pr->set_value(2, record->CH_CHRG);
    e = _pcharge_man->add_tuple(_pssm, pr);
    _pcharge_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_charge()
{
    bool isLast= pGenerateAndLoad->isLastCharge();
    while (!isLast){
        PCHARGE_ROW record = pGenerateAndLoad->getChargeRow();
        chargeBuffer.append(record);
        isLast= pGenerateAndLoad->isLastCharge();
    }
    chargeBuffer.setMoreToRead(false);
}


// Populates one COMMISSION_RATE

w_rc_t ShoreTPCEEnv::_load_one_commission_rate(rep_row_t& areprow, PCOMMISSION_RATE_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pcommission_rate_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, (short)(record->CR_C_TIER)); //int in EGEN
    pr->set_value(1, record->CR_TT_ID);
    pr->set_value(2, record->CR_EX_ID);
    pr->set_value(3, (int)(record->CR_FROM_QTY)); //double in egen
    pr->set_value(4, (int)(record->CR_TO_QTY)); //double in egen
    pr->set_value(5, record->CR_RATE);
    /*
      printf("\n%s",  record->TX_ID);
      printf("\n%s",  record->TX_NAME);
      printf("\n%lf\n",  record->TX_RATE);
    */	

    e = _pcommission_rate_man->add_tuple(_pssm, pr);
    _pcommission_rate_man->give_tuple(pr);
	
    /*	printf("\n%s",  record->TX_ID);
	printf("\n%s",  record->TX_NAME);
	printf("\n%lf\n",  record->TX_RATE);
    */	
    return (e);
}

void ShoreTPCEEnv::_read_commission_rate()
{
    bool isLast= pGenerateAndLoad->isLastCommissionRate();
    while (!isLast){
        PCOMMISSION_RATE_ROW record = pGenerateAndLoad->getCommissionRateRow();
        commissionRateBuffer.append(record);
        isLast= pGenerateAndLoad->isLastCommissionRate();
    }
    commissionRateBuffer.setMoreToRead(false);
}


// Populates one EXCHANGE
w_rc_t ShoreTPCEEnv::_load_one_exchange(rep_row_t& areprow,  PEXCHANGE_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pexchange_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->EX_ID);
    pr->set_value(1, record->EX_NAME);
    pr->set_value(2, record->EX_NUM_SYMB);
    pr->set_value(3, record->EX_OPEN);
    pr->set_value(4, record->EX_CLOSE);
    pr->set_value(5, record->EX_DESC);
    pr->set_value(6, record->EX_AD_ID );
    e = _pexchange_man->add_tuple(_pssm, pr);
    _pexchange_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_exchange()
{
    bool isLast= pGenerateAndLoad->isLastExchange();
    while (!isLast){
	assert(testCnt<10);
        PEXCHANGE_ROW record = pGenerateAndLoad->getExchangeRow();
	exchangeBuffer.append(record);    
        isLast= pGenerateAndLoad->isLastExchange();
    }
    exchangeBuffer.setMoreToRead(false);
}

// Populates one INDUSTRY
w_rc_t ShoreTPCEEnv::_load_one_industry(rep_row_t& areprow,  PINDUSTRY_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pindustry_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->IN_ID);
    pr->set_value(1, record->IN_NAME);
    pr->set_value(2, record->IN_SC_ID);

    e = _pindustry_man->add_tuple(_pssm, pr);
    _pindustry_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_industry()
{
    bool isLast= pGenerateAndLoad->isLastIndustry();
    while (!isLast){
        PINDUSTRY_ROW record = pGenerateAndLoad->getIndustryRow();
        industryBuffer.append(record);
        isLast= pGenerateAndLoad->isLastIndustry();
    }
    industryBuffer.setMoreToRead(false);
}

// Populates one STATUS_TYPE
w_rc_t ShoreTPCEEnv::_load_one_status_type(rep_row_t& areprow,  PSTATUS_TYPE_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pstatus_type_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->ST_ID);
    pr->set_value(1, record->ST_NAME);

    e = _pstatus_type_man->add_tuple(_pssm, pr);
    _pstatus_type_man->give_tuple(pr);

    return (e);
}

void ShoreTPCEEnv::_read_status_type()
{
    bool isLast= pGenerateAndLoad->isLastStatusType();
    while (!isLast){
        PSTATUS_TYPE_ROW record = pGenerateAndLoad->getStatusTypeRow();
        statusTypeBuffer.append(record);
        isLast= pGenerateAndLoad->isLastStatusType();
    }
    statusTypeBuffer.setMoreToRead(false);
}


// Populates one TAXRATE
w_rc_t ShoreTPCEEnv::_load_one_taxrate(rep_row_t& areprow,  PTAXRATE_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _ptaxrate_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->TX_ID);
    pr->set_value(1, record->TX_NAME);
    pr->set_value(2, (record->TX_RATE));
    e = _ptaxrate_man->add_tuple(_pssm, pr);
    _ptaxrate_man->give_tuple(pr);

    return (e);
}

void ShoreTPCEEnv::_read_taxrate()
{
    bool hasNext;
    do{
        PTAXRATE_ROW record = pGenerateAndLoad->getTaxrateRow();
        taxrateBuffer.append(record);
        hasNext= pGenerateAndLoad->hasNextTaxrate();
    }while(hasNext);
    taxrateBuffer.setMoreToRead(false);
}

// Populates one TRADE_TYPE
w_rc_t ShoreTPCEEnv::_load_one_trade_type(rep_row_t& areprow,  PTRADE_TYPE_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _ptrade_type_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->TT_ID);
    pr->set_value(1, record->TT_NAME);
    pr->set_value(2, (bool)record->TT_IS_SELL);
    pr->set_value(3, (bool)record->TT_IS_MRKT);
    e = _ptrade_type_man->add_tuple(_pssm, pr);
    _ptrade_type_man->give_tuple(pr);
    return (e);
}


void ShoreTPCEEnv::_read_trade_type()
{
    bool isLast= pGenerateAndLoad->isLastTradeType();
    while (!isLast){
        PTRADE_TYPE_ROW record = pGenerateAndLoad->getTradeTypeRow();
        tradeTypeBuffer.append(record);
        isLast= pGenerateAndLoad->isLastTradeType();
    }
    tradeTypeBuffer.setMoreToRead(false);
}


// Populates one ZIP_CODE
w_rc_t ShoreTPCEEnv::_load_one_zip_code(rep_row_t& areprow,  PZIP_CODE_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pzip_code_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->ZC_CODE);
    pr->set_value(1, record->ZC_TOWN);
    pr->set_value(2, record->ZC_DIV);

    e = _pzip_code_man->add_tuple(_pssm, pr);
    _pzip_code_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_zip_code()
{
    bool hasNext= pGenerateAndLoad->hasNextZipCode();
    while (hasNext){
        PZIP_CODE_ROW record = pGenerateAndLoad->getZipCodeRow();
        zipCodeBuffer.append(record);
        hasNext= pGenerateAndLoad->hasNextZipCode();
    }
    zipCodeBuffer.setMoreToRead(false);
}

//customer-related tables		

// Populates one CUSTOMER
w_rc_t ShoreTPCEEnv::_load_one_customer(rep_row_t& areprow, PCUSTOMER_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pcustomer_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->C_ID);
    pr->set_value(1, record->C_TAX_ID);
    pr->set_value(2, record->C_ST_ID);
    pr->set_value(3, record->C_L_NAME);
    pr->set_value(4, record->C_F_NAME);
    pr->set_value(5, record->C_M_NAME);

    char xxz[2];
    xxz[0]=record->C_GNDR;
    xxz[1]='\0';

    pr->set_value(6, xxz);
    pr->set_value(7,  (short)record->C_TIER);
    pr->set_value(8,  (long long) EgenTimeToTimeT(record->C_DOB));
    pr->set_value(9,  record->C_AD_ID);
    pr->set_value(10, record->C_CTRY_1);
    pr->set_value(11, record->C_AREA_1);
    pr->set_value(12, record->C_LOCAL_1);
    pr->set_value(13, record->C_EXT_1);
    pr->set_value(14, record->C_CTRY_2);
    pr->set_value(15, record->C_AREA_2);
    pr->set_value(16, record->C_LOCAL_2);
    pr->set_value(17, record->C_EXT_2);
    pr->set_value(18, record->C_CTRY_3);
    pr->set_value(19, record->C_AREA_3);
    pr->set_value(20, record->C_LOCAL_3);
    pr->set_value(21, record->C_EXT_3);
    pr->set_value(22, record->C_EMAIL_1);
    pr->set_value(23, record->C_EMAIL_2);
    e = _pcustomer_man->add_tuple(_pssm, pr);
    _pcustomer_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_customer()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextCustomer();
        PCUSTOMER_ROW record = pGenerateAndLoad->getCustomerRow();
        customerBuffer.append(record);
    }while ((hasNext && customerBuffer.hasSpace()));
    customerBuffer.setMoreToRead(hasNext);
}



// Populates one CUSTOMER_TAXRATE
w_rc_t ShoreTPCEEnv::_load_one_customer_taxrate(rep_row_t& areprow,  PCUSTOMER_TAXRATE_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pcustomer_taxrate_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;


    pr->set_value(0, record->CX_TX_ID);
    pr->set_value(1, record->CX_C_ID);

    e = _pcustomer_taxrate_man->add_tuple(_pssm, pr);
    _pcustomer_taxrate_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_customer_taxrate()
{
    bool hasNext;
    int taxrates=pGenerateAndLoad->getTaxratesCount();
    do {
        hasNext= pGenerateAndLoad->hasNextCustomerTaxrate();
        for(int i=0; i<taxrates; i++)
            {
                PCUSTOMER_TAXRATE_ROW record = pGenerateAndLoad->getCustomerTaxrateRow(i);
                customerTaxrateBuffer.append(record);
            }
    }while ((hasNext && customerTaxrateBuffer.hasSpace()));

    customerTaxrateBuffer.setMoreToRead(hasNext);
}


// Populates one CUSTOMER_ACCOUNT
w_rc_t ShoreTPCEEnv::_load_one_customer_account(rep_row_t& areprow,  PCUSTOMER_ACCOUNT_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pcustomer_account_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->CA_ID);
    pr->set_value(1, record->CA_B_ID);
    pr->set_value(2, record->CA_C_ID);
    pr->set_value(3, record->CA_NAME);
    pr->set_value(4, (short)(record->CA_TAX_ST));
    pr->set_value(5, record->CA_BAL);

    e = _pcustomer_account_man->add_tuple(_pssm, pr);
    _pcustomer_account_man->give_tuple(pr);
    return (e);
}

// Populates one ACCOUNT_PERMISSION
w_rc_t ShoreTPCEEnv::_load_one_account_permission(rep_row_t& areprow, PACCOUNT_PERMISSION_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _paccount_permission_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;
    pr->set_value(0, record->AP_CA_ID);
    pr->set_value(1, record->AP_ACL);
    pr->set_value(2, record->AP_TAX_ID);
    pr->set_value(3, record->AP_L_NAME);
    pr->set_value(4, record->AP_F_NAME);

    e = _paccount_permission_man->add_tuple(_pssm, pr);

    _paccount_permission_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_ca_and_ap()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextCustomerAccount();
        PCUSTOMER_ACCOUNT_ROW record = pGenerateAndLoad->getCustomerAccountRow();
        customerAccountBuffer.append(record);
        int perms = pGenerateAndLoad->PermissionsPerCustomer();
        for(int i=0; i<perms; i++)
            {
                PACCOUNT_PERMISSION_ROW row = pGenerateAndLoad->getAccountPermissionRow(i);
                accountPermissionBuffer.append(row);
            }
    }while ((hasNext && customerAccountBuffer.hasSpace()));

    customerAccountBuffer.setMoreToRead(hasNext);
}


// Populates one ADDRESS
w_rc_t ShoreTPCEEnv::_load_one_address(rep_row_t& areprow, PADDRESS_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _paddress_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;
    pr->set_value(0, record->AD_ID);
    pr->set_value(1, record->AD_LINE1);
    pr->set_value(2, record->AD_LINE2);
    pr->set_value(3, record->AD_ZC_CODE);
    pr->set_value(4, record->AD_CTRY);

    e = _paddress_man->add_tuple(_pssm, pr);
    _paddress_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_address()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextAddress();
        PADDRESS_ROW record = pGenerateAndLoad->getAddressRow();
        addressBuffer.append(record);
    }while ((hasNext && addressBuffer.hasSpace()));
    addressBuffer.setMoreToRead(hasNext);
}



// Populates one WATCH_LIST
w_rc_t ShoreTPCEEnv::_load_one_watch_list(rep_row_t& areprow,  PWATCH_LIST_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pwatch_list_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;
    pr->set_value(0, record->WL_ID);
    pr->set_value(1, record->WL_C_ID);
    e = _pwatch_list_man->add_tuple(_pssm, pr);
    _pwatch_list_man->give_tuple(pr);
    return (e);
}


// Populates one WATCH_ITEM
w_rc_t ShoreTPCEEnv::_load_one_watch_item(rep_row_t& areprow,  PWATCH_ITEM_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pwatch_item_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;
    pr->set_value(0, record->WI_WL_ID);
    pr->set_value(1, record->WI_S_SYMB);
    e = _pwatch_item_man->add_tuple(_pssm, pr);
    _pwatch_item_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_wl_and_wi()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextWatchList();
        PWATCH_LIST_ROW record = pGenerateAndLoad->getWatchListRow();
        watchListBuffer.append(record);
        int items = pGenerateAndLoad->ItemsPerWatchList();
        for(int i=0; i<items; ++i)
            {
                PWATCH_ITEM_ROW row = pGenerateAndLoad->getWatchItemRow(i);
                watchItemBuffer.append(row);
            }
    }while (hasNext && watchListBuffer.hasSpace());

    watchListBuffer.setMoreToRead(hasNext);
}


/* logic

   do
   {
   bRet = Table.GenerateNextRecord();

   pWatchListsLoad->WriteNextRecord(Table.GetWLRow());

   for (i=0; i<Table.GetWICount(); ++i)
   {
   pWatchItemsLoad->WriteNextRecord(Table.GetWIRow(i));

   if (++iCnt % 20000 == 0)
   {
   m_pOutput->OutputProgress("."); //output progress

   pWatchListsLoad->Commit();  //commit
   pWatchItemsLoad->Commit();  //commit
   }
   }
   } while (bRet);
*/



// Now security/company related tables

// Populates one COMPANY
w_rc_t ShoreTPCEEnv::_load_one_company(rep_row_t& areprow, PCOMPANY_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pcompany_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->CO_ID);
    pr->set_value(1, record->CO_ST_ID);
    pr->set_value(2, record->CO_NAME);
    pr->set_value(3, record->CO_IN_ID);
    pr->set_value(4, record->CO_SP_RATE);
    pr->set_value(5, record->CO_CEO);
    pr->set_value(6, record->CO_AD_ID);
    pr->set_value(7, record->CO_DESC);
    pr->set_value(8, EgenTimeToTimeT(record->CO_OPEN_DATE));
    e = _pcompany_man->add_tuple(_pssm, pr);
    _pcompany_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_company()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextCompany();
        PCOMPANY_ROW record = pGenerateAndLoad->getCompanyRow();
        companyBuffer.append(record);
    }while ((hasNext && companyBuffer.hasSpace()));
    companyBuffer.setMoreToRead(hasNext);
}


// Populates one COMPANY_COMPETITOR
w_rc_t ShoreTPCEEnv::_load_one_company_competitor(rep_row_t& areprow,  PCOMPANY_COMPETITOR_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pcompany_competitor_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;
    pr->set_value(0, (record->CP_CO_ID));
    pr->set_value(1, record->CP_COMP_CO_ID);
    pr->set_value(2, record->CP_IN_ID);

    e = _pcompany_competitor_man->add_tuple(_pssm, pr);
    _pcompany_competitor_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_company_competitor()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextCompanyCompetitor();
        PCOMPANY_COMPETITOR_ROW record = pGenerateAndLoad->getCompanyCompetitorRow();
        companyCompetitorBuffer.append(record);
    }while ((hasNext && companyCompetitorBuffer.hasSpace()));
    companyCompetitorBuffer.setMoreToRead(hasNext);
}


// Populates one DAILY_MARKET
w_rc_t ShoreTPCEEnv::_load_one_daily_market(rep_row_t& areprow,  PDAILY_MARKET_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pdaily_market_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, EgenTimeToTimeT(record->DM_DATE));
    pr->set_value(1, record->DM_S_SYMB);
    pr->set_value(2, record->DM_CLOSE);
    pr->set_value(3, record->DM_HIGH);
    pr->set_value(4, record->DM_LOW);
    pr->set_value(5, (int)record->DM_VOL); //double in EGEN

    e = _pdaily_market_man->add_tuple(_pssm, pr);
    _pdaily_market_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_daily_market()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextDailyMarket();
        PDAILY_MARKET_ROW record = pGenerateAndLoad->getDailyMarketRow();
        dailyMarketBuffer.append(record);
    }while ((hasNext && dailyMarketBuffer.hasSpace()));
    dailyMarketBuffer.setMoreToRead(hasNext);
}


// Populates one FINANCIAL
w_rc_t ShoreTPCEEnv::_load_one_financial(rep_row_t& areprow,  PFINANCIAL_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pfinancial_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;
    pr->set_value(0, record->FI_CO_ID);
    pr->set_value(1, record->FI_YEAR);
    pr->set_value(2, (short)record->FI_QTR); //int in Egen
    pr->set_value(3,  EgenTimeToTimeT(record->FI_QTR_START_DATE));
    pr->set_value(4,  record->FI_REVENUE);
    pr->set_value(5,  record->FI_NET_EARN);
    pr->set_value(6,  record->FI_BASIC_EPS);
    pr->set_value(7,  record->FI_DILUT_EPS);
    pr->set_value(8,  record->FI_MARGIN);
    pr->set_value(9,  record->FI_INVENTORY);
    pr->set_value(10, record->FI_ASSETS);
    pr->set_value(11, record->FI_LIABILITY);
    pr->set_value(12, record->FI_OUT_BASIC);
    pr->set_value(13, record->FI_OUT_DILUT);

    e = _pfinancial_man->add_tuple(_pssm, pr);
    _pfinancial_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_financial()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextFinancial();
        PFINANCIAL_ROW record = pGenerateAndLoad->getFinancialRow();
        financialBuffer.append(record);
    }while ((hasNext && financialBuffer.hasSpace()));
    financialBuffer.setMoreToRead(hasNext);
}

// Populates one LAST_TRADE
w_rc_t ShoreTPCEEnv::_load_one_last_trade(rep_row_t& areprow,  PLAST_TRADE_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _plast_trade_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->LT_S_SYMB);
    pr->set_value(1, EgenTimeToTimeT(record->LT_DTS));
    pr->set_value(2, record->LT_PRICE);
    pr->set_value(3, record->LT_OPEN_PRICE);
    pr->set_value(4, (double)record->LT_VOL); //int in Egen, INT64 in new Egen

    e = _plast_trade_man->add_tuple(_pssm, pr);
    _plast_trade_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_last_trade()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextLastTrade();
        PLAST_TRADE_ROW record = pGenerateAndLoad->getLastTradeRow();
        lastTradeBuffer.append(record);
    }while ((hasNext && lastTradeBuffer.hasSpace()));
    lastTradeBuffer.setMoreToRead(hasNext);
}


// Populates one NEWS_ITEM
w_rc_t ShoreTPCEEnv::_load_one_news_item(rep_row_t& areprow,  PNEWS_ITEM_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pnews_item_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;
    pr->set_value(0, record->NI_ID);
    pr->set_value(1, record->NI_HEADLINE);
    pr->set_value(2, record->NI_SUMMARY);
    char ni[max_news_item_size+1]; ni[max_news_item_size] = '\0';
    memcpy(ni,record->NI_ITEM,max_news_item_size);
    pr->set_value(3, ni);
    pr->set_value(4, EgenTimeToTimeT(record->NI_DTS));
    pr->set_value(5, record->NI_SOURCE);
    pr->set_value(6, record->NI_AUTHOR);

    e = _pnews_item_man->add_tuple(_pssm, pr);
    _pnews_item_man->give_tuple(pr);
    return (e);
}

// Populates one NEWS_XREF
w_rc_t ShoreTPCEEnv::_load_one_news_xref(rep_row_t& areprow,  PNEWS_XREF_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pnews_xref_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;
    pr->set_value(0, record->NX_NI_ID);
    pr->set_value(1, record->NX_CO_ID);

    e = _pnews_xref_man->add_tuple(_pssm, pr);
    _pnews_xref_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_ni_and_nx()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextNewsItemAndNewsXRef();
        PNEWS_ITEM_ROW record1 = pGenerateAndLoad->getNewsItemRow();
        PNEWS_XREF_ROW record2 = pGenerateAndLoad->getNewsXRefRow();
        newsItemBuffer.append(record1);
        newsXRefBuffer.append(record2);
    }while ((hasNext && newsItemBuffer.hasSpace()));
    newsItemBuffer.setMoreToRead(hasNext);
    newsXRefBuffer.setMoreToRead(hasNext);
}



// Populates one SECURITY
w_rc_t ShoreTPCEEnv::_load_one_security(rep_row_t& areprow,  PSECURITY_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _psecurity_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->S_SYMB);
    pr->set_value(1, record->S_ISSUE);
    pr->set_value(2, record->S_ST_ID);
    pr->set_value(3, record->S_NAME);
    pr->set_value(4, record->S_EX_ID);
    pr->set_value(5, record->S_CO_ID);
    pr->set_value(6, record->S_NUM_OUT);
    pr->set_value(7, EgenTimeToTimeT(record->S_START_DATE));
    pr->set_value(8, EgenTimeToTimeT(record->S_EXCH_DATE));
    pr->set_value(9, record->S_PE);
    pr->set_value(10, (record->S_52WK_HIGH));
    pr->set_value(11, EgenTimeToTimeT(record->S_52WK_HIGH_DATE));
    pr->set_value(12, (record->S_52WK_LOW));
    pr->set_value(13, EgenTimeToTimeT(record->S_52WK_LOW_DATE));

    pr->set_value(14, (record->S_DIVIDEND));
    pr->set_value(15, (record->S_YIELD));
#ifdef COMPILE_FLAT_FILE_LOAD 
    fprintf(fssec,"%s|%s|%s|%s\n",record->S_SYMB, record->S_ISSUE, record->S_ST_ID, record->S_NAME); 
#endif
    e = _psecurity_man->add_tuple(_pssm, pr);
    _psecurity_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_security()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextSecurity();
        PSECURITY_ROW record = pGenerateAndLoad->getSecurityRow();
        securityBuffer.append(record);
    }while ((hasNext && securityBuffer.hasSpace()));
    securityBuffer.setMoreToRead(hasNext);
}



//growing tables + broker

// Populates one TRADE
w_rc_t ShoreTPCEEnv::_load_one_trade(rep_row_t& areprow,  PTRADE_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _ptrade_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->T_ID);
    pr->set_value(1, EgenTimeToTimeT(record->T_DTS));
    pr->set_value(2, record->T_ST_ID);
    pr->set_value(3, record->T_TT_ID);
    pr->set_value(4, (bool)record->T_IS_CASH);
    pr->set_value(5, record->T_S_SYMB);
    pr->set_value(6, record->T_QTY);
    pr->set_value(7, (record->T_BID_PRICE));
    pr->set_value(8, record->T_CA_ID);
    pr->set_value(9, record->T_EXEC_NAME);
    pr->set_value(10, (record->T_TRADE_PRICE));
    pr->set_value(11, (record->T_CHRG));
    pr->set_value(12, (record->T_COMM));
    pr->set_value(13, (record->T_TAX));
    pr->set_value(14, (bool)record->T_LIFO);


    e = _ptrade_man->add_tuple(_pssm, pr);
    _ptrade_man->give_tuple(pr);
    return (e);
}

// Populates one TRADE_HISTORY
w_rc_t ShoreTPCEEnv::_load_one_trade_history(rep_row_t& areprow,  PTRADE_HISTORY_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _ptrade_history_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->TH_T_ID);
    pr->set_value(1, EgenTimeToTimeT(record->TH_DTS));
    pr->set_value(2, record->TH_ST_ID);

    e = _ptrade_history_man->add_tuple(_pssm, pr);
    _ptrade_history_man->give_tuple(pr);
    return (e);
}

// Populates one SETTLEMENT
w_rc_t ShoreTPCEEnv::_load_one_settlement(rep_row_t& areprow, PSETTLEMENT_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _psettlement_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;
    pr->set_value(0, record->SE_T_ID);
    pr->set_value(1, record->SE_CASH_TYPE);
    pr->set_value(2, EgenTimeToTimeT(record->SE_CASH_DUE_DATE));
    pr->set_value(3, record->SE_AMT);
    e = _psettlement_man->add_tuple(_pssm, pr);

    _psettlement_man->give_tuple(pr);
    return (e);
}

// Populates one CASH_TRANSACTION
w_rc_t ShoreTPCEEnv::_load_one_cash_transaction(rep_row_t& areprow, PCASH_TRANSACTION_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pcash_transaction_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->CT_T_ID);
    pr->set_value(1, EgenTimeToTimeT(record->CT_DTS));
    pr->set_value(2, record->CT_AMT);
    pr->set_value(3, record->CT_NAME);
    e = _pcash_transaction_man->add_tuple(_pssm, pr);
    _pcash_transaction_man->give_tuple(pr);
    return (e);
}

// Populates one HOLDING_HISTORY
w_rc_t ShoreTPCEEnv::_load_one_holding_history(rep_row_t& areprow,  PHOLDING_HISTORY_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pholding_history_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->HH_H_T_ID);
    pr->set_value(1, record->HH_T_ID);
    pr->set_value(2, record->HH_BEFORE_QTY);
    pr->set_value(3, record->HH_AFTER_QTY);

    e = _pholding_history_man->add_tuple(_pssm, pr);
    _pholding_history_man->give_tuple(pr);
    return (e);
}


void ShoreTPCEEnv::_read_trade_unit()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextTrade();
        PTRADE_ROW row = pGenerateAndLoad->getTradeRow();
        tradeBuffer.append(row);
        int hist = pGenerateAndLoad->getTradeHistoryRowCount();
        for(int i=0; i<hist; i++)
            {
                PTRADE_HISTORY_ROW record = pGenerateAndLoad->getTradeHistoryRow(i);
                tradeHistoryBuffer.append(record);
            }
        if(pGenerateAndLoad->shouldProcessSettlementRow())
            {
                PSETTLEMENT_ROW record = pGenerateAndLoad->getSettlementRow();
                settlementBuffer.append(record);
            }

        if(pGenerateAndLoad->shouldProcessCashTransactionRow())
            {
                PCASH_TRANSACTION_ROW record = pGenerateAndLoad->getCashTransactionRow();
                cashTransactionBuffer.append(record);
            }

        hist = pGenerateAndLoad->getHoldingHistoryRowCount();
        for(int i=0; i<hist; i++)
            {
                PHOLDING_HISTORY_ROW record = pGenerateAndLoad->getHoldingHistoryRow(i);
                holdingHistoryBuffer.append(record);
            }

    }while ((hasNext && tradeBuffer.hasSpace()));
    tradeBuffer.setMoreToRead(hasNext);
}


// Populates one BROKER
w_rc_t ShoreTPCEEnv::_load_one_broker(rep_row_t& areprow, PBROKER_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pbroker_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->B_ID);
    pr->set_value(1, record->B_ST_ID);
    pr->set_value(2, record->B_NAME);
    pr->set_value(3, record->B_NUM_TRADES);
    pr->set_value(4, (record->B_COMM_TOTAL));

    e = _pbroker_man->add_tuple(_pssm, pr);
    _pbroker_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_broker()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextBroker();
        PBROKER_ROW record = pGenerateAndLoad->getBrokerRow();
        brokerBuffer.append(record);
    }while ((hasNext && brokerBuffer.hasSpace()));
    brokerBuffer.setMoreToRead(hasNext);
}


// Populates one HOLDING
w_rc_t ShoreTPCEEnv::_load_one_holding(rep_row_t& areprow,  PHOLDING_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pholding_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0,record->H_T_ID);
    pr->set_value(1, record->H_CA_ID);
    pr->set_value(2,record->H_S_SYMB);
    pr->set_value(3, EgenTimeToTimeT(record->H_DTS));
    pr->set_value(4, record->H_PRICE);
    pr->set_value(5, record->H_QTY);

    e = _pholding_man->add_tuple(_pssm, pr);
    _pholding_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_holding()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextHolding();
        PHOLDING_ROW record = pGenerateAndLoad->getHoldingRow();
        holdingBuffer.append(record);
    }while ((hasNext && holdingBuffer.hasSpace()));
    holdingBuffer.setMoreToRead(hasNext);
}


// Populates one HOLDING_SUMMARY
w_rc_t ShoreTPCEEnv::_load_one_holding_summary(rep_row_t& areprow,  PHOLDING_SUMMARY_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _pholding_summary_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0,  record->HS_CA_ID);
    //printf(" %s \t",record->HS_S_SYMB ); //cansu
    pr->set_value(1,  record->HS_S_SYMB);
    pr->set_value(2,  record->HS_QTY);
#ifdef COMPILE_FLAT_FILE_LOAD 
    fprintf(fshs,"%lld|%s|%d\n",record->HS_CA_ID, record->HS_S_SYMB, record->HS_QTY); 
#endif

    e = _pholding_summary_man->add_tuple(_pssm, pr);
    _pholding_summary_man->give_tuple(pr);
    return (e);
}

void ShoreTPCEEnv::_read_holding_summary()
{
    bool hasNext;
    do {
        hasNext= pGenerateAndLoad->hasNextHoldingSummary();
        PHOLDING_SUMMARY_ROW record = pGenerateAndLoad->getHoldingSummaryRow();
        holdingSummaryBuffer.append(record);
    }while ((hasNext && holdingSummaryBuffer.hasSpace()));
    holdingSummaryBuffer.setMoreToRead(hasNext);
}

// Populates one TRADE_REQUEST
w_rc_t ShoreTPCEEnv::_load_one_trade_request(rep_row_t& areprow,  PTRADE_REQUEST_ROW record)
{    
    w_rc_t e = RCOK;
    table_row_t*   pr = _ptrade_request_man->get_tuple();
    assert (pr);
    pr->_rep = &areprow;

    pr->set_value(0, record->TR_T_ID);
    pr->set_value(1, record->TR_TT_ID);
    pr->set_value(2, record->TR_S_SYMB);
    pr->set_value(3, record->TR_QTY);
    pr->set_value(4, (record->TR_BID_PRICE));
    pr->set_value(5, record->TR_B_ID); //named incorrectly in egen
    e = _ptrade_request_man->add_tuple(_pssm, pr);
    _ptrade_request_man->give_tuple(pr);
    return (e);
}


/******************************************************************** 
 *
 * TPC-E Loading: Population transactions
 *
 ********************************************************************/

//populating small tables
w_rc_t 
ShoreTPCEEnv::xct_populate_small(const int xct_id, populate_small_input_t& ptoin)
{
    int rows;

    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);

    // The exchange has the biggest row all the fixed tables
    rep_row_t areprow(_pexchange_man->ts());
    areprow.set(_pexchange_desc->maxsize());

    w_rc_t e = RCOK;

    // 2. Build the small tables

    TRACE( TRACE_ALWAYS, "Building CHARGE !!!\n");
    rows=chargeBuffer.getSize();
    for(int i=0; i<rows; i++){
        PCHARGE_ROW record = chargeBuffer.get(i);
        e=_load_one_charge(areprow, record);
        if(e.is_error())  return (e);
    }
    pGenerateAndLoad->ReleaseCharge();

    TRACE( TRACE_ALWAYS, "Building COMMISSION_RATE !!!\n");
    rows=commissionRateBuffer.getSize();
    for(int i=0; i<rows; i++){
        PCOMMISSION_RATE_ROW record = commissionRateBuffer.get(i);
        e=_load_one_commission_rate(areprow, record);
        if(e.is_error())  return (e);
    }
    pGenerateAndLoad->ReleaseCommissionRate();


    TRACE( TRACE_ALWAYS, "Building EXCHANGE !!!\n");
    rows=exchangeBuffer.getSize();
    for(int i=0; i<rows; i++){
        PEXCHANGE_ROW record = exchangeBuffer.get(i);
        e=_load_one_exchange(areprow, record);
        if(e.is_error())  return (e);
    }
    pGenerateAndLoad->ReleaseExchange();




    TRACE( TRACE_ALWAYS, "Building INDUSTRY !!!\n");
    rows=industryBuffer.getSize();
    for(int i=0; i<rows; i++){
        PINDUSTRY_ROW record = industryBuffer.get(i);
        e=_load_one_industry(areprow, record);
        if(e.is_error()) { return (e); }
    }
    pGenerateAndLoad->ReleaseIndustry();


    TRACE( TRACE_ALWAYS, "Building SECTOR !!!\n");
    rows=sectorBuffer.getSize();
    for(int i=0; i<rows; i++){
        PSECTOR_ROW record = sectorBuffer.get(i);
        e=_load_one_sector(areprow, record);
        if(e.is_error()) { return (e); }
    }
    pGenerateAndLoad->ReleaseSector();

	
    TRACE( TRACE_ALWAYS, "Building STATUS_TYPE !!!\n");
    rows=statusTypeBuffer.getSize();
    for(int i=0; i<rows; i++){
        PSTATUS_TYPE_ROW record = statusTypeBuffer.get(i);
        e=_load_one_status_type(areprow, record);
        if(e.is_error()) { return (e); }
    }
    pGenerateAndLoad->ReleaseStatusType();

    TRACE( TRACE_ALWAYS, "Building TAXRATE !!!\n");
    rows=taxrateBuffer.getSize();
    for(int i=0; i<rows; i++){
        PTAXRATE_ROW record = taxrateBuffer.get(i);
        e=_load_one_taxrate(areprow, record);
        if(e.is_error()) { return (e); }
    }
    pGenerateAndLoad->ReleaseTaxrate();

    TRACE( TRACE_ALWAYS, "Building TRADE_TYPE !!!\n");
    rows=tradeTypeBuffer.getSize();
    for(int i=0; i<rows; i++){
        PTRADE_TYPE_ROW record = tradeTypeBuffer.get(i);
        e=_load_one_trade_type(areprow, record);
        if(e.is_error()) { return (e); }
    }
    pGenerateAndLoad->ReleaseTradeType();

    TRACE( TRACE_ALWAYS, "Building ZIP CODE !!!\n");
    rows=zipCodeBuffer.getSize();
    for(int i=0; i<rows; i++){
        PZIP_CODE_ROW record = zipCodeBuffer.get(i);
        e=_load_one_zip_code(areprow, record);
        if(e.is_error()) { return (e); }
    }
    pGenerateAndLoad->ReleaseZipCode();

    //commit
    e = _pssm->commit_xct();
    return (e);
}


//customer
w_rc_t ShoreTPCEEnv::xct_populate_customer(const int xct_id, populate_customer_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pcustomer_man->ts());
    areprow.set(_pcustomer_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=customerBuffer.getSize();
    for(int i=0; i<rows; i++){
        PCUSTOMER_ROW record = customerBuffer.get(i);
        e=_load_one_customer(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_customer()
{	
    pGenerateAndLoad->InitCustomer();
    TRACE( TRACE_ALWAYS, "Building CUSTOMER !!!\n");
    w_rc_t e = RCOK;
    while(customerBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        customerBuffer.reset();
        _read_customer();
        populate_customer_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_customer(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseCustomer();
    customerBuffer.release();
}


//address
w_rc_t ShoreTPCEEnv::xct_populate_address(const int xct_id, populate_address_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_paddress_man->ts());
    areprow.set(_paddress_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=addressBuffer.getSize();
    for(int i=0; i<rows; i++){
        PADDRESS_ROW record = addressBuffer.get(i);
        e=_load_one_address(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_address()
{	
    pGenerateAndLoad->InitAddress();
    TRACE( TRACE_ALWAYS, "Building ADDRESS !!!\n");
    w_rc_t e = RCOK;
    while(addressBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        addressBuffer.reset();
        _read_address();
        populate_address_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_address(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseAddress();
    addressBuffer.release();
}

//CustomerAccount and AccountPermission
w_rc_t ShoreTPCEEnv::xct_populate_ca_and_ap(const int xct_id, populate_ca_and_ap_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pcustomer_account_man->ts());
    areprow.set(_pcustomer_account_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=customerAccountBuffer.getSize();
    for(int i=0; i<rows; i++){
        PCUSTOMER_ACCOUNT_ROW record = customerAccountBuffer.get(i);
        e=_load_one_customer_account(areprow, record);
        if(e.is_error()) { return (e); }
    }
    rows=accountPermissionBuffer.getSize();
    for(int i=0; i<rows; i++){
        PACCOUNT_PERMISSION_ROW record = accountPermissionBuffer.get(i);
        e=_load_one_account_permission(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_ca_and_ap()
{	pGenerateAndLoad->InitCustomerAccountAndAccountPermission();
    TRACE( TRACE_ALWAYS, "Building CustomerAccount and AccountPermission !!!\n");

    w_rc_t e = RCOK;
    while(customerAccountBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        customerAccountBuffer.reset();
        accountPermissionBuffer.reset();
        _read_ca_and_ap();
        populate_ca_and_ap_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_ca_and_ap(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseCustomerAccountAndAccountPermission();
    customerAccountBuffer.release();
    accountPermissionBuffer.release();
}


//Watch List and Watch Item
w_rc_t ShoreTPCEEnv::xct_populate_wl_and_wi(const int xct_id, populate_wl_and_wi_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pwatch_item_man->ts());
    areprow.set(_pwatch_item_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=watchListBuffer.getSize();
    for(int i=0; i<rows; i++){
        PWATCH_LIST_ROW record = watchListBuffer.get(i);
        e=_load_one_watch_list(areprow, record);
        if(e.is_error()) { return (e); }
    }

    int rows2=watchItemBuffer.getSize();
    for(int i=0; i<rows2; i++){
        PWATCH_ITEM_ROW record = watchItemBuffer.get(i);
        e=_load_one_watch_item(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_wl_and_wi()
{	
    pGenerateAndLoad->InitWatchListAndWatchItem();
    TRACE( TRACE_ALWAYS, "Building WATCH_LIST table and WATCH_ITEM !!!\n");

    w_rc_t e = RCOK;
    while(watchListBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        watchItemBuffer.reset();
        watchListBuffer.reset();
        _read_wl_and_wi();
        populate_wl_and_wi_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_wl_and_wi(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseWatchListAndWatchItem();
    watchItemBuffer.release();
    watchListBuffer.release();
}




//CUSTOMER_TAXRATE
w_rc_t ShoreTPCEEnv::xct_populate_customer_taxrate(const int xct_id, populate_customer_taxrate_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pcustomer_taxrate_man->ts());
    areprow.set(_pcustomer_taxrate_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=customerTaxrateBuffer.getSize();
    for(int i=0; i<rows; i++){
        PCUSTOMER_TAXRATE_ROW record = customerTaxrateBuffer.get(i);
        e=_load_one_customer_taxrate(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_customer_taxrate()
{	
    pGenerateAndLoad->InitCustomerTaxrate();
    TRACE( TRACE_ALWAYS, "Building CUSTOMER_TAXRATE !!!\n");
    w_rc_t e = RCOK;
    while(customerTaxrateBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        customerTaxrateBuffer.reset();
        _read_customer_taxrate();
        populate_customer_taxrate_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_customer_taxrate(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseCustomerTaxrate();
    customerTaxrateBuffer.release();
}

//COMPANY
w_rc_t ShoreTPCEEnv::xct_populate_company(const int xct_id, populate_company_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pcompany_man->ts());
    areprow.set(_pcompany_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=companyBuffer.getSize();
    for(int i=0; i<rows; i++){
        PCOMPANY_ROW record = companyBuffer.get(i);
        e=_load_one_company(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_company()
{	
    pGenerateAndLoad->InitCompany();
    TRACE( TRACE_ALWAYS, "Building COMPANY  !!!\n");
    w_rc_t e = RCOK;
    while(companyBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        companyBuffer.reset();
        _read_company();
        populate_company_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_company(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseCompany();
    companyBuffer.release();
}



//COMPANY COMPETITOR
w_rc_t ShoreTPCEEnv::xct_populate_company_competitor(const int xct_id, populate_company_competitor_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pcompany_competitor_man->ts());
    areprow.set(_pcompany_competitor_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=companyCompetitorBuffer.getSize();
    for(int i=0; i<rows; i++){
        PCOMPANY_COMPETITOR_ROW record = companyCompetitorBuffer.get(i);
        e=_load_one_company_competitor(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_company_competitor()
{	
    pGenerateAndLoad->InitCompanyCompetitor();
    TRACE( TRACE_ALWAYS, "Building COMPANY COMPETITOR !!!\n");
    w_rc_t e = RCOK;
    while(companyCompetitorBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        companyCompetitorBuffer.reset();
        _read_company_competitor();
        populate_company_competitor_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_company_competitor(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseCompanyCompetitor();
    companyCompetitorBuffer.release();
}

//COMPANY
w_rc_t ShoreTPCEEnv::xct_populate_daily_market(const int xct_id,populate_daily_market_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pdaily_market_man->ts());
    areprow.set(_pdaily_market_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=dailyMarketBuffer.getSize();
    for(int i=0; i<rows; i++){
        PDAILY_MARKET_ROW record = dailyMarketBuffer.get(i);
        e=_load_one_daily_market(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_daily_market()
{	
    pGenerateAndLoad->InitDailyMarket();
    TRACE( TRACE_ALWAYS, "DAILY_MARKET   !!!\n");
    w_rc_t e = RCOK;
    while(dailyMarketBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        dailyMarketBuffer.reset();
        _read_daily_market();
        populate_daily_market_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_daily_market(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseDailyMarket();
    dailyMarketBuffer.release();
}

//FINANCIAL
w_rc_t ShoreTPCEEnv::xct_populate_financial(const int xct_id, populate_financial_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pfinancial_man->ts());
    areprow.set(_pfinancial_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=financialBuffer.getSize();
    for(int i=0; i<rows; i++){
        PFINANCIAL_ROW record = financialBuffer.get(i);
        e=_load_one_financial(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}


void ShoreTPCEEnv::populate_financial()
{	
    pGenerateAndLoad->InitFinancial();
    TRACE( TRACE_ALWAYS, "Building FINANCIAL  !!!\n");
    w_rc_t e = RCOK;
    while(financialBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        financialBuffer.reset();
        _read_financial();
        populate_financial_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_financial(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseFinancial();
    financialBuffer.release();
}


//SECURITY
w_rc_t ShoreTPCEEnv::xct_populate_security(const int xct_id, populate_security_input_t& ptoin)
{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_psecurity_man->ts());
    areprow.set(_psecurity_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=securityBuffer.getSize();
    for(int i=0; i<rows; i++){
        PSECURITY_ROW record = securityBuffer.get(i);
        e=_load_one_security(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}


void ShoreTPCEEnv::populate_security()
{	
    pGenerateAndLoad->InitSecurity();
    TRACE( TRACE_ALWAYS, "Building SECURITY  !!!\n");
    w_rc_t e = RCOK;
    while(securityBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        securityBuffer.reset();
        _read_security();
        populate_security_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_security(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseSecurity();
    securityBuffer.release();
}


//LAST_TRADE
w_rc_t ShoreTPCEEnv::xct_populate_last_trade(const int xct_id, populate_last_trade_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_plast_trade_man->ts());
    areprow.set(_plast_trade_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=lastTradeBuffer.getSize();
    for(int i=0; i<rows; i++){
        PLAST_TRADE_ROW record = lastTradeBuffer.get(i);
        e=_load_one_last_trade(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}


void ShoreTPCEEnv::populate_last_trade()
{	
    pGenerateAndLoad->InitLastTrade();
    TRACE( TRACE_ALWAYS, "Building LAST_TRADE  !!!\n");
    w_rc_t e = RCOK;
    while(lastTradeBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        lastTradeBuffer.reset();
        _read_last_trade();
        populate_last_trade_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_last_trade(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseLastTrade();
    lastTradeBuffer.release();
}

//Watch List and Watch Item
w_rc_t ShoreTPCEEnv::xct_populate_ni_and_nx(const int xct_id, populate_ni_and_nx_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pnews_item_man->ts());
    areprow.set(_pnews_item_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=newsXRefBuffer.getSize();
    for(int i=0; i<rows; i++){
        PNEWS_XREF_ROW record = newsXRefBuffer.get(i);
        e=_load_one_news_xref(areprow, record);
        if(e.is_error()) { return (e); }
    }

    rows=newsItemBuffer.getSize();
    for(int i=0; i<rows; i++){
        PNEWS_ITEM_ROW record = newsItemBuffer.get(i);
        e=_load_one_news_item(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_ni_and_nx()
{	
    pGenerateAndLoad->InitNewsItemAndNewsXRef();
    TRACE( TRACE_ALWAYS, "Building NEWS_ITEM and NEWS_XREF !!!\n");
    w_rc_t e = RCOK;
    while(newsItemBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        newsItemBuffer.reset();
        newsXRefBuffer.reset();
        _read_ni_and_nx();
        populate_ni_and_nx_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_ni_and_nx(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
    pGenerateAndLoad->ReleaseNewsItemAndNewsXRef();
    newsItemBuffer.release();
    newsXRefBuffer.release();
}


//populating growing tables
w_rc_t ShoreTPCEEnv::xct_populate_unit_trade(const int xct_id, populate_unit_trade_input_t& ptoin)
{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pnews_item_man->ts());
    areprow.set(_pnews_item_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=tradeBuffer.getSize();
    for(int i=0; i<rows; i++){
        PTRADE_ROW record = tradeBuffer.get(i);
        e=_load_one_trade(areprow, record);
        if(e.is_error()) { return (e); }
    }

    rows=tradeHistoryBuffer.getSize();
    for(int i=0; i<rows; i++){
        PTRADE_HISTORY_ROW record = tradeHistoryBuffer.get(i);
        e=_load_one_trade_history(areprow, record);
        if(e.is_error()) { return (e); }
    }

    rows=settlementBuffer.getSize();
    for(int i=0; i<rows; i++){
        PSETTLEMENT_ROW record = settlementBuffer.get(i);
        e=_load_one_settlement(areprow, record);
        if(e.is_error()) { return (e); }
    }

    rows=cashTransactionBuffer.getSize();
    for(int i=0; i<rows; i++){
        PCASH_TRANSACTION_ROW record = cashTransactionBuffer.get(i);
        e=_load_one_cash_transaction(areprow, record);
        if(e.is_error()) { return (e); }
    }

    rows=holdingHistoryBuffer.getSize();
    for(int i=0; i<rows; i++){
        PHOLDING_HISTORY_ROW record = holdingHistoryBuffer.get(i);
        e=_load_one_holding_history(areprow, record);
        if(e.is_error()) { return (e); }
    }


    e = _pssm->commit_xct();
    return (e);
}

//BROKER
w_rc_t ShoreTPCEEnv::xct_populate_broker(const int xct_id, populate_broker_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pbroker_man->ts());
    areprow.set(_pbroker_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=brokerBuffer.getSize();
    for(int i=0; i<rows; i++){
        PBROKER_ROW record = brokerBuffer.get(i);
        e=_load_one_broker(areprow, record);
        if(e.is_error()) { return (e); }
    }
    e = _pssm->commit_xct();
    return (e);
}


void ShoreTPCEEnv::populate_broker()
{	
    w_rc_t e = RCOK;
    while(brokerBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        brokerBuffer.reset();
        _read_broker();
        populate_broker_input_t in;
    retry:
        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_broker(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
}


//HOLDING_SUMMARY
w_rc_t ShoreTPCEEnv::xct_populate_holding_summary(const int xct_id, populate_holding_summary_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pholding_summary_man->ts());
    areprow.set(_pholding_summary_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=holdingSummaryBuffer.getSize();
    for(int i=0; i<rows; i++){
        PHOLDING_SUMMARY_ROW record = holdingSummaryBuffer.get(i);
        e=_load_one_holding_summary(areprow, record);
        if(e.is_error()) { return (e); }
    }

    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_holding_summary()
{	
    w_rc_t e = RCOK;
    while(holdingSummaryBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        holdingSummaryBuffer.reset();
        _read_holding_summary();
        populate_holding_summary_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_holding_summary(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
}


//HOLDING
w_rc_t ShoreTPCEEnv::xct_populate_holding(const int xct_id, populate_holding_input_t& ptoin)

{
    assert (_pssm);
    assert (_initialized);
    rep_row_t areprow(_pholding_man->ts());
    areprow.set(_pholding_desc->maxsize());

    w_rc_t e = RCOK;

    int rows=holdingBuffer.getSize();
    for(int i=0; i<rows; i++){
        PHOLDING_ROW record = holdingBuffer.get(i);
        e=_load_one_holding(areprow, record);
        if(e.is_error()) { return (e); }
    }

    e = _pssm->commit_xct();
    return (e);
}

void ShoreTPCEEnv::populate_holding()
{	
    w_rc_t e = RCOK;
    while(holdingBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        holdingBuffer.reset();
        _read_holding();
        populate_holding_input_t in;
    retry:

        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_holding(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
}



void ShoreTPCEEnv::populate_unit_trade()
{
    w_rc_t e = RCOK;
    while(tradeBuffer.hasMoreToRead()){
        long log_space_needed = 0;
        tradeBuffer.reset();
        tradeHistoryBuffer.reset();
        settlementBuffer.reset();
        cashTransactionBuffer.reset();
        holdingHistoryBuffer.reset();
        _read_trade_unit();
        populate_unit_trade_input_t in;
    retry:
        W_COERCE(this->db()->begin_xct());
        e = this->xct_populate_unit_trade(1, in);
        CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    }
}

void ShoreTPCEEnv::populate_growing()
{	
    pGenerateAndLoad->InitHoldingAndTrade();
    TRACE( TRACE_ALWAYS, "Building growing tables  !!!\n");
    w_rc_t e = RCOK;	
    int cnt =0;
    do{
        populate_unit_trade();
        populate_broker();
        populate_holding_summary();
        populate_holding();
	printf("\nload unit %d\n",++cnt);
    } while(pGenerateAndLoad->hasNextLoadUnit());

    pGenerateAndLoad->ReleaseHoldingAndTrade();
    tradeBuffer.release();
    tradeHistoryBuffer.release();
    settlementBuffer.release();
    cashTransactionBuffer.release();
    holdingHistoryBuffer.release();
    brokerBuffer.release();
    holdingSummaryBuffer.release();
    holdingBuffer.release();
}



w_rc_t ShoreTPCEEnv::xct_find_maxtrade_id(const int xct_id, find_maxtrade_id_input_t& ptoin)
{
    w_rc_t e = RCOK;
    assert (_pssm);

    table_row_t* prtrade = _ptrade_man->get_tuple();
    assert (prtrade);
	
    rep_row_t lowrep(_ptrade_man->ts());
    rep_row_t highrep(_ptrade_man->ts());

    lowrep.set(_ptrade_desc->maxsize());
    highrep.set(_ptrade_desc->maxsize());	
	
    rep_row_t areprow(_ptrade_man->ts());
    areprow.set(_ptrade_desc->maxsize());
	
    prtrade->_rep = &areprow;
	
    TIdent trade_id;
	
    if(lastTradeId == -1){
        guard<index_scan_iter_impl<trade_t> > t_iter;
        {
            index_scan_iter_impl<trade_t>* tmp_t_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d TO:t-iter-by-caid-idx \n", xct_id);
            e = _ptrade_man->t_get_iter_by_index(_pssm, tmp_t_iter, prtrade, lowrep, highrep, 0);
            if (e.is_error()) { goto done; }
            t_iter = tmp_t_iter;	  
        }
        bool eof;
        TRACE( TRACE_TRX_FLOW, "App: %d TO:t-iter-next \n", xct_id);
        e = t_iter->next(_pssm, eof, *prtrade);
        if (e.is_error()) { goto done; }
        while(!eof){
	    
	    prtrade->get_value(0, trade_id);
	    
	    //TRACE( TRACE_TRX_FLOW, "App: %d TO:t-iter-next \n", xct_id);
	    e = t_iter->next(_pssm, eof, *prtrade);
	    if (e.is_error()) { goto done; }
        }
        lastTradeId = ++trade_id;		  
    }			
    e = _pssm->commit_xct();
 done:
    _ptrade_man->give_tuple(prtrade);
    return (e);
}
void ShoreTPCEEnv::find_maxtrade_id()
{
    w_rc_t e = RCOK;
    find_maxtrade_id_input_t in;
    long log_space_needed = 0;
 retry:
    W_COERCE(this->db()->begin_xct());
    e = this->xct_find_maxtrade_id(1, in);
    CHECK_XCT_RETURN(e,log_space_needed,retry,this);
    printf("last trade id: %lld\n",  lastTradeId);
}




/******************************************************************** 
 *
 * TPC-E TRXs Wrappers
 *
 * @brief: They are wrappers to the functions that execute the transaction
 *         body. Their responsibility is to:
 *
 *         1. Prepare the corresponding input
 *         2. Check the return of the trx function and abort the trx,
 *            if something went wrong
 *         3. Update the tpce db environment statistics
 *
 ********************************************************************/

DEFINE_TRX(ShoreTPCEEnv,broker_volume);
DEFINE_TRX(ShoreTPCEEnv,customer_position);
DEFINE_TRX(ShoreTPCEEnv,market_feed);
DEFINE_TRX(ShoreTPCEEnv,market_watch);
DEFINE_TRX(ShoreTPCEEnv,security_detail);
DEFINE_TRX(ShoreTPCEEnv,trade_lookup);
DEFINE_TRX(ShoreTPCEEnv,trade_order);
DEFINE_TRX(ShoreTPCEEnv,trade_result);
DEFINE_TRX(ShoreTPCEEnv,trade_status);
DEFINE_TRX(ShoreTPCEEnv,trade_update);
DEFINE_TRX(ShoreTPCEEnv,data_maintenance);
DEFINE_TRX(ShoreTPCEEnv,trade_cleanup);

/******************************************************************** 
 *
 * TPC-E TRANSACTION IMPLEMENTATION
 *
 ********************************************************************/




/******************************************************************** 
 *
 * TPC-E TRADE_ORDER
 *
 ********************************************************************/

w_rc_t ShoreTPCEEnv::xct_trade_order(const int xct_id, trade_order_input_t& ptoin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prcustacct = _pcustomer_account_man->get_tuple();
    assert (prcustacct);

    table_row_t* prcust = _pcustomer_man->get_tuple();
    assert (prcust);

    table_row_t* prbrok = _pbroker_man->get_tuple();
    assert (prbrok);

    table_row_t* pracctperm = _paccount_permission_man->get_tuple();
    assert (pracctperm);

    table_row_t* prcompany = _pcompany_man->get_tuple();
    assert (prcompany);

    table_row_t* prsecurity = _psecurity_man->get_tuple();
    assert (prsecurity);

    table_row_t* prlasttrade = _plast_trade_man->get_tuple();
    assert (prlasttrade);

    table_row_t* prholdingsummary = _pholding_summary_man->get_tuple();
    assert (prholdingsummary);

    table_row_t* prholding = _pholding_man->get_tuple();
    assert (prholding);

    table_row_t* prcharge = _pcharge_man->get_tuple();
    assert (prcharge);

    table_row_t* prtrade = _ptrade_man->get_tuple();
    assert (prtrade);

    table_row_t* prtradetype = _ptrade_type_man->get_tuple();
    assert (prtradetype);

    table_row_t* prtradehist = _ptrade_history_man->get_tuple();
    assert (prtradehist);

    table_row_t* prtradereq = _ptrade_request_man->get_tuple();
    assert (prtradereq);

    table_row_t* prcusttaxrate = _pcustomer_taxrate_man->get_tuple();
    assert (prcusttaxrate);

    table_row_t* prtaxrate = _ptaxrate_man->get_tuple();
    assert (prtaxrate);

    table_row_t* prcommrate = _pcommission_rate_man->get_tuple();
    assert (prcommrate);

    w_rc_t e = RCOK;
    rep_row_t areprow(_pcompany_man->ts());
	
    areprow.set(_pcompany_desc->maxsize());

    prcustacct->_rep = &areprow;
    prcust->_rep = &areprow;
    prbrok->_rep = &areprow;
    pracctperm->_rep = &areprow;
    prtrade->_rep = &areprow;
    prtradereq->_rep = &areprow;
    prtradehist->_rep = &areprow;
    prcompany->_rep = &areprow;
    prsecurity->_rep = &areprow;
    prlasttrade->_rep = &areprow;
    prtradetype->_rep = &areprow;
    prholdingsummary->_rep = &areprow;
    prholding->_rep = &areprow;
    prcharge->_rep = &areprow;
    prcusttaxrate->_rep = &areprow;
    prtaxrate->_rep = &areprow;
    prcommrate->_rep = &areprow;

    rep_row_t lowrep(_pcompany_man->ts());
    rep_row_t highrep(_pcompany_man->ts());

    lowrep.set(_pcompany_desc->maxsize());
    highrep.set(_pcompany_desc->maxsize());

    //TradeRequest for Marker
    PTradeRequest req = NULL;
    /*
      {//try
      {
      int cnt = 0;
      holding_summary_man_impl::table_iter* hs_iter;
      //	TRACE( TRACE_TRX_FLOW, "App: %d MW:lt-get-table-iter \n", xct_id);
      e = _pholding_summary_man->hs_get_table_iter(_pssm, hs_iter);
      if (e.is_error()) { goto done; }

      bool eof;
      //TRACE( TRACE_TRX_FLOW, "App: %d MW:lt-iter-next \n", xct_id);
      e = hs_iter->next(_pssm, eof, *prholdingsummary);
      if (e.is_error()) { goto done; }
      while(!eof){
      TIdent id;
      prholdingsummary->get_value(0, id);	
		  
      //TRACE( TRACE_TRX_FLOW, "App: %d TO:ca-idx-probe (%ld) \n", xct_id,  ptoin._acct_id);
      e =  _pcustomer_account_man->ca_index_probe(_pssm, prcustacct, id);
      if (e.is_error()) {printf("NOT FOUND \n"); }
		  
      e = hs_iter->next(_pssm, eof, *prholdingsummary);
      if (e.is_error()) { goto done; }		  
      cnt++;
      }
      printf("HS TABLE COUNT: 	%d \n\n\n\n\n\n\n\n\n\n\n", cnt);
      }	  
	  
      }*/
    { // make gotos safe
        //ptoin.print();
        double requested_price = ptoin._requested_price;
		
        //BEGIN FRAME1
        char cust_f_name[21];  //20
        char cust_l_name[26]; //21
        char tax_id[21]; //20
        short tax_status;
        TIdent cust_id;
        short cust_tier;
        TIdent broker_id;
        {
            /**
             * SELECT 	acct_name = CA_NAME, broker_id = CA_B_ID, cust_id = CA_C_ID, tax_status = CA_TAX_ST
             * FROM 	CUSTOMER_ACCOUNT
             * WHERE	CA_ID = acct_id
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TO:ca-idx-probe (%ld) \n", xct_id,  ptoin._acct_id);
            e =  _pcustomer_account_man->ca_index_probe(_pssm, prcustacct, ptoin._acct_id);
            if (e.is_error()) { goto done; }

            char acct_name[51] = "\0"; //50
            prcustacct->get_value(3, acct_name, 51);
            prcustacct->get_value(1, broker_id);
            prcustacct->get_value(2, cust_id);
            prcustacct->get_value(4, tax_status);

            assert(acct_name[0] != 0); //Harness control

            /**
             * 	SELECT	cust_f_name = C_F_NAME,	cust_l_name = C_L_NAME,	cust_tier = C_TIER, tax_id = C_TAX_ID
             *	FROM	CUSTOMER
             *	WHERE 	C_ID = cust_id
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TO:c-idx-probe (%ld) \n", xct_id,  cust_id);
            e =  _pcustomer_man->c_index_probe(_pssm, prcust, cust_id);
            if (e.is_error()) { goto done; }

            prcust->get_value(4, cust_f_name, 21);
            prcust->get_value(3, cust_l_name, 26);
            prcust->get_value(7, cust_tier);
            prcust->get_value(1, tax_id, 21);

            /**
             * 	SELECT	broker_name = B_NAME
             *	FROM	BROKER
             *	WHERE	B_ID = broker_id
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TO:b-idx-probe (%ld) \n", xct_id, broker_id);
            e =  _pbroker_man->b_index_probe(_pssm, prbrok, broker_id);
            if (e.is_error()) { goto done; }

            char broker_name[50];
            prbrok->get_value(2, broker_name, 50);
        }
        //END FRAME1


        //BEGIN FRAME2
        {
            /**
             * 	select
             *		ap_acl = AP_ACL
             *	from
             *		ACCOUNT_PERMISSION
             *	where
             *		AP_CA_ID = acct_id and
             *		AP_F_NAME = exec_f_name and
             *		AP_L_NAME = exec_l_name and
             *		AP_TAX_ID = exec_tax_id
             */

            char ap_acl[5] = ""; //4
            if(strcmp(ptoin._exec_l_name, cust_l_name) != 0 || strcmp(ptoin._exec_f_name, cust_f_name) != 0 || strcmp(ptoin._exec_tax_id, tax_id) != 0 ){
                TRACE( TRACE_TRX_FLOW, "App: %d TO:ap-idx-probe (%ld) (%s) \n", xct_id,  ptoin._acct_id, ptoin._exec_tax_id);
                e =  _paccount_permission_man->ap_index_probe(_pssm, pracctperm, ptoin._acct_id, ptoin._exec_tax_id);
                if (e.is_error()) { goto done; } //Validate authorization, can fails sometimes, NORMAL to abort, rollback

                char f_name[21], l_name[26];
                pracctperm->get_value(3, l_name, 26);
                pracctperm->get_value(4, f_name, 21);

                if(strcmp(ptoin._exec_l_name, l_name) == 0 && strcmp(ptoin._exec_f_name, f_name) == 0){
                    pracctperm->get_value(1, ap_acl, 5);
                }
                //assert(strcmp(ap_acl, "") != 0); //Validate authorization, can fails sometimes, NORMAL to abort, rollback
                if(strcmp(ap_acl, "") == 0){
                    e = RC(se_NOT_FOUND);
                    if (e.is_error()) { goto done; }
                }
            }
        }
        //END FRAME2

        //BEGIN FRAME3
        double comm_rate;
        char status_id[5]; //4
        double charge_amount;
        bool type_is_market;

        double buy_value;
        double sell_value;
        double tax_amount;
		
        char 	symbol[16]; //15
        strcpy(symbol, ptoin._symbol);
        {
            TIdent 	co_id;
            char	exch_id[7]; //6

            double 	cust_assets;
            double 	market_price;
            char	s_name[71]; //70

            bool 	type_is_sell;
            if(symbol[0] == 0){
                /**
                 * 	select
                 *		co_id = CO_ID
                 *	from
                 *		COMPANY
                 *	where
                 *		CO_NAME = co_name
                 */

                guard< index_scan_iter_impl<company_t> > co_iter;
                {
                    index_scan_iter_impl<company_t>* tmp_co_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TO:co-get-iter-by-idx2 (%s) \n", xct_id, ptoin._co_name);
                    e = _pcompany_man->co_get_iter_by_index2(_pssm, tmp_co_iter, prcompany, lowrep, highrep, ptoin._co_name);
                    co_iter = tmp_co_iter;
                    if (e.is_error()) { goto done; }
                }

                bool eof;
                TRACE( TRACE_TRX_FLOW, "App: %d TO:co-iter-next \n", xct_id);
                e = co_iter->next(_pssm, eof, *prcompany);
                if (e.is_error()) { goto done; }

                prcompany->get_value(0, co_id);

                /**
                 * 	select
                 *		exch_id = S_EX_ID,
                 *		s_name = S_NAME,
                 *		symbol = S_SYMB
                 *	from
                 *		SECURITY
                 *	where
                 *		S_CO_ID = co_id and
                 *		S_ISSUE = issue
                 *
                 */

                guard< index_scan_iter_impl<security_t> > s_iter;
                {
                    index_scan_iter_impl<security_t>* tmp_s_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TO:s-get-iter-by-idx2 (%s) (%ld) \n", xct_id, ptoin._issue, co_id);
                    e = _psecurity_man->s_get_iter_by_index2(_pssm, tmp_s_iter, prsecurity, lowrep, highrep, true, ptoin._issue, co_id);
                    s_iter = tmp_s_iter;
                    if (e.is_error()) { goto done; }
                }

                TRACE( TRACE_TRX_FLOW, "App: %d TO:s-iter-next \n", xct_id);
                e = s_iter->next(_pssm, eof, *prsecurity);
                if (e.is_error()) { goto done; }
                while(!eof){				  
                    char s_issue[7];
                    prsecurity->get_value(1, s_issue, 7);				  
				  	  
                    if(strcmp(s_issue, ptoin._issue) == 0){
                        prsecurity->get_value(4, exch_id, 7);
                        prsecurity->get_value(3, s_name, 71);				
                        prsecurity->get_value(0, symbol, 16);
                        break;
                    }
				  
                    TRACE( TRACE_TRX_FLOW, "App: %d TO:s-iter-next \n", xct_id);
                    e = s_iter->next(_pssm, eof, *prsecurity);
                    if (e.is_error()) { goto done; }
                }
            }
            else{
                /**
                 * 	select
                 *		co_id = S_CO_ID,
                 *		exch_id = S_EX_ID,
                 *		s_name = S_NAME
                 *	from
                 *		SECURITY
                 *	where
                 *		S_SYMB = symbol
                 */

                TRACE( TRACE_TRX_FLOW, "App: %d TO:s-idx-probe (%s) \n", xct_id, symbol);
                e =  _psecurity_man->s_index_probe(_pssm, prsecurity, symbol);
                if(e.is_error()) { goto done; }

                prsecurity->get_value(5, co_id);
                prsecurity->get_value(4, exch_id, 7);
                prsecurity->get_value(3, s_name, 71);

                /**
                 * 	select
                 *		co_name = CO_NAME
                 *	from
                 *		COMPANY
                 *	where
                 *		CO_ID = co_id
                 */

                TRACE( TRACE_TRX_FLOW, "App: %d TO:co-idx-probe (%ld) \n", xct_id, co_id);
                e =  _pcompany_man->co_index_probe(_pssm, prcompany, co_id);
                if(e.is_error()) { goto done; }

                char co_name[61]; //60
                prcompany->get_value(2, co_name, 61);
            }

            /**
             * 	select
             *		market_price = LT_PRICE
             *	from
             *		LAST_TRADE
             *	where
             *		LT_S_SYMB = symbol
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TO:lt-idx-probe (%s) \n", xct_id, symbol);
            e =  _plast_trade_man->lt_index_probe(_pssm, prlasttrade, symbol);
            if(e.is_error()) { goto done; }

            prlasttrade->get_value(2, market_price);

            /**
             * 	select
             *		type_is_market = TT_IS_MRKT,
             *		type_is_sell = TT_IS_SELL
             *	from
             *		TRADE_TYPE
             *	where
             *		TT_ID = trade_type_id
             *
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TO:tt-idx-probe (%s) \n", xct_id,  ptoin._trade_type_id);
            e =  _ptrade_type_man->tt_index_probe(_pssm, prtradetype, ptoin._trade_type_id);
            if (e.is_error()) { goto done; }

            prtradetype->get_value(3, type_is_market);
            prtradetype->get_value(2, type_is_sell);

            if(type_is_market){
                requested_price = market_price;
            }

            double hold_price;
            int hold_qty;
            double needed_qty;
            int hs_qty = -1;

            buy_value = 0;
            sell_value = 0;
            needed_qty = ptoin._trade_qty;

            /**
             * 	select
             *		hs_qty = HS_QTY
             *	from
             *		HOLDING_SUMMARY
             *	where
             *		HS_CA_ID = acct_id and
             *		HS_S_SYMB = symbol
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TO:hs-idx-probe (%ld) (%s) \n", xct_id,  ptoin._acct_id, symbol);
            e =  _pholding_summary_man->hs_index_probe(_pssm, prholdingsummary, ptoin._acct_id, symbol);
            if (e.is_error()) {
                hs_qty = 0;			
            }
            else{
                prholdingsummary->get_value(2, hs_qty);
            }

            if(type_is_sell){
                if(hs_qty > 0){
                    sort_buffer_t h_list(3); //change
                    guard<index_scan_iter_impl<holding_t> > h_iter;
                    if(ptoin._is_lifo){
                        /**
                         * SELECT	H_QTY, H_PRICE
                         * FROM		HOLDING
                         * WHERE	H_CA_ID = acct_id and H_S_SYMB = symbol
                         * ORDER BY 	H_DTS DESC
                         */
                        {  
                            index_scan_iter_impl<holding_t>* tmp_h_iter;
                            TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-by-idx2 \n", xct_id);
                            e = _pholding_man->h_get_iter_by_index2(_pssm, tmp_h_iter, prholding, lowrep, highrep, ptoin._acct_id, symbol);
                            if (e.is_error()) { goto done; }
                            h_iter = tmp_h_iter;
                        }
                        //descending order
                        h_list.setup(0, SQL_LONG);
                        h_list.setup(1, SQL_INT);
                        h_list.setup(2, SQL_FLOAT);
                        table_row_t rsb(&h_list);
						
                        rep_row_t sortrep(_pcompany_man->ts());
                        sortrep.set(_pcompany_desc->maxsize());
                        sort_man_impl h_sorter(&h_list, &sortrep);

                        bool eof;
                        TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-next \n", xct_id);
                        e = h_iter->next(_pssm, eof, *prholding);
                        if (e.is_error()) { goto done; }
                        while (!eof) {
                            /* put the value into the sorted buffer */
                            int temp_qty;
                            myTime temp_dts;
                            double temp_price;
							
                            char hs_s_symb[16];
                            prholding->get_value(2, hs_s_symb, 16);
							
                            if(strcmp(hs_s_symb, symbol) == 0){
                                prholding->get_value(5, temp_qty);
                                prholding->get_value(4, temp_price);
                                prholding->get_value(3, temp_dts);
							    
                                rsb.set_value(0, temp_dts);
                                rsb.set_value(1, temp_qty);
                                rsb.set_value(2, temp_price);

                                h_sorter.add_tuple(rsb);
                            }
                            TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-next \n", xct_id);
                            e = h_iter->next(_pssm, eof, *prholding);
                            if (e.is_error()) { goto done; }
                        }
                        assert (h_sorter.count());

                        sort_iter_impl h_list_sort_iter(_pssm, &h_list, &h_sorter);
                        TRACE( TRACE_TRX_FLOW, "App: %d TO:h-sort-iter-next \n", xct_id);
                        e = h_list_sort_iter.next(_pssm, eof, rsb);
                        if (e.is_error()) { goto done; }
                        while(needed_qty != 0 && !eof){
                            int hold_qty;
                            double hold_price;

                            rsb.get_value(1, hold_qty);
                            rsb.get_value(2, hold_price);

                            if(hold_qty > needed_qty){
                                buy_value += needed_qty * hold_price;
                                sell_value += needed_qty * requested_price;
                                needed_qty = 0;
                            }
                            else{
                                buy_value += hold_qty * hold_price;
                                sell_value += hold_qty * requested_price;
                                needed_qty = needed_qty - hold_qty;
                            }

                            TRACE( TRACE_TRX_FLOW, "App: %d TO:h-sort-iter-next \n", xct_id);
                            e = h_list_sort_iter.next(_pssm, eof, rsb);
                            if (e.is_error()) { goto done; }
                        }
                    }
                    else{
                        /**
                         * 	SELECT	H_QTY, H_PRICE
                         *   	FROM	HOLDING
                         *	WHERE	H_CA_ID = acct_id and H_S_SYMB = symbol
                         *	ORDER BY H_DTS ASC
                         */
						
                        {
                            index_scan_iter_impl<holding_t>* tmp_h_iter;
                            TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-by-idx2 \n", xct_id);
                            e = _pholding_man->h_get_iter_by_index2(_pssm, tmp_h_iter, prholding, lowrep, highrep, ptoin._acct_id, symbol);
                            if (e.is_error()) { goto done; }
                            h_iter = tmp_h_iter;
                        }
                        //already sorted in ascending order because of its index

                        bool eof;
                        TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-next \n", xct_id);
                        e = h_iter->next(_pssm, eof, *prholding);
                        if (e.is_error()) { goto done; }
                        while (needed_qty != 0 && !eof) {
                            char hs_s_symb[16];
                            prholding->get_value(2, hs_s_symb, 16);
							
                            if(strcmp(hs_s_symb, symbol) == 0){
                                int hold_qty;
                                double hold_price;

                                prholding->get_value(5, hold_qty);
                                prholding->get_value(4, hold_price);

                                if(hold_qty > needed_qty){
                                    buy_value += needed_qty * hold_price;
                                    sell_value += needed_qty * requested_price;
                                    needed_qty = 0;
                                }
                                else{
                                    buy_value += hold_qty * hold_price;
                                    sell_value += hold_qty * requested_price;
                                    needed_qty = needed_qty - hold_qty;
                                }
                            }
                            TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-next \n", xct_id);
                            e = h_iter->next(_pssm, eof, *prholding);
                            if (e.is_error()) { goto done; }
                        }
                    }
                }
            }
            else{
                sort_buffer_t h_list(3);
                guard<index_scan_iter_impl<holding_t> > h_iter;
                sort_scan_t* h_list_sort_iter;
                if(hs_qty < 0){
                    if(ptoin._is_lifo){
                        /**
                         * 	SELECT	H_QTY, H_PRICE
                         *   	FROM	HOLDING
                         *	WHERE	H_CA_ID = acct_id and H_S_SYMB = symbol
                         *	ORDER BY H_DTS DESC
                         */
                        {
                            index_scan_iter_impl<holding_t>* tmp_h_iter;
                            TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-by-idx2 (%ld) (%s) \n", xct_id, ptoin._acct_id, symbol);
                            e = _pholding_man->h_get_iter_by_index2(_pssm, tmp_h_iter, prholding, lowrep, highrep, ptoin._acct_id, symbol);
                            if (e.is_error()) { goto done; }
                            h_iter = tmp_h_iter;
                        }
  
                        //descending order
                        rep_row_t sortrep(_pcompany_man->ts());
                        sortrep.set(_pcompany_desc->maxsize());

                        h_list.setup(0, SQL_LONG);
                        h_list.setup(1, SQL_INT);
                        h_list.setup(2, SQL_FLOAT);

                        table_row_t rsb(&h_list);
                        sort_man_impl h_sorter(&h_list, &sortrep);

                        bool eof;
                        TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-next \n", xct_id);
                        e = h_iter->next(_pssm, eof, *prholding);
                        if (e.is_error()) { goto done; }
                        while (!eof) {
                            /* put the value into the sorted buffer */
                            char hs_s_symb[16];
                            prholding->get_value(2, hs_s_symb, 16);
							
                            if(strcmp(hs_s_symb, symbol) == 0){
                                int temp_qty;
                                myTime temp_dts;
                                double temp_price;

                                prholding->get_value(5, temp_qty);
                                prholding->get_value(4, temp_price);
                                prholding->get_value(3, temp_dts);

                                rsb.set_value(0, temp_dts);
                                rsb.set_value(1, temp_qty);
                                rsb.set_value(2, temp_price);

                                h_sorter.add_tuple(rsb);
                            }

                            TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-next \n", xct_id);
                            e = h_iter->next(_pssm, eof, *prholding);
                            if (e.is_error()) { goto done; }
                        }
                        assert (h_sorter.count());

                        sort_iter_impl h_list_sort_iter(_pssm, &h_list, &h_sorter);

                        TRACE( TRACE_TRX_FLOW, "App: %d TO:h-sort-iter-next \n", xct_id);
                        e = h_list_sort_iter.next(_pssm, eof, rsb);
                        if (e.is_error()) { goto done; }
                        while(needed_qty != 0 && !eof){
                            int hold_qty;
                            double hold_price;

                            rsb.get_value(1, hold_qty);
                            rsb.get_value(2, hold_price);

                            if(hold_qty + needed_qty < 0){
                                sell_value += needed_qty * hold_price;
                                buy_value += needed_qty * requested_price;
                                needed_qty = 0;
                            }
                            else{
                                hold_qty = -hold_qty;
                                sell_value += hold_qty * hold_price;
                                buy_value += hold_qty * requested_price;
                                needed_qty = needed_qty - hold_qty;
                            }

                            TRACE( TRACE_TRX_FLOW, "App: %d TO:h-sort-iter-next \n", xct_id);
                            e = h_list_sort_iter.next(_pssm, eof, rsb);
                            if (e.is_error()) { goto done; }
                        }
                    }
                    else{
                        /**
                         * 	SELECT	H_QTY, H_PRICE
                         *  	FROM	HOLDING
                         *	WHERE	H_CA_ID = acct_id and H_S_SYMB = symbol
                         *	ORDER BY H_DTS ASC
                         */
                        {
                            index_scan_iter_impl<holding_t>* tmp_h_iter;
                            TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-by-idx2 \n", xct_id);
                            e = _pholding_man->h_get_iter_by_index2(_pssm, tmp_h_iter, prholding, lowrep, highrep, ptoin._acct_id, symbol);
                            if (e.is_error()) { goto done; }
                            h_iter = tmp_h_iter;
                        }
                        //already sorted in ascending order because of its index
                        bool eof;
                        TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-next \n", xct_id);
                        e = h_iter->next(_pssm, eof, *prholding);
                        if (e.is_error()) { goto done; }
                        while (needed_qty != 0 && !eof) {
                            /* put the value into the sorted buffer */
                            char hs_s_symb[16];
                            prholding->get_value(2, hs_s_symb, 16);
							
                            if(strcmp(hs_s_symb, symbol) == 0){
                                int hold_qty;
                                double hold_price;

                                prholding->get_value(5, hold_qty);
                                prholding->get_value(4, hold_price);

                                if(hold_qty + needed_qty < 0){
                                    sell_value += needed_qty * hold_price;
                                    buy_value += needed_qty * requested_price;
                                    needed_qty = 0;
                                }
                                else{
                                    hold_qty = -hold_qty;
                                    sell_value += hold_qty * hold_price;
                                    buy_value += hold_qty * requested_price;
                                    needed_qty = needed_qty - hold_qty;
                                }
                            }
                            TRACE( TRACE_TRX_FLOW, "App: %d TO:h-iter-next \n", xct_id);
                            e = h_iter->next(_pssm, eof, *prholding);
                            if (e.is_error()) { goto done; }
                        }
                    }
                }
            }
            if((sell_value > buy_value) && ((tax_status == 1 || tax_status == 2))){
                double tax_rates;
                /**
                 *	select
                 *		tax_rates = sum(TX_RATE)
                 *	from
                 *		TAXRATE
                 *	where
                 *		TX_ID in (
                 *				select
                 *					CX_TX_ID
                 *				from
                 *					CUSTOMER_TAXRATE
                 *				where
                 *					CX_C_ID = cust_id
                 *			)
                 */
                guard< index_scan_iter_impl<customer_taxrate_t> > cx_iter;
                {
                    index_scan_iter_impl<customer_taxrate_t>* tmp_cx_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TO:cx-get-iter-by-idx (%ld) \n", xct_id, cust_id);
                    e = _pcustomer_taxrate_man->cx_get_iter_by_index(_pssm, tmp_cx_iter, prcusttaxrate, lowrep, highrep, cust_id);
                    if (e.is_error()) { goto done; }
                    cx_iter = tmp_cx_iter;
                }

                bool eof;
                TRACE( TRACE_TRX_FLOW, "App: %d TO:cx-iter-next \n", xct_id);
                e = cx_iter->next(_pssm, eof, *prcusttaxrate);
                if (e.is_error()) { goto done; }
                while (!eof) {
                    char tax_id[5]; //4
                    prcusttaxrate->get_value(0, tax_id, 5);

                    TRACE( TRACE_TRX_FLOW, "App: %d TO:tx-idx-probe (%s) \n", xct_id, tax_id);
                    e =  _ptaxrate_man->tx_index_probe(_pssm, prtaxrate, tax_id);
                    if (e.is_error()) { goto done; }

                    double rate;
                    prtaxrate->get_value(2, rate);
                    tax_rates += rate;

                    TRACE( TRACE_TRX_FLOW, "App: %d TO:cx-iter-next \n", xct_id);
                    e = cx_iter->next(_pssm, eof, *prcusttaxrate);
                    if (e.is_error()) { goto done; }
                }
                tax_amount = (sell_value - buy_value) * tax_rates;
            }
            /**
             *
             *	select
             *		comm_rate = CR_RATE
             *	from
             *		COMMISSION_RATE
             *	where
             *		CR_C_TIER = cust_tier and
             *		CR_TT_ID = trade_type_id and
             *		CR_EX_ID = exch_id and
             *		CR_FROM_QTY <= trade_qty and
             *		CR_TO_QTY >= trade_qty
             */

            guard< index_scan_iter_impl<commission_rate_t> > cr_iter;
            {
                index_scan_iter_impl<commission_rate_t>* tmp_cr_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d TR:cr-iter-by-idx (%d) (%s) (%s) (%d) \n", xct_id, cust_tier, ptoin._trade_type_id, exch_id, ptoin._trade_qty);
                e = _pcommission_rate_man->cr_get_iter_by_index(_pssm, tmp_cr_iter, prcommrate, lowrep, highrep, cust_tier, ptoin._trade_type_id, exch_id, ptoin._trade_qty);
                if (e.is_error()) { goto done; }
                cr_iter = tmp_cr_iter;
            }
            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d TO:cr-iter-next \n", xct_id);
            e = cr_iter->next(_pssm, eof, *prcommrate);
            if (e.is_error()) { goto done; }
            while (!eof) {
                short cr_c_tier;
                prcommrate->get_value(0, cr_c_tier);
				
                char cr_tt_id[4], cr_ex_id[7];
                prcommrate->get_value(1, cr_tt_id, 4);
                prcommrate->get_value(2, cr_ex_id, 7);	
				
                int to_qty;
                prcommrate->get_value(4, to_qty);

                if(cr_c_tier == cust_tier && strcmp(cr_tt_id, ptoin._trade_type_id) == 0 && strcmp(cr_ex_id, exch_id) == 0 && to_qty >= ptoin._trade_qty){
                    prcommrate->get_value(5, comm_rate);
                    break;
                }

                TRACE( TRACE_TRX_FLOW, "App: %d TO:cr-iter-next \n", xct_id);
                e = cr_iter->next(_pssm, eof, *prcommrate);
                if (e.is_error()) { goto done; }
            }
            /**
             *
             *	select
             *		charge_amount = CH_CHRG
             *	from
             *		CHARGE
             *	where
             *		CH_C_TIER = cust_tier and
             *		CH_TT_ID = trade_type_id
             */

            guard< index_scan_iter_impl<charge_t> > ch_iter;
            {
                index_scan_iter_impl<charge_t>* tmp_ch_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d TO:ch-get-iter-by-idx2 (%s) \n", xct_id, ptoin._trade_type_id);
                e =  _pcharge_man->ch_get_iter_by_index2(_pssm, tmp_ch_iter, prcharge, lowrep, highrep,  ptoin._trade_type_id);
                if (e.is_error()) { goto done; }
                ch_iter = tmp_ch_iter;
            }
			
            TRACE( TRACE_TRX_FLOW, "App: %d TO:ch-iter-next \n", xct_id);
            e = ch_iter->next(_pssm, eof, *prcharge);
            if (e.is_error()) { goto done; }
            while (!eof) {
                short ch_c_tier;
                prcharge->get_value(1, ch_c_tier);
			  
                if(ch_c_tier == cust_tier){
                    prcharge->get_value(2, charge_amount);
                    break;
                }		  
			  
                TRACE( TRACE_TRX_FLOW, "App: %d TO:ch-iter-next \n", xct_id);
                e = ch_iter->next(_pssm, eof, *prcharge);
                if (e.is_error()) { goto done; }
            }

            double acct_bal;
            double hold_assets = 0;
            cust_assets = 0;

            if(ptoin._type_is_margin){
                /**
                 *	select
                 *		acct_bal = CA_BAL
                 *	from
                 *		CUSTOMER_ACCOUNT
                 *	where
                 *		CA_ID = acct_id
                 */
                TRACE( TRACE_TRX_FLOW, "App: %d TO:ca-idx-probe (%ld) \n", xct_id, ptoin._acct_id);
                e =  _pcustomer_account_man->ca_index_probe(_pssm, prcustacct, ptoin._acct_id);
                if (e.is_error()) { goto done; }

                prcustacct->get_value(5, acct_bal);

                /**
                 *
                 *	select
                 *		hold_assets = sum(HS_QTY * LT_PRICE)
                 *	from
                 *		HOLDING_SUMMARY,
                 *		LAST_TRADE
                 *	where
                 *		HS_CA_ID = acct_id and
                 *		LT_S_SYMB = HS_S_SYMB
                 */

                guard< index_scan_iter_impl<holding_summary_t> > hs_iter;
                {
                    index_scan_iter_impl<holding_summary_t>* tmp_hs_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TO:hs-iter-by-idx (%ld) \n", xct_id, ptoin._acct_id);
                    e = _pholding_summary_man->hs_get_iter_by_index(_pssm, tmp_hs_iter, prholdingsummary, lowrep, highrep, ptoin._acct_id);
                    if (e.is_error()) { goto done; }
                    hs_iter = tmp_hs_iter;
                }

                TRACE( TRACE_TRX_FLOW, "App: %d TO:hs-iter-next \n", xct_id);
                e = hs_iter->next(_pssm, eof, *prholdingsummary);
                if (e.is_error()) { goto done; }
                while (!eof) {
                    char symb[16]; //15
                    prholdingsummary->get_value(1, symb, 16);
                    int hs_qty;
                    prholdingsummary->get_value(2, hs_qty);

                    TRACE( TRACE_TRX_FLOW, "App: %d TO:lt-idx-probe (%s) \n", xct_id, symb);
                    e =  _plast_trade_man->lt_index_probe(_pssm, prlasttrade, symb);
                    if(e.is_error()) {goto done; }

                    double lt_price;
                    prlasttrade->get_value(3, lt_price);
                    hold_assets += (lt_price * hs_qty);

                    TRACE( TRACE_TRX_FLOW, "App: %d TO:hs-iter-next \n", xct_id);
                    e = hs_iter->next(_pssm, eof, *prholdingsummary);
                    if (e.is_error()) { goto done; }
                }

                if(hold_assets == 0){
                    cust_assets = acct_bal;
                }
                else{
                    cust_assets = hold_assets + acct_bal;
                }
            }
        }
        //END FRAME3

        if((sell_value > buy_value) && ((tax_status == 1) || (tax_status == 2)) && (tax_amount == 0)){
            assert(false); //Harness control
        }
        else if(comm_rate == 0.0000){
            assert(false); //Harness control
        }
        else if(charge_amount == 0){
            assert(false); //Harness control
        }

        double comm_amount = (comm_rate/100) * ptoin._trade_qty * requested_price;
        char exec_name[50]; //49
        strcpy(exec_name, ptoin._exec_f_name);
        strcat(exec_name, " ");
        strcat(exec_name, ptoin._exec_l_name);
        bool is_cash = !ptoin._type_is_margin;
        //BEGIN FRAME4
        {
            myTime now_dts = time(NULL);
            TIdent trade_id;
			
            {
                CRITICAL_SECTION(cs_to,_trade_order_lock);
                trade_id = lastTradeId++;			  
            }
		    
            /**
             *
             *	INSERT INTO TRADE 	(
             *					T_ID, T_DTS, T_ST_ID, T_TT_ID, T_IS_CASH,
             *					T_S_SYMB, T_QTY, T_BID_PRICE, T_CA_ID, T_EXEC_NAME,
             *					T_TRADE_PRICE, T_CHRG, T_COMM, T_TAX, T_LIFO
             *				)
             *	VALUES 			(	trade_id, // T_ID
             *					now_dts, // T_DTS
             *					status_id, // T_ST_ID
             *					trade_type_id, // T_TT_ID
             *					is_cash, // T_IS_CASH
             *					symbol, // T_S_SYMB
             *					trade_qty, // T_QTY
             *					requested_price, // T_BID_PRICE
             *					acct_id, // T_CA_ID
             *					exec_name, // T_EXEC_NAME
             *					NULL, // T_TRADE_PRICE
             *					charge_amount, // T_CHRG
             *					comm_amount // T_COMM
             *					0, // T_TAX
             *					is_lifo // T_LIFO
             *				)
             */
            prtrade->set_value(0, trade_id);
            prtrade->set_value(1, now_dts);
            prtrade->set_value(2, status_id);
            prtrade->set_value(3, ptoin._trade_type_id);
            prtrade->set_value(4, is_cash);
            prtrade->set_value(5, symbol);
            prtrade->set_value(6, ptoin._trade_qty);
            prtrade->set_value(7, requested_price);
            prtrade->set_value(8, ptoin._acct_id);
            prtrade->set_value(9, exec_name);
            prtrade->set_value(10, (double)-1);
            prtrade->set_value(11, charge_amount);
            prtrade->set_value(12, comm_amount);
            prtrade->set_value(13, (double)0);
            prtrade->set_value(14, ptoin._is_lifo);

            TRACE( TRACE_TRX_FLOW, "App: %d TO:t-add-tuple (%ld) \n", xct_id, trade_id);
            e = _ptrade_man->add_tuple(_pssm, prtrade);
            if (e.is_error()) { goto done; }

            //prepare TradeRequest to send to market
			
			
            req= new TTradeRequest();
            req->trade_id = trade_id;
            req->trade_qty = ptoin._trade_qty;
            strcpy(req->symbol, symbol);
            strcpy(req->trade_type_id, ptoin._trade_type_id);
            req->price_quote = requested_price;
            if(type_is_market)   req->eAction=eMEEProcessOrder;
            else req->eAction=eMEESetLimitOrderTrigger;
			
            if(!type_is_market){

                /**
                 *
                 *	INSERT INTO TRADE_REQUEST 	(
                 *						TR_T_ID, TR_TT_ID, TR_S_SYMB,
                 *						TR_QTY, TR_BID_PRICE, TR_B_ID
                 *					)
                 *	VALUES 				(
                 *						trade_id, // TR_T-ID
                 *						trade_type_id, // TR_TT_ID
                 *						symbol, // TR_S_SYMB
                 *						trade_qty, // TR_QTY
                 *						requested_price, // TR_BID_PRICE
                 *						broker_id // TR_B_ID
                 *					)
                 */
                prtradereq->set_value(0, trade_id);
                prtradereq->set_value(1, ptoin._trade_type_id);
                prtradereq->set_value(2, symbol);
                prtradereq->set_value(3, ptoin._trade_qty);
                prtradereq->set_value(4, requested_price);
                prtradereq->set_value(5, broker_id);

                TRACE( TRACE_TRX_FLOW, "App: %d TO:tr-add-tuple (%ld) \n", xct_id, trade_id);
                e = _ptrade_request_man->add_tuple(_pssm, prtradereq);
                if (e.is_error()) { goto done; }
            }

            /**
             *
             *	INSERT INTO TRADE_HISTORY	(
             *						TH_T_ID, TH_DTS, TH_ST_ID
             *					)
             *	VALUES 				(
             *						trade_id, // TH_T_ID
             *						now_dts, // TH_DTS
             *						status_id // TH_ST_ID
             *					)
             *
             **/

            prtradehist->set_value(0, trade_id);
            prtradehist->set_value(1, now_dts);
            prtradehist->set_value(2, status_id);

            TRACE( TRACE_TRX_FLOW, "App: %d TO:th-add-tuple (%ld) \n", xct_id, trade_id);
            e = _ptrade_history_man->add_tuple(_pssm, prtradehist);
            if (e.is_error()) { goto done; }
        }
        //END FRAME4

        //BEGIN FRAME5
        {
            if(ptoin._roll_it_back){			
                e = RC(se_NOT_FOUND);
                if (e.is_error()) { goto done; }		 
            }
        }

        //send TradeRequest to Market

        mee->SubmitTradeRequest(req);			  

        //END FRAME5
    } // goto

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rcustacct.print_tuple();
    rcust.print_tuple();
    rbrok.print_tuple();
    racctperm.print_tuple();
    rtrade.print_tuple();
    rtradereq.print_tuple();
    rtradehist.print_tuple();
    rcompany.print_tuple();
    rsecurity.print_tuple();
    rlasttrade.print_tuple();
    rtradetype.print_tuple();
    rholdingsummary.print_tuple();
    rholding.print_tuple();
    rcharge.print_tuple();
    rcusttaxrate.print_tuple();
    rtaxrate.print_tuple();
    rcommrate.print_tuple();
#endif

 done:
    // return the tuples to the cache
    _pcustomer_account_man->give_tuple(prcustacct);
    _pcustomer_man->give_tuple(prcust);
    _pbroker_man->give_tuple(prbrok);
    _paccount_permission_man->give_tuple(pracctperm);
    _ptrade_man->give_tuple(prtrade);
    _ptrade_request_man->give_tuple(prtradereq);
    _ptrade_history_man->give_tuple(prtradehist);
    _pcompany_man->give_tuple(prcompany);
    _psecurity_man->give_tuple(prsecurity);
    _plast_trade_man->give_tuple(prlasttrade);
    _ptrade_type_man->give_tuple(prtradetype);
    _pholding_summary_man->give_tuple(prholdingsummary);
    _pholding_man->give_tuple(prholding);
    _pcharge_man->give_tuple(prcharge);
    _pcustomer_taxrate_man->give_tuple(prcusttaxrate);
    _ptaxrate_man->give_tuple(prtaxrate);
    _pcommission_rate_man->give_tuple(prcommrate);
    if (req!=NULL) delete req;
    return (e);

}



w_rc_t ShoreTPCEEnv::xct_trade_result(const int xct_id, trade_result_input_t& ptrin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prtrade = _ptrade_man->get_tuple();	//149B
    assert (prtrade);

    table_row_t* prtradetype = _ptrade_type_man->get_tuple();
    assert (prtradetype);

    table_row_t* prholdingsummary = _pholding_summary_man->get_tuple();
    assert (prholdingsummary);

    table_row_t* prcustaccount = _pcustomer_account_man->get_tuple();
    assert (prcustaccount);

    table_row_t* prholding = _pholding_man->get_tuple();
    assert (prholding);

    table_row_t* prholdinghistory = _pholding_history_man->get_tuple();
    assert (prholdinghistory);

    table_row_t* prsecurity = _psecurity_man->get_tuple();	//194B
    assert (prsecurity);

    table_row_t* prbroker = _pbroker_man->get_tuple();
    assert (prbroker);

    table_row_t* prsettlement = _psettlement_man->get_tuple();
    assert (prsettlement);

    table_row_t* prcashtrans = _pcash_transaction_man->get_tuple();
    assert (prcashtrans);

    table_row_t* prcommissionrate = _pcommission_rate_man->get_tuple();
    assert (prcommissionrate);

    table_row_t* prcusttaxrate = _pcustomer_taxrate_man->get_tuple();
    assert (prcusttaxrate);

    table_row_t* prtaxrate = _ptaxrate_man->get_tuple();
    assert (prtaxrate);

    table_row_t* prcustomer = _pcustomer_man->get_tuple();	//280B
    assert (prcustomer);

    table_row_t* prtradehist = _ptrade_history_man->get_tuple();
    assert (prtradehist);

    w_rc_t e = RCOK;
    rep_row_t areprow(_pcustomer_man->ts());

    areprow.set(_pcustomer_desc->maxsize());

    prtrade->_rep = &areprow;
    prtradetype->_rep = &areprow;
    prholdingsummary->_rep = &areprow;
    prcustaccount->_rep = &areprow;
    prholding->_rep = &areprow;
    prholdinghistory->_rep = &areprow;
    prsecurity->_rep = &areprow;
    prbroker->_rep = &areprow;
    prsettlement->_rep = &areprow;
    prcashtrans->_rep = &areprow;
    prcommissionrate->_rep = &areprow;
    prcusttaxrate->_rep = &areprow;
    prtaxrate->_rep = &areprow;
    prcustomer->_rep = &areprow;
    prtradehist->_rep = &areprow;

    rep_row_t lowrep(_pcustomer_man->ts());
    rep_row_t highrep(_pcustomer_man->ts());

    lowrep.set(_pcustomer_desc->maxsize());
    highrep.set(_pcustomer_desc->maxsize());
    { // make gotos safe

        int trade_qty;
        TIdent acct_id = -1;
        bool type_is_sell;
        int hs_qty = -1;
        char symbol[16]; //15
        bool is_lifo;
        char type_id[4]; //3
        bool trade_is_cash;
        char type_name[13];	//12
        double charge;

        //BEGIN FRAME1
        {
            /**
             * 	SELECT 	acct_id = T_CA_ID, type_id = T_TT_ID, symbol = T_S_SYMB, trade_qty = T_QTY,
             *		charge = T_CHRG, is_lifo = T_LIFO, trade_is_cash = T_IS_CASH
             *	FROM	TRADE
             *	WHERE	T_ID = trade_id
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TR:t-idx-probe (%ld) \n", xct_id,  ptrin._trade_id);
            e =  _ptrade_man->t_index_probe(_pssm, prtrade, ptrin._trade_id);
            if (e.is_error()) { goto done; }

            prtrade->get_value(8, acct_id);
            prtrade->get_value(3, type_id, 4); 	//3
            prtrade->get_value(5, symbol, 16);	//15
            prtrade->get_value(6, trade_qty);
            prtrade->get_value(11, charge);
            prtrade->get_value(14, is_lifo);
            prtrade->get_value(4, trade_is_cash);

            assert(acct_id != -1); //Harness control

            /**
             * 	SELECT 	type_name = TT_NAME, type_is_sell = TT_IS_SELL, type_is_market = TT_IS_MRKT
             *	FROM	TRADE_TYPE
             *   	WHERE	TT_ID = type_id
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TR:tt-idx-probe (%s) \n", xct_id,  type_id);
            e =  _ptrade_type_man->tt_index_probe(_pssm, prtradetype, type_id);
            if (e.is_error()) { goto done; }

            prtradetype->get_value(1, type_name, 13); //12
            prtradetype->get_value(2, type_is_sell);
            bool type_is_market;
            prtradetype->get_value(3, type_is_market);

            /**
             * 	SELECT 	hs_qty = HS_QTY
             *	FROM	HOLDING_SUMMARY
             *  	WHERE	HS_CA_ID = acct_id and HS_S_SYMB = symbol
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TR:hs-idx-probe (%ld) (%s) \n", xct_id,  acct_id, symbol);
            e =  _pholding_summary_man->hs_index_probe_forupdate(_pssm, prholdingsummary, acct_id, symbol);
            if (e.is_error()) { goto done; }

            prholdingsummary->get_value(2, hs_qty);

            if(hs_qty == -1){ //-1 = NULL, no prior holdings exist
                hs_qty = 0;
            }
        }
        //END FRAME1

        TIdent cust_id;
        double buy_value = 0;
        double sell_value = 0;
        myTime trade_dts = time(NULL);
        TIdent broker_id;
        short tax_status;

        //BEGIN FRAME2
        {
            TIdent hold_id;
            double hold_price;
            int hold_qty;
            int needed_qty;

            needed_qty = trade_qty;
            /**
             *
             * 	SELECT 	broker_id = CA_B_ID, cust_id = CA_C_ID, tax_status = CA_TAX_ST
             *	FROM	CUSTOMER_ACCOUNT
             *	WHERE	CA_ID = acct_id
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TR:ca-idx-probe (%ld) \n", xct_id,  acct_id);
            e =  _pcustomer_account_man->ca_index_probe_forupdate(_pssm, prcustaccount, acct_id);
            if (e.is_error()) {  goto done; }

            prcustaccount->get_value(1, broker_id);
            prcustaccount->get_value(2, cust_id);
            prcustaccount->get_value(4, tax_status);

            if(type_is_sell){
                if(hs_qty == 0){
                    /**
                     *	INSERT INTO HOLDING_SUMMARY (
                     *					HS_CA_ID,
                     *					HS_S_SYMB,
                     *					HS_QTY
                     *				)
                     *	VALUES 			(
                     *					acct_id,
                     *					symbol,
                     *					-trade_qty
                     *				)
                     */
                    prholdingsummary->set_value(0, acct_id);
                    prholdingsummary->set_value(1, symbol);
                    prholdingsummary->set_value(2, -trade_qty);

                    TRACE( TRACE_TRX_FLOW, "App: %d TR:hs-add-tuple (%ld) \n", xct_id, acct_id);
                    e = _pholding_summary_man->add_tuple(_pssm, prholdingsummary);
                    if (e.is_error()) {  goto done; }
                }
                else{
                    if(hs_qty != trade_qty){
                        /**
                         *	UPDATE	HOLDING_SUMMARY
                         *	SET		HS_QTY = hs_qty - trade_qty
                         *	WHERE	HS_CA_ID = acct_id and HS_S_SYMB = symbol
                         */

                        TRACE( TRACE_TRX_FLOW, "App: %d TR:hs-update (%ld) (%s) (%d) \n", xct_id, acct_id, symbol, (hs_qty - trade_qty));
                        e = _pholding_summary_man->hs_update_qty(_pssm, prholdingsummary, acct_id, symbol, (hs_qty - trade_qty));
                        if (e.is_error()) {  goto done; }
                    }
                }
                if (hs_qty > 0) {
                    guard<index_scan_iter_impl<holding_t> > h_iter;
                    sort_scan_t* h_list_sort_iter;
                    if (is_lifo){
                        /**
                         *
                         * 	SELECT	H_T_ID, H_QTY, H_PRICE
                         *  	FROM	HOLDING
                         *	WHERE	H_CA_ID = acct_id and H_S_SYMB = symbol
                         *	ORDER BY H_DTS DESC
                         */
                        index_scan_iter_impl<holding_t>* tmp_h_iter;
                        TRACE( TRACE_TRX_FLOW, "App: %d TR:h-get-iter-by-idx2 \n", xct_id);
                        e = _pholding_man->h_get_iter_by_index2(_pssm, tmp_h_iter, prholding, lowrep, highrep, acct_id, symbol);
                        h_iter = tmp_h_iter;
                        if (e.is_error()) {  goto done; }

                        //descending order
                        rep_row_t sortrep(_pholding_man->ts());
                        sortrep.set(_pholding_desc->maxsize());
                        sort_buffer_t h_list(4);

                        h_list.setup(0, SQL_LONG);	//dts
                        h_list.setup(1, SQL_LONG); 	//id
                        h_list.setup(2, SQL_INT); 	//qty
                        h_list.setup(3, SQL_FLOAT);     //price

                        table_row_t rsb(&h_list);
                        sort_man_impl h_sorter(&h_list, &sortrep);
                        bool eof;
                        e = h_iter->next(_pssm, eof, *prholding);
                        if (e.is_error()) {  goto done; }
                        while (!eof) {
                            /* put the value into the sorted buffer */
                            char hs_s_symb[16];
                            prholding->get_value(2, hs_s_symb, 16);
							
                            if(strcmp(hs_s_symb, symbol) == 0){
                                int temp_qty;
                                myTime temp_dts;
                                TIdent temp_tid;
                                double temp_price;
							
                                prholding->get_value(5, temp_qty);
                                prholding->get_value(4, temp_price);
                                prholding->get_value(3, temp_dts);
                                prholding->get_value(0, temp_tid);

                                rsb.set_value(0, temp_dts);
                                rsb.set_value(1, temp_tid);
                                rsb.set_value(2, temp_qty);
                                rsb.set_value(3, temp_price);

                                h_sorter.add_tuple(rsb);
                                TRACE( TRACE_TRX_FLOW, "App: %d TR:hold-list (%ld)  \n", xct_id, temp_tid);
                            }
                            e = h_iter->next(_pssm, eof, *prholding);
                            if (e.is_error()) {  goto done; }
                        }
                        assert (h_sorter.count());

                        sort_iter_impl h_list_sort_iter(_pssm, &h_list, &h_sorter);
                        e = h_list_sort_iter.next(_pssm, eof, rsb);
                        if (e.is_error()) {  goto done; }
                        while(needed_qty != 0 && !eof){
                            TIdent hold_id;
                            int hold_qty;
                            double hold_price;

                            rsb.get_value(1, hold_id);
                            rsb.get_value(2, hold_qty);
                            rsb.get_value(3, hold_price);

                            if(hold_qty > needed_qty){
                                /**
                                 * 	INSERT INTO
                                 *		HOLDING_HISTORY (
                                 *		HH_H_T_ID,
                                 *		HH_T_ID,
                                 *		HH_BEFORE_QTY,
                                 *		HH_AFTER_QTY
                                 *	)
                                 *	VALUES(
                                 *		hold_id, // H_T_ID of original trade
                                 *		trade_id, // T_ID current trade
                                 *		hold_qty, // H_QTY now
                                 *		hold_qty - needed_qty // H_QTY after update
                                 *	)
                                 */

                                prholdinghistory->set_value(0, hold_id);
                                prholdinghistory->set_value(1, ptrin._trade_id);
                                prholdinghistory->set_value(2, hold_qty);
                                prholdinghistory->set_value(3, (hold_qty - needed_qty));

                                TRACE( TRACE_TRX_FLOW, "App: %d TR:hh-add-tuple (%ld) (%ld) (%d) (%d)\n", xct_id, hold_id, ptrin._trade_id, hold_price, (hold_qty - needed_qty));
                                e = _pholding_history_man->add_tuple(_pssm, prholdinghistory);
                                if (e.is_error()) {  goto done; }

                                /**
                                 * 	UPDATE 	HOLDING
                                 *	SET	H_QTY = hold_qty - needed_qty
                                 *	WHERE	current of hold_list
                                 */

                                TRACE( TRACE_TRX_FLOW, "App: %d TR:hold-update (%ld) (%s) (%d) \n", xct_id, acct_id, symbol, (hs_qty - trade_qty));
                                e = _pholding_man->h_update_qty(_pssm, prholding, hold_id, (hold_qty - needed_qty));
                                if (e.is_error()) {  goto done; }

                                buy_value += needed_qty * hold_price;
                                sell_value += needed_qty * ptrin._trade_price;
                                needed_qty = 0;
                            }
                            else{
                                /**
                                 * 	INSERT INTO
                                 *		HOLDING_HISTORY (
                                 *		HH_H_T_ID,
                                 *		HH_T_ID,
                                 *		HH_BEFORE_QTY,
                                 *		HH_AFTER_QTY
                                 *	)
                                 *	VALUES(
                                 *		hold_id, // H_T_ID of original trade
                                 *		trade_id, // T_ID current trade
                                 *		hold_qty, // H_QTY now
                                 *		0 // H_QTY after update
                                 *	)
                                 */

                                prholdinghistory->set_value(0, hold_id);
                                prholdinghistory->set_value(1, ptrin._trade_id);
                                prholdinghistory->set_value(2, hold_qty);
                                prholdinghistory->set_value(3, 0);

                                TRACE( TRACE_TRX_FLOW, "App: %d TR:hh-add-tuple (%ld) (%ld) (%d) (%d)\n", xct_id, hold_id, ptrin._trade_id, hold_price, 0);
                                e = _pholding_history_man->add_tuple(_pssm, prholdinghistory);
                                if (e.is_error()) {  goto done; }

                                /**
                                 * 	DELETE 	FROM	HOLDING
                                 *	WHERE	current of hold_list
                                 */

                                TRACE( TRACE_TRX_FLOW, "App: %d TR:h-delete-by-index (%ld) \n", xct_id, hold_id);
                                e = _pholding_man->h_delete_by_index(_pssm, prholding, hold_id);
                                if (e.is_error()) {  goto done; }

                                buy_value += hold_qty * hold_price;
                                sell_value += hold_qty * ptrin._trade_price;
                                needed_qty = needed_qty - hold_qty;
                            }

                            TRACE( TRACE_TRX_FLOW, "App: %d TR:h-iter-next \n", xct_id);
                            e = h_list_sort_iter.next(_pssm, eof, rsb);
                            if (e.is_error()) {  goto done; }
                        }
                    }
                    else{
                        /**
                         * 	SELECT	H_T_ID, H_QTY, H_PRICE
                         *  	FROM	HOLDING
                         *	WHERE	H_CA_ID = acct_id and H_S_SYMB = symbol
                         *	ORDER BY H_DTS ASC
                         */
						
                        {
                            index_scan_iter_impl<holding_t>* tmp_h_iter;
                            TRACE( TRACE_TRX_FLOW, "App: %d TR:h-get-iter-by-idx2 \n", xct_id);
                            e = _pholding_man->h_get_iter_by_index2(_pssm, tmp_h_iter, prholding, lowrep, highrep, acct_id, symbol);
                            if (e.is_error()) {  goto done; }
                            h_iter = tmp_h_iter;
                        }
                        //already sorted in ascending order because of its index

                        bool eof;
                        e = h_iter->next(_pssm, eof, *prholding);
                        if (e.is_error()) {  goto done; }
                        while(needed_qty != 0 && !eof){
                            char hs_s_symb[16];
                            prholding->get_value(2, hs_s_symb, 16);
							
                            if(strcmp(hs_s_symb, symbol) == 0)
                                {
                                    TIdent hold_id;
                                    int hold_qty;
                                    double hold_price;

                                    prholding->get_value(5, hold_qty);
                                    prholding->get_value(4, hold_price);
                                    prholding->get_value(0, hold_id);

                                    if(hold_qty > needed_qty){
                                        //Selling some of the holdings
                                        /**
                                         *
                                         * 	INSERT INTO
                                         *		HOLDING_HISTORY (
                                         *		HH_H_T_ID,
                                         *		HH_T_ID,
                                         *		HH_BEFORE_QTY,
                                         *		HH_AFTER_QTY
                                         *	)
                                         *	VALUES(
                                         *		hold_id, // H_T_ID of original trade
                                         *		trade_id, // T_ID current trade
                                         *		hold_qty, // H_QTY now
                                         *		hold_qty - needed_qty // H_QTY after update
                                         *	)
                                         */

                                        prholdinghistory->set_value(0, hold_id);
                                        prholdinghistory->set_value(1, ptrin._trade_id);
                                        prholdinghistory->set_value(2, hold_qty);
                                        prholdinghistory->set_value(3, (hold_qty - needed_qty));

                                        TRACE( TRACE_TRX_FLOW, "App: %d TR:hh-add-tuple (%ld) (%ld) (%d) (%d) \n", xct_id, hold_id, ptrin._trade_id, hold_price, (hold_qty - needed_qty));
                                        e = _pholding_history_man->add_tuple(_pssm, prholdinghistory);
                                        if (e.is_error()) {  goto done; }

                                        /**
                                         * 	UPDATE 	HOLDING
                                         *	SET	H_QTY = hold_qty - needed_qty
                                         *	WHERE	current of hold_list
                                         */

                                        TRACE( TRACE_TRX_FLOW, "App: %d TR:hold-update (%ld) (%s) (%d) \n", xct_id, acct_id, symbol, (hs_qty - trade_qty));
                                        //e = _pholding_man->h_update_qty(_pssm, prholding, hold_id, (hold_qty - needed_qty));
                                        if (e.is_error()) {  goto done; }

                                        buy_value += needed_qty * hold_price;
                                        sell_value += needed_qty * ptrin._trade_price;
                                        needed_qty = 0;
                                    }
                                    else{
                                        // Selling all holdings
                                        /**
                                         * 	INSERT INTO
                                         *		HOLDING_HISTORY (
                                         *		HH_H_T_ID,
                                         *		HH_T_ID,
                                         *		HH_BEFORE_QTY,
                                         *		HH_AFTER_QTY
                                         *	)
                                         *	VALUES(
                                         *		hold_id, // H_T_ID of original trade
                                         *		trade_id, // T_ID current trade
                                         *		hold_qty, // H_QTY now
                                         *		0 // H_QTY after update
                                         *	)
                                         */

                                        prholdinghistory->set_value(0, hold_id);
                                        prholdinghistory->set_value(1, ptrin._trade_id);
                                        prholdinghistory->set_value(2, hold_qty);
                                        prholdinghistory->set_value(3, 0);

                                        TRACE( TRACE_TRX_FLOW, "App: %d TR:hh-add-tuple (%d) (%ld) (%ld) (%d) \n", xct_id, hold_id, ptrin._trade_id, hold_price, 0);
                                        e = _pholding_history_man->add_tuple(_pssm, prholdinghistory);
                                        if (e.is_error()) {  goto done; }

                                        /**
                                         * 	DELETE 	FROM	HOLDING
                                         *	WHERE	current of hold_list
                                         */

                                        TRACE( TRACE_TRX_FLOW, "App: %d TR:h-delete-by-index (%ld) \n", xct_id, hold_id);
                                        e = _pholding_man->h_delete_by_index(_pssm, prholding, hold_id);
                                        if (e.is_error()) {  goto done; }

                                        buy_value += hold_qty * hold_price;
                                        sell_value += hold_qty * ptrin._trade_price;
                                        needed_qty = needed_qty - hold_qty;
                                    }
                                }
                            TRACE( TRACE_TRX_FLOW, "App: %d TR:h-iter-next \n", xct_id);
                            e = h_iter->next(_pssm, eof, *prholding);
                            if (e.is_error()) {  goto done; }
                        }
                    }
                }
                if (needed_qty > 0){
                    /**
                     * 	INSERT INTO
                     *		HOLDING_HISTORY (
                     *		HH_H_T_ID,
                     *		HH_T_ID,
                     *		HH_BEFORE_QTY,
                     *		HH_AFTER_QTY
                     *	)
                     *	VALUES(
                     *		trade_id, // T_ID current is original trade
                     *		trade_id, // T_ID current trade
                     *		0, // H_QTY before
                     *		(-1) * needed_qty // H_QTY after insert
                     *	)
                     */

                    prholdinghistory->set_value(0, ptrin._trade_id);
                    prholdinghistory->set_value(1, ptrin._trade_id);
                    prholdinghistory->set_value(2, 0);
                    prholdinghistory->set_value(3, (-1) * needed_qty);

                    TRACE( TRACE_TRX_FLOW, "App: %d TR:hh-add-tuple (%ld) (%ld) (%d) (%d) \n", xct_id, ptrin._trade_id, ptrin._trade_id, 0, (-1) * needed_qty);
                    e = _pholding_history_man->add_tuple(_pssm, prholdinghistory);
                    if (e.is_error()) {  goto done; }

                    /**
                     * 	INSERT INTO
                     *		HOLDING (
                     *				H_T_ID,
                     *				H_CA_ID,
                     *				H_S_SYMB,
                     *				H_DTS,
                     *				H_PRICE,
                     *				H_QTY
                     *			)
                     *	VALUES 		(
                     *				trade_id, // H_T_ID
                     *				acct_id, // H_CA_ID
                     *				symbol, // H_S_SYMB
                     *				trade_dts, // H_DTS
                     *				trade_price, // H_PRICE
                     *				(-1) * needed_qty //* H_QTY
                     *			)
                     */

                    prholding->set_value(0, ptrin._trade_id);
                    prholding->set_value(1, acct_id);
                    prholding->set_value(2, symbol);
                    prholding->set_value(3, trade_dts);
                    prholding->set_value(4, ptrin._trade_price);
                    prholding->set_value(5, (-1) * needed_qty);

                    TRACE( TRACE_TRX_FLOW, "App: %d TR:h-add-tuple (%ld) (%ld) (%s) (%ld) (%d) (%d) \n", xct_id, ptrin._trade_id, acct_id, symbol, trade_dts, ptrin._trade_price, (-1) * needed_qty);
                    e = _pholding_man->add_tuple(_pssm, prholding);
                    if (e.is_error()) {  goto done; }
                }
                else{
                    if (hs_qty == trade_qty){
                        /**
                         * DELETE FROM	HOLDING_SUMMARY
                         * WHERE HS_CA_ID = acct_id and HS_S_SYMB = symbol
                         */

                        TRACE( TRACE_TRX_FLOW, "App: %d TR:hs-delete-by-index (%ld) (%s) \n", xct_id, acct_id, symbol);
                        e = _pholding_summary_man->hs_delete_by_index(_pssm, prholdingsummary, acct_id, symbol);
                        if (e.is_error()) {  goto done; }
                    }
                }
            }
            else{
                if (hs_qty == 0){
                    /**
                     *	INSERT INTO HOLDING_SUMMARY (
                     *					HS_CA_ID,
                     *					HS_S_SYMB,
                     *					HS_QTY
                     *				   )
                     *	VALUES 			(
                     *					acct_id,
                     *					symbol,
                     *					trade_qty
                     *				)
                     **/

                    prholdingsummary->set_value(0, acct_id);
                    prholdingsummary->set_value(1, symbol);
                    prholdingsummary->set_value(2, trade_qty);

                    TRACE( TRACE_TRX_FLOW, "App: %d TR:hs-add-tuple (%ld) \n", xct_id, acct_id);
                    e = _pholding_summary_man->add_tuple(_pssm, prholdingsummary);
                    if (e.is_error()) {  goto done; }
                }
                else{
                    if (-hs_qty != trade_qty){
                        /**
                         *	UPDATE	HOLDING_SUMMARY
                         *	SET	HS_QTY = hs_qty + trade_qty
                         *	WHERE	HS_CA_ID = acct_id and HS_S_SYMB = symbol
                         */

                        TRACE( TRACE_TRX_FLOW, "App: %d TR:holdsumm-update (%ld) (%s) (%d) \n", xct_id, acct_id, symbol, (hs_qty + trade_qty));
                        e = _pholding_summary_man->hs_update_qty(_pssm, prholdingsummary, acct_id, symbol, (hs_qty + trade_qty));
                        if (e.is_error()) {  goto done; }
                    }

                    if(hs_qty < 0){
                        guard<index_scan_iter_impl<holding_t> > h_iter;
                        sort_scan_t* h_list_sort_iter;
                        if(is_lifo){
                            /**
                             * 	SELECT	H_T_ID, H_QTY, H_PRICE
                             *  	FROM	HOLDING
                             *	WHERE	H_CA_ID = acct_id and H_S_SYMB = symbol
                             *	ORDER BY H_DTS DESC
                             */

                            index_scan_iter_impl<holding_t>* tmp_h_iter;
                            TRACE( TRACE_TRX_FLOW, "App: %d TR:h-get-iter-by-idx2 \n", xct_id);
                            e = _pholding_man->h_get_iter_by_index2(_pssm, tmp_h_iter, prholding, lowrep, highrep, acct_id, symbol);
                            if (e.is_error()) {  goto done; }
                            h_iter = tmp_h_iter;

                            //descending order
                            rep_row_t sortrep(_pholding_man->ts());
                            sortrep.set(_pholding_desc->maxsize());
                            sort_buffer_t h_list(4);
							
                            h_list.setup(0, SQL_LONG);
                            h_list.setup(1, SQL_LONG);
                            h_list.setup(2, SQL_INT);
                            h_list.setup(3, SQL_FLOAT);

                            table_row_t rsb(&h_list);
                            sort_man_impl h_sorter(&h_list, &sortrep);

                            bool eof;
                            e = h_iter->next(_pssm, eof, *prholding);
                            if (e.is_error()) {  goto done; }
                            while (!eof) {
                                char hs_s_symb[16];
                                prholding->get_value(2, hs_s_symb, 16);
								
                                if(strcmp(hs_s_symb, symbol) == 0){
                                    /* put the value into the sorted buffer */
                                    int temp_qty;
                                    myTime temp_dts;
                                    TIdent temp_tid;
                                    double temp_price;

                                    prholding->get_value(5, temp_qty);
                                    prholding->get_value(4, temp_price);
                                    prholding->get_value(3, temp_dts);
                                    prholding->get_value(0, temp_tid);

                                    rsb.set_value(0, temp_dts);
                                    rsb.set_value(1, temp_tid);
                                    rsb.set_value(2, temp_qty);
                                    rsb.set_value(3, temp_price);

                                    h_sorter.add_tuple(rsb);
                                }
							
                                e = h_iter->next(_pssm, eof, *prholding);
                                if (e.is_error()) {  goto done; }
                            }
                            assert (h_sorter.count());

                            sort_iter_impl h_list_sort_iter(_pssm, &h_list, &h_sorter);
                            e = h_list_sort_iter.next(_pssm, eof, rsb);
                            if (e.is_error()) {  goto done; }
                            while(needed_qty != 0 && !eof){
                                TIdent hold_id;
                                int hold_qty;
                                double hold_price;

                                rsb.get_value(1, hold_id);
                                rsb.get_value(2, hold_qty);
                                rsb.get_value(3, hold_price);

                                if(hold_qty + needed_qty < 0){
                                    /**
                                     *
                                     * 	INSERT INTO
                                     *		HOLDING_HISTORY (
                                     *		HH_H_T_ID,
                                     *		HH_T_ID,
                                     *		HH_BEFORE_QTY,
                                     *		HH_AFTER_QTY
                                     *	)
                                     *	VALUES(
                                     *		hold_id, // H_T_ID of original trade
                                     *		trade_id, // T_ID current trade
                                     *		hold_qty, // H_QTY now
                                     *		hold_qty + needed_qty // H_QTY after update
                                     *	)
                                     *
                                     */

                                    prholdinghistory->set_value(0, hold_id);
                                    prholdinghistory->set_value(1, ptrin._trade_id);
                                    prholdinghistory->set_value(2, hold_qty);
                                    prholdinghistory->set_value(3, (hold_qty + needed_qty));

                                    TRACE( TRACE_TRX_FLOW, "App: %d TR:hh-add-tuple (%ld) (%ld) (%d) (%d)\n", xct_id, hold_id, ptrin._trade_id, hold_price, (hold_qty + needed_qty));
                                    e = _pholding_history_man->add_tuple(_pssm, prholdinghistory);
                                    if (e.is_error()) {  goto done; }

                                    /**
                                     * 	UPDATE 	HOLDING
                                     *	SET	H_QTY = hold_qty + needed_qty
                                     *	WHERE	current of hold_list
                                     */

                                    TRACE( TRACE_TRX_FLOW, "App: %d TR:h-update (%ld) (%s) (%d) \n", xct_id, acct_id, symbol, (hs_qty + trade_qty));
                                    e = _pholding_man->h_update_qty(_pssm, prholding, hold_id, (hold_qty + needed_qty));
                                    if (e.is_error()) {  goto done; }

                                    sell_value += needed_qty * hold_price;
                                    buy_value += needed_qty * ptrin._trade_price;
                                    needed_qty = 0;
                                }
                                else{
                                    /**
                                     *
                                     * 	INSERT INTO
                                     *		HOLDING_HISTORY (
                                     *		HH_H_T_ID,
                                     *		HH_T_ID,
                                     *		HH_BEFORE_QTY,
                                     *		HH_AFTER_QTY
                                     *	)
                                     *	VALUES(
                                     *		hold_id, // H_T_ID of original trade
                                     *		trade_id, // T_ID current trade
                                     *		hold_qty, // H_QTY now
                                     *		0 // H_QTY after update
                                     *	)
                                     */

                                    TIdent ttt = hold_id;
                                    prholdinghistory->set_value(0, hold_id);
                                    prholdinghistory->set_value(1, ptrin._trade_id);
                                    prholdinghistory->set_value(2, hold_qty);
                                    prholdinghistory->set_value(3, 0);

                                    TRACE( TRACE_TRX_FLOW, "App: %d TR:hh-add-tuple (%ld) (%ld) (%d) (%d) \n", xct_id, hold_id, ptrin._trade_id, hold_price, 0);
                                    e = _pholding_history_man->add_tuple(_pssm, prholdinghistory);
                                    if (e.is_error()) {  goto done; }

                                    /**
                                     * DELETE FROM	HOLDING
                                     * WHERE current of hold_list
                                     */

                                    TRACE( TRACE_TRX_FLOW, "App: %d TR:h-delete-by-index (%ld) \n", ttt);
                                    e = _pholding_man->h_delete_by_index(_pssm, prholding,ttt);
                                    if (e.is_error()) {  goto done; }

                                    hold_qty = -hold_qty;
                                    sell_value += hold_qty * hold_price;
                                    buy_value += hold_qty * ptrin._trade_price;
                                    needed_qty = needed_qty - hold_qty;
                                }

                                TRACE( TRACE_TRX_FLOW, "App: %d TR:h-iter-next \n", xct_id);
                                e = h_list_sort_iter.next(_pssm, eof, rsb);
                                if (e.is_error()) {  goto done; }
                            }
                        }
                        else{
                            /**
                             * 	SELECT	H_T_ID, H_QTY, H_PRICE
                             *  FROM	HOLDING
                             *	WHERE	H_CA_ID = acct_id and H_S_SYMB = symbol
                             *	ORDER  BY H_DTS ASC
                             */

                            index_scan_iter_impl<holding_t>* tmp_h_iter;
                            TRACE( TRACE_TRX_FLOW, "App: %d TR:h-get-iter-by-idx2\n", xct_id);
                            e = _pholding_man->h_get_iter_by_index2(_pssm, tmp_h_iter, prholding, lowrep, highrep, acct_id, symbol);
                            if (e.is_error()) {  goto done; }
                            h_iter = tmp_h_iter;
                            //already sorted in ascending order because of index

                            bool eof;
                            e = h_iter->next(_pssm, eof, *prholding);
                            if (e.is_error()) {  goto done; }
                            while(needed_qty != 0 && !eof){
                                char hs_s_symb[16];
                                prholding->get_value(2, hs_s_symb, 16);
							
                                if(strcmp(hs_s_symb, symbol) == 0){
                                    TIdent hold_id;
                                    int hold_qty;
                                    double hold_price;

                                    prholding->get_value(5, hold_qty);
                                    prholding->get_value(4, hold_price);
                                    prholding->get_value(0, hold_id);

                                    if(hold_qty + needed_qty < 0){
                                        /**
                                         * 	INSERT INTO
                                         *		HOLDING_HISTORY (
                                         *		HH_H_T_ID,
                                         *		HH_T_ID,
                                         *		HH_BEFORE_QTY,
                                         *		HH_AFTER_QTY
                                         *	)
                                         *	VALUES(
                                         *		hold_id, // H_T_ID of original trade
                                         *		trade_id, // T_ID current trade
                                         *		hold_qty, // H_QTY now
                                         *		hold_qty + needed_qty // H_QTY after update
                                         *	)
                                         */

                                        prholdinghistory->set_value(0, hold_id);
                                        prholdinghistory->set_value(1, ptrin._trade_id);
                                        prholdinghistory->set_value(2, hold_qty);
                                        prholdinghistory->set_value(3, (hold_qty + needed_qty));

                                        TRACE( TRACE_TRX_FLOW, "App: %d TR:hh-add-tuple (%ld) (%ld) (%d) (%d)\n", xct_id, hold_id, ptrin._trade_id, hold_price, (hold_qty + needed_qty));
                                        e = _pholding_history_man->add_tuple(_pssm, prholdinghistory);
                                        if (e.is_error()) {  goto done; }

                                        /**
                                         * 	UPDATE 	HOLDING
                                         *	SET	H_QTY = hold_qty + needed_qty
                                         *	WHERE	current of hold_list
                                         */

                                        TRACE( TRACE_TRX_FLOW, "App: %d TR:h-update (%ld) (%s) (%d) \n", xct_id, acct_id, symbol, (hs_qty + trade_qty));
                                        e = _pholding_man->h_update_qty(_pssm, prholding, hold_id, (hold_qty + needed_qty));
                                        if (e.is_error()) {  goto done; }

                                        sell_value += needed_qty * hold_price;
                                        buy_value += needed_qty * ptrin._trade_price;
                                        needed_qty = 0;
                                    }
                                    else{
                                        /**
                                         * 	INSERT INTO
                                         *		HOLDING_HISTORY (
                                         *		HH_H_T_ID,
                                         *		HH_T_ID,
                                         *		HH_BEFORE_QTY,
                                         *		HH_AFTER_QTY
                                         *	)
                                         *	VALUES(
                                         *		hold_id, // H_T_ID of original trade
                                         *		trade_id, // T_ID current trade
                                         *		hold_qty, // H_QTY now
                                         *		0 // H_QTY after update
                                         *	)
                                         */

                                        prholdinghistory->set_value(0, hold_id);
                                        prholdinghistory->set_value(1, ptrin._trade_id);
                                        prholdinghistory->set_value(2, hold_qty);
                                        prholdinghistory->set_value(3, 0);

                                        TRACE( TRACE_TRX_FLOW, "App: %d TR:hh-add-tuple (%ld) (%ld) (%d) (%d)\n", xct_id, hold_id, ptrin._trade_id, hold_price, 0);
                                        e = _pholding_history_man->add_tuple(_pssm, prholdinghistory);
                                        if (e.is_error()) {  goto done; }

                                        /**
                                         * DELETE FROM	HOLDING
                                         * WHERE current of hold_list
                                         */

                                        TRACE( TRACE_TRX_FLOW, "App: %d TR:h-delete-by-index (%ld) (%s) \n", xct_id, acct_id, symbol);
                                        e = _pholding_man->h_delete_by_index(_pssm, prholding, hold_id);
                                        if (e.is_error()) {  goto done; }

                                        hold_qty = -hold_qty;
                                        sell_value += hold_qty * hold_price;
                                        buy_value += hold_qty * ptrin._trade_price;
                                        needed_qty = needed_qty - hold_qty;
                                    }
                                }
                                TRACE( TRACE_TRX_FLOW, "App: %d TR:h-iter-next \n", xct_id);
                                e = h_iter->next(_pssm, eof, *prholding);
                                if (e.is_error()) {  goto done; }
                            }
                        }
                    }
                    if (needed_qty > 0){
                        /**
                         * 	INSERT INTO
                         *		HOLDING_HISTORY (
                         *		HH_H_T_ID,
                         *		HH_T_ID,
                         *		HH_BEFORE_QTY,
                         *		HH_AFTER_QTY
                         *	)
                         *	VALUES(
                         *		trade_id, // T_ID current is original trade
                         *		trade_id, // T_ID current trade
                         *		0, // H_QTY before
                         *		needed_qty // H_QTY after insert
                         *	)
                         */

                        prholdinghistory->set_value(0, ptrin._trade_id);
                        prholdinghistory->set_value(1, ptrin._trade_id);
                        prholdinghistory->set_value(2, 0);
                        prholdinghistory->set_value(3, needed_qty);

                        TRACE( TRACE_TRX_FLOW, "App: %d TR:hh-add-tuple (%ld) (%ld) (%d) (%d)\n", xct_id, ptrin._trade_id, ptrin._trade_id, 0, needed_qty);
                        e = _pholding_history_man->add_tuple(_pssm, prholdinghistory);
                        if (e.is_error()) {  goto done; }

                        /**
                         *
                         * 	INSERT INTO
                         *		HOLDING (
                         *				H_T_ID,
                         *				H_CA_ID,
                         *				H_S_SYMB,
                         *				H_DTS,
                         *				H_PRICE,
                         *				H_QTY
                         *			)
                         *	VALUES 		(
                         *				trade_id, // H_T_ID
                         *				acct_id, // H_CA_ID
                         *				symbol, // H_S_SYMB
                         *				trade_dts, // H_DTS
                         *				trade_price, // H_PRICE
                         *				needed_qty // H_QTY
                         *			)
                         */

                        prholding->set_value(0, ptrin._trade_id);
                        prholding->set_value(1, acct_id);
                        prholding->set_value(2, symbol);
                        prholding->set_value(3, trade_dts);
                        prholding->set_value(4, ptrin._trade_price);
                        prholding->set_value(5, needed_qty);

                        TRACE( TRACE_TRX_FLOW, "App: %d TR:h-add-tuple (%ld) (%ld) (%s) (%d) (%d) (%d) \n", xct_id, ptrin._trade_id, acct_id, symbol, trade_dts,  ptrin._trade_price, needed_qty);
                        e = _pholding_man->add_tuple(_pssm, prholding);
                        if (e.is_error()) {  goto done; }
                    }
                    else if ((-hs_qty) == trade_qty){
                        /**
                         * DELETE FROM	HOLDING_SUMMARY
                         * WHERE HS_CA_ID = acct_id and HS_S_SYMB = symbol
                         */

                        TRACE( TRACE_TRX_FLOW, "App: %d TR:hs-delete-by-index (%ld) (%s) \n", xct_id, acct_id, symbol);
                        e = _pholding_summary_man->hs_delete_by_index(_pssm, prholdingsummary, acct_id, symbol);
                        if (e.is_error()) {  goto done; }
                    }
                }
            }
        }
        //END FRAME2


        //BEGIN FRAME3
        double tax_amount = 0;
        if((tax_status == 1 || tax_status == 2) && (sell_value > buy_value))
            {
                double tax_rates = 0;
                /**
                 *	SELECT 	tax_rates = sum(TX_RATE)
                 *	FROM	TAXRATE
                 *	WHERE  	TX_ID in ( 	SELECT	CX_TX_ID
                 *				FROM	CUSTOMER_TAXRATE
                 *				WHERE	CX_C_ID = cust_id)
                 */

                guard< index_scan_iter_impl<customer_taxrate_t> > cx_iter;
                {
                    index_scan_iter_impl<customer_taxrate_t>* tmp_cx_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TR:ct-iter-by-idx (%ld) \n", xct_id, cust_id);
                    e = _pcustomer_taxrate_man->cx_get_iter_by_index(_pssm, tmp_cx_iter, prcusttaxrate, lowrep, highrep, cust_id);
                    if (e.is_error()) {  goto done; }
                    cx_iter = tmp_cx_iter;
                }
                bool eof;
                e = cx_iter->next(_pssm, eof, *prcusttaxrate);
                if (e.is_error()) {  goto done; }
                while (!eof) {
                    char tax_id[5]; //4
                    prcusttaxrate->get_value(0, tax_id, 5); //4

                    TRACE( TRACE_TRX_FLOW, "App: %d TR:tx-idx-probe (%s) \n", xct_id, tax_id);
                    e =  _ptaxrate_man->tx_index_probe(_pssm, prtaxrate, tax_id);
                    if (e.is_error()) {  goto done; }

                    double rate;
                    prtaxrate->get_value(2, rate);

                    tax_rates += rate;

                    e = cx_iter->next(_pssm, eof, *prcusttaxrate);
                    if (e.is_error()) {  goto done; }
                }
                tax_amount = (sell_value - buy_value) * tax_rates;

                /**
                 *	UPDATE	TRADE
                 *	SET	T_TAX = tax_amount
                 *	WHERE	T_ID = trade_id
                 */

                TRACE( TRACE_TRX_FLOW, "App: %d TR:t-upd-tax-by-ind (%ld) (%d)\n", xct_id, ptrin._trade_id, tax_amount);
                e = _ptrade_man->t_update_tax_by_index(_pssm, prtrade, ptrin._trade_id, tax_amount);
                if (e.is_error()) {  goto done; }

                assert(tax_amount > 0); //Harness control
            }

        //END FRAME3

        double comm_rate = 0;
        char s_name[51]; //50
        //BEGIN FRAME4
        {
            /**
             *	SELECT	s_ex_id = S_EX_ID, s_name = S_NAME
             *	FROM	SECURITY
             *	WHERE	S_SYMB = symbol
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TR:security-idx-probe (%s) \n", xct_id,  symbol);
            e =  _psecurity_man->s_index_probe(_pssm, prsecurity, symbol);
            if (e.is_error()) {  goto done; }

            char s_ex_id[7]; //6
            prsecurity->get_value(4, s_ex_id, 7); //7
            prsecurity->get_value(3, s_name, 51); //50

            /**
             *	SELECT	c_tier = C_TIER
             *	FROM	CUSTOMER
             *	WHERE	C_ID = cust_id
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TR:c-idx-probe (%ld) \n", xct_id,  cust_id);
            e = _pcustomer_man->c_index_probe(_pssm, prcustomer, cust_id);
            if (e.is_error()) {  goto done; }

            short c_tier;
            prcustomer->get_value(7, c_tier);

            /**
             *	SELECT	1 row comm_rate = CR_RATE
             *	FROM	COMMISSION_RATE
             *	WHERE	CR_C_TIER = c_tier and CR_TT_ID = type_id and CR_EX_ID = s_ex_id and
             *			CR_FROM_QTY <= trade_qty and CR_TO_QTY >= trade_qty
             */

            guard< index_scan_iter_impl<commission_rate_t> > cr_iter;
            {
                index_scan_iter_impl<commission_rate_t>* tmp_cr_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d TR:cr-iter-by-idx (%d) (%s) (%ld) (%d) \n", xct_id, c_tier, type_id, s_ex_id, trade_qty);
                e = _pcommission_rate_man->cr_get_iter_by_index(_pssm, tmp_cr_iter, prcommissionrate, lowrep, highrep, c_tier, type_id, s_ex_id, trade_qty);
                if (e.is_error()) {  goto done; }
                cr_iter = tmp_cr_iter;
            }

            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d TR:cr-iter-next \n", xct_id);
            e = cr_iter->next(_pssm, eof, *prcommissionrate);
            if (e.is_error()) {  goto done; }
            while (!eof) {			  
                short cr_c_tier;
                prcommissionrate->get_value(0, cr_c_tier);
				
                char cr_tt_id[4], cr_ex_id[7];
                prcommissionrate->get_value(1, cr_tt_id, 4);
                prcommissionrate->get_value(2, cr_ex_id, 7);	
				
                int to_qty;
                prcommissionrate->get_value(4, to_qty);
                if(cr_c_tier == c_tier && strcmp(cr_tt_id, type_id) == 0 && strcmp(cr_ex_id, s_ex_id) == 0 && to_qty >= trade_qty){				
                    prcommissionrate->get_value(5, comm_rate);
                    break;
                }

                TRACE( TRACE_TRX_FLOW, "App: %d TR:cr-iter-next \n", xct_id);
                e = cr_iter->next(_pssm, eof, *prcommissionrate);
                if (e.is_error()) {  goto done; }
            }
        }
        //END FRAME4
        assert(comm_rate > 0.00);  //Harness control

        double comm_amount = (comm_rate / 100) * (trade_qty * ptrin._trade_price);
        char st_completed_id[5] = "CMPT"; //4
        //BEGIN FRAME5
        {
            /**
             *	UPDATE	TRADE
             * 	SET	T_COMM = comm_amount, T_DTS = trade_dts, T_ST_ID = st_completed_id,
             *		T_TRADE_PRICE = trade_price
             *	WHERE	T_ID = trade_id
             */

            prtrade->set_value(0, ptrin._trade_id);
            TRACE( TRACE_TRX_FLOW, "App: %d TR:t-upd-ca_td_sci_tp-by-ind (%ld) (%lf) (%ld) (%s) (%lf) \n",
                   xct_id, ptrin._trade_id, comm_amount, trade_dts, st_completed_id, ptrin._trade_price);
            e = _ptrade_man->t_update_ca_td_sci_tp_by_index(_pssm, prtrade, ptrin._trade_id, comm_amount, trade_dts, st_completed_id, ptrin._trade_price);
            if (e.is_error()) {  goto done; }

            /**
             *	INSERT INTO TRADE_HISTORY(
             *					TH_T_ID, TH_DTS, TH_ST_ID
             *				)
             *	VALUES 			(
             *					trade_id, // TH_T_ID
             *					now_dts, // TH_DTS
             *					st_completed_id // TH_ST_ID
             *				)
             */

            myTime now_dts = time(NULL);

            prtradehist->set_value(0, ptrin._trade_id);
            prtradehist->set_value(1, now_dts);
            prtradehist->set_value(2, st_completed_id);

            TRACE( TRACE_TRX_FLOW, "App: %d TR:th-add-tuple (%ld) \n", xct_id, ptrin._trade_id);
            e = _ptrade_history_man->add_tuple(_pssm, prtradehist);
            if (e.is_error()) {  goto done; }

            /**
             *	UPDATE	BROKER
             *	SET	B_COMM_TOTAL = B_COMM_TOTAL + comm_amount, B_NUM_TRADES = B_NUM_TRADES + 1
             *	WHERE	B_ID = broker_id
             */

            TRACE( TRACE_TRX_FLOW, "App: %d TR:broker-upd-ca_td_sci_tp-by-ind (%ld) (%d) \n", xct_id, broker_id, comm_amount);
            e = _pbroker_man->broker_update_ca_nt_by_index(_pssm, prbroker, broker_id, comm_amount);
            if (e.is_error()) {  goto done; }
        }
        //END FRAME5

        myTime due_date = time(NULL) + 48*60*60; //add 2 days
        double se_amount;
        if(type_is_sell){
            se_amount = (trade_qty * ptrin._trade_price) - charge - comm_amount;
        }
        else{
            se_amount = -((trade_qty * ptrin._trade_price) + charge + comm_amount);
        }
        if (tax_status == 1){
            se_amount = se_amount - tax_amount;
        }

        //BEGIN FRAME6
        {
            char cash_type[41] = "\0"; //40
            if(trade_is_cash){
                strcpy (cash_type, "Cash Account");
            }
            else{
                strcpy(cash_type,"Margin");
            }

            /**
             *	INSERT INTO SETTLEMENT (
             *					SE_T_ID,
             *					SE_CASH_TYPE,
             *					SE_CASH_DUE_DATE,
             *					SE_AMT
             *				)
             *	VALUES 			(
             * 					trade_id,
             *					cash_type,
             *					due_date,
             *					se_amount
             *				)
             */

            prsettlement->set_value(0, ptrin._trade_id);
            prsettlement->set_value(1, cash_type);
            prsettlement->set_value(2, due_date);
            prsettlement->set_value(3, se_amount);

            TRACE( TRACE_TRX_FLOW, "App: %d TR:se-add-tuple (%ld)\n", xct_id, ptrin._trade_id);
            e = _psettlement_man->add_tuple(_pssm, prsettlement);
            if (e.is_error()) {  goto done; }

            if(trade_is_cash){
                /**
                 *	update 	CUSTOMER_ACCOUNT
                 *	set	CA_BAL = CA_BAL + se_amount
                 *	where	CA_ID = acct_id
                 */

                TRACE( TRACE_TRX_FLOW, "App: %d TR:ca-upd-tuple \n", xct_id);
                e = _pcustomer_account_man->ca_update_bal(_pssm, prcustaccount, se_amount);
                if (e.is_error()) {  goto done; }

                /**
                 *	insert into	CASH_TRANSACTION 	(
                 *							CT_DTS,
                 *							CT_T_ID,
                 *							CT_AMT,
                 *							CT_NAME
                 *						)
                 *	values 					(
                 *							trade_dts,
                 *							trade_id,
                 *							se_amount,
                 *							type_name + " " + trade_qty + " shares of " + s_name
                 *						)
                 */

                prcashtrans->set_value(0, ptrin._trade_id);
                prcashtrans->set_value(1, trade_dts);
                prcashtrans->set_value(2, se_amount);

                stringstream ss;
                ss << type_name << " " << trade_qty << " shares of " << s_name;
                prcashtrans->set_value(3, ss.str().c_str());

                TRACE( TRACE_TRX_FLOW, "App: %d TR:ct-add-tuple (%ld) (%ld) (%lf) (%s)\n", xct_id, trade_dts, ptrin._trade_id, se_amount, ss.str().c_str());
                e = _pcash_transaction_man->add_tuple(_pssm, prcashtrans);
                if (e.is_error()) {  goto done; }
            }

            /**
             *	select	acct_bal = CA_BAL
             *	from	CUSTOMER_ACCOUNT
             *	where	CA_ID = acct_id
             */

            double acct_bal;
            prcustaccount->get_value(5, acct_bal);
        }
        //END FRAME6
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rtrade.print_tuple();
    rtradetype.print_tuple();
    rholdingsummary.print_tuple();
    rcustaccount.print_tuple();
    rholding.print_tuple();
    rsecurity.print_tuple();
    rbroker.print_tuple();
    rsettlement.print_tuple();
    rcashtrans.print_tuple();
    rcommussionrate.print_tuple();
    rcusttaxrate.print_tuple();
    rtaxrate.print_tuple();
    rcustomer.print_tuple();
    rtradehist.print_tuple();
#endif

 done:
    // return the tuples to the cache
    _ptrade_man->give_tuple(prtrade);
    _ptrade_type_man->give_tuple(prtradetype);
    _pholding_summary_man->give_tuple(prholdingsummary);
    _pcustomer_account_man->give_tuple(prcustaccount);
    _pholding_man->give_tuple(prholding);
    _pholding_history_man->give_tuple(prholdinghistory);
    _psecurity_man->give_tuple(prsecurity);
    _pbroker_man->give_tuple(prbroker);
    _psettlement_man->give_tuple(prsettlement);
    _pcash_transaction_man->give_tuple(prcashtrans);
    _pcommission_rate_man->give_tuple(prcommissionrate);
    _pcustomer_taxrate_man->give_tuple(prcusttaxrate);
    _ptaxrate_man->give_tuple(prtaxrate);
    _pcustomer_man->give_tuple(prcustomer);
    _ptrade_history_man->give_tuple(prtradehist);

    return (e);
}


w_rc_t ShoreTPCEEnv::xct_trade_lookup(const int xct_id, trade_lookup_input_t& ptlin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prtrade = _ptrade_man->get_tuple(); //149B
    assert (prtrade);

    table_row_t* prtradetype = _ptrade_type_man->get_tuple();
    assert (prtradetype);

    table_row_t* prsettlement = _psettlement_man->get_tuple();
    assert (prsettlement);

    table_row_t* prcashtrans = _pcash_transaction_man->get_tuple();
    assert (prcashtrans);

    table_row_t* prtradehist = _ptrade_history_man->get_tuple();
    assert (prtradehist);

    table_row_t* prholdinghistory = _pholding_history_man->get_tuple();
    assert (prholdinghistory);

    w_rc_t e = RCOK;
    rep_row_t areprow(_ptrade_man->ts());

    areprow.set(_ptrade_desc->maxsize());

    prtrade->_rep = &areprow;
    prtradetype->_rep = &areprow;
    prsettlement->_rep = &areprow;
    prcashtrans->_rep = &areprow;
    prtradehist->_rep = &areprow;
    prholdinghistory->_rep = &areprow;

    rep_row_t lowrep(_ptrade_man->ts());
    rep_row_t highrep(_ptrade_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_ptrade_desc->maxsize());
    highrep.set(_ptrade_desc->maxsize());

    {
        /*
          TRACE( TRACE_TRX_FLOW, "TL: %d SE:se-idx-probe (%ld) \n", xct_id,  8932182);
          e = _psettlement_man->se_index_probe(_pssm, prsettlement, 8932182);
          if (e.is_error()) { goto done; }
        */
    }

    unsigned int max_trades = 20; //instead of ptlin._max_trades

    {
        //ptlin.print();

        array_guard_t<double> bid_price = new double[max_trades];
        array_guard_t< char[50] > exec_name = new char[max_trades][50]; //49
        array_guard_t<bool> is_cash = new bool[max_trades];
        array_guard_t<bool> is_market = new bool[max_trades];
        array_guard_t<double> trade_price = new double[max_trades];

        array_guard_t<TIdent> trade_list = new TIdent[max_trades];

        array_guard_t<double> settlement_amount = new double[max_trades];
        array_guard_t<myTime> settlement_cash_due_date = new myTime[max_trades];
        array_guard_t< char[41] > settlement_cash_type = new char[max_trades][41]; //40

        array_guard_t<double> cash_transaction_amount = new double[max_trades];
        array_guard_t<myTime> cash_transaction_dts = new myTime[max_trades];
        array_guard_t< char[101] > cash_transaction_name = new char[max_trades][101]; //100

        array_guard_t< myTime[3] > trade_history_dts = new myTime[max_trades][3];
        array_guard_t< char[3][5] > trade_history_status_id = new char[max_trades][3][5]; //4

        array_guard_t<TIdent> acct_id = new TIdent[max_trades];
        array_guard_t<int> quantity = new int[max_trades];
        array_guard_t< char[4] > trade_type = new char[max_trades][4]; //3
        array_guard_t<myTime> trade_dts = new myTime[max_trades];

        //BEGIN FRAME1
        int num_found = 0;
        if(ptlin._frame_to_execute == 1)
            {
                int i;
                for (i = 0; i < max_trades; i++){
                    /**
                     *	select
                     *		bid_price[i] = T_BID_PRICE,
                     *		exec_name[i] = T_EXEC_NAME,
                     *		is_cash[i] = T_IS_CASH,
                     *		is_market[i] = TT_IS_MRKT,
                     *		trade_price[i] = T_TRADE_PRICE
                     *	from
                     *		TRADE,
                     *		TRADE_TYPE
                     *	where
                     *		T_ID = trade_id[i] and
                     *		T_TT_ID = TT_ID
                     */

                    TRACE( TRACE_TRX_FLOW, "App: %d TL:t-idx-probe (%ld) \n", xct_id,  ptlin._trade_id[i]);
                    e = _ptrade_man->t_index_probe(_pssm, prtrade, ptlin._trade_id[i]);
                    if (e.is_error()) { goto done; }

                    char tt_id[4]; //3
                    prtrade->get_value(3, tt_id, 4);

                    TRACE( TRACE_TRX_FLOW, "App: %d TL:tt-idx-probe (%s) \n", xct_id,  tt_id);
                    e = _ptrade_type_man->tt_index_probe(_pssm, prtradetype, tt_id);
                    if (e.is_error()) { goto done; }

                    prtrade->get_value(7, bid_price[i]);
                    prtrade->get_value(9, exec_name[i],50); //49
                    prtrade->get_value(4, is_cash[i]);
                    prtradetype->get_value(3, is_market[i]);
                    prtrade->get_value(10, trade_price[i]);

                    num_found++;

                    /**
                     *	select
                     *		settlement_amount[i] = SE_AMT,
                     *		settlement_cash_due_date[i] = SE_CASH_DUE_DATE,
                     *		settlement_cash_type[i] = SE_CASH_TYPE
                     *	from
                     *		SETTLEMENT
                     *	where
                     *		SE_T_ID = trade_id[i]
                     */

                    TRACE( TRACE_TRX_FLOW, "App: %d TL:se-idx-probe (%ld) \n", xct_id,  ptlin._trade_id[i]);
                    e = _psettlement_man->se_index_probe(_pssm, prsettlement, ptlin._trade_id[i]);
                    if (e.is_error()) { goto done; }

                    prsettlement->get_value(3, settlement_amount[i]);
                    prsettlement->get_value(2, settlement_cash_due_date[i]);
                    prsettlement->get_value(1, settlement_cash_type[i], 41); //40

                    // get cash information if this is a cash transaction
                    // Should only return one row for each trade that was a cash transaction
                    if(is_cash[i]){
                        /**
                         *	select
                         *		cash_transaction_amount[i] = CT_AMT,
                         *		cash_transaction_dts[i] = CT_DTS,
                         *		cash_transaction_name[i] = CT_NAME
                         *	from
                         *		CASH_TRANSACTION
                         *	where
                         *		CT_T_ID = trade_id[i]
                         *
                         */

                        TRACE( TRACE_TRX_FLOW, "App: %d TL:ct-idx-probe (%ld) \n", xct_id,  ptlin._trade_id[i]);
                        e = _pcash_transaction_man->ct_index_probe(_pssm, prcashtrans, ptlin._trade_id[i]);
                        if (e.is_error()) { goto done; }

                        prcashtrans->get_value(2, cash_transaction_amount[i]);
                        prcashtrans->get_value(1, cash_transaction_dts[i]);
                        prcashtrans->get_value(3, cash_transaction_name[i], 101); //100

                        /**
                         *	select first 3 rows
                         *		trade_history_dts[i][] = TH_DTS,
                         *		trade_history_status_id[i][] = TH_ST_ID
                         *	from
                         *		TRADE_HISTORY
                         *	where
                         *		TH_T_ID = trade_id[i]
                         *	order by
                         *		TH_DTS
                         */

                        guard<index_scan_iter_impl<trade_history_t> > th_iter;
                        {
                            index_scan_iter_impl<trade_history_t>* tmp_th_iter;
                            TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-by-trade-idx (%ld) \n", xct_id, ptlin._trade_id[i]);
                            e = _ptrade_history_man->th_get_iter_by_index(_pssm, tmp_th_iter, prtradehist, lowrep, highrep, ptlin._trade_id[i]);
                            if (e.is_error()) { goto done; }
                            th_iter = tmp_th_iter;
                        }

                        //ascending order
                        rep_row_t sortrep(_ptrade_man->ts());
                        sortrep.set(_ptrade_desc->maxsize());
                        asc_sort_buffer_t th_list(2);
                        th_list.setup(0, SQL_LONG);
                        th_list.setup(1, SQL_FIXCHAR, 4);

                        table_row_t rsb(&th_list);
                        asc_sort_man_impl th_sorter(&th_list, &sortrep);

                        bool eof;
                        TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                        e = th_iter->next(_pssm, eof, *prtradehist);
                        if (e.is_error()) { goto done; }
                        while (!eof) {
                            /* put the value into the sorted buffer */
                            myTime temp_dts;
                            char temp_stid[5]; //4

                            prtradehist->get_value(1, temp_dts);
                            prtradehist->get_value(2, temp_stid, 5);

                            rsb.set_value(0, temp_dts);
                            rsb.set_value(1, temp_stid);

                            th_sorter.add_tuple(rsb);

                            TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                            e = th_iter->next(_pssm, eof, *prtradehist);
                            if (e.is_error()) { goto done; }
                        }
                        assert (th_sorter.count());

                        asc_sort_iter_impl th_list_sort_iter(_pssm, &th_list, &th_sorter);
                        TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                        e = th_list_sort_iter.next(_pssm, eof, rsb);
                        if (e.is_error()) { goto done; }
                        for(int j = 0; j < 3 && !eof; j++) {
                            rsb.get_value(0, trade_history_dts[i][j]);
                            rsb.get_value(1, trade_history_status_id[i][j], 5);

                            TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                            e = th_list_sort_iter.next(_pssm, eof, rsb);
                            if (e.is_error()) { goto done; }
                        }
                    }
                }
                assert(num_found == max_trades); //Harness control
            }
        //END FRAME1

        //BEGIN FRAME2
        if(ptlin._frame_to_execute == 2)
            {
                int i;
                // Get trade information
                // Should return between 0 and max_trades rows

                /**
                 *	select first max_trades rows
                 *		bid_price[] = T_BID_PRICE,
                 *		exec_name[] = T_EXEC_NAME,
                 *		is_cash[] = T_IS_CASH,
                 *		trade_list[] = T_ID,
                 *		trade_price[] = T_TRADE_PRICE
                 *	from
                 *		TRADE
                 *	where
                 *		T_CA_ID = acct_id and
                 *		T_DTS >= start_trade_dts and
                 *		T_DTS <= end_trade_dts
                 *	order by
                 *		T_DTS asc
                 */

                guard<index_scan_iter_impl<trade_t> > t_iter;
                {
                    index_scan_iter_impl<trade_t>* tmp_t_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-by-idx2 %ld %ld %ld \n", xct_id, ptlin._acct_id, ptlin._start_trade_dts, ptlin._end_trade_dts);
                    e = _ptrade_man->t_get_iter_by_index2(_pssm, tmp_t_iter, prtrade, lowrep, highrep, ptlin._acct_id, ptlin._start_trade_dts, ptlin._end_trade_dts);
                    t_iter = tmp_t_iter;
                    if (e.is_error()) { goto done; }
                }
                //already sorted in ascending order because of index

                bool eof;
                TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-next \n", xct_id);
                e = t_iter->next(_pssm, eof, *prtrade);
                if (e.is_error()) { goto done; }
                int j;
                for(j = 0; j < max_trades && !eof ; j++){
                    prtrade->get_value(7, bid_price[j]);
                    prtrade->get_value(9, exec_name[j], 50); //49
                    prtrade->get_value(4, is_cash[j]);
                    prtrade->get_value(0, trade_list[j]);
                    prtrade->get_value(10, trade_price[j]);

                    TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-next \n", xct_id);
                    e = t_iter->next(_pssm, eof, *prtrade);
                    if (e.is_error()) { goto done; }
                }
                int num_found = j;

                for(i = 0; i < num_found; i++){
                    /**
                     *	select
                     *		settlement_amount[i] = SE_AMT,
                     *		settlement_cash_due_date[i] = SE_CASH_DUE_DATE,
                     *		settlement_cash_type[i] = SE_CASH_TYPE
                     *	from
                     *		SETTLEMENT
                     *	where
                     *		SE_T_ID = trade_list[i]
                     */

                    TRACE( TRACE_TRX_FLOW, "App: %d TL:se-idx-probe (%ld) \n", xct_id,  trade_list[i]);
                    e = _psettlement_man->se_index_probe(_pssm, prsettlement, trade_list[i]);
                    if (e.is_error()) { goto done; }

                    prsettlement->get_value(3, settlement_amount[i]);
                    prsettlement->get_value(2, settlement_cash_due_date[i]);
                    prsettlement->get_value(1, settlement_cash_type[i], 41); //40

                    if(is_cash[i]){
                        /**
                         *	select
                         *		cash_transaction_amount[i] = CT_AMT,
                         *		cash_transaction_dts[i] = CT_DTS,
                         *		cash_transaction_name[i] = CT_NAME
                         *	from
                         *		CASH_TRANSACTION
                         *	where
                         *		CT_T_ID = trade_list[i]
                         *
                         */
                        TRACE( TRACE_TRX_FLOW, "App: %d TL:ct-idx-probe (%ld) \n", xct_id,  trade_list[i]);
                        e = _pcash_transaction_man->ct_index_probe(_pssm, prcashtrans, trade_list[i]);
                        if (e.is_error()) { goto done; }

                        prcashtrans->get_value(2, cash_transaction_amount[i]);
                        prcashtrans->get_value(1, cash_transaction_dts[i]);
                        prcashtrans->get_value(3, cash_transaction_name[i], 101); //100

                        /**
                         *
                         *	select first 3 rows
                         *		trade_history_dts[i][] = TH_DTS,
                         *		trade_history_status_id[i][] = TH_ST_ID
                         *	from
                         *		TRADE_HISTORY
                         *	where
                         *		TH_T_ID = trade_list[i]
                         *	order by
                         *		TH_DTS
                         */

                        guard<index_scan_iter_impl<trade_history_t> > th_iter;
                        {
                            index_scan_iter_impl<trade_history_t>* tmp_th_iter;
                            TRACE( TRACE_TRX_FLOW, "App: %d TL:th-get-iter-by-idx %ld \n", xct_id, trade_list[i]);
                            e = _ptrade_history_man->th_get_iter_by_index(_pssm, tmp_th_iter, prtradehist, lowrep, highrep, trade_list[i]);
                            if (e.is_error()) { goto done; }
                            th_iter = tmp_th_iter;
                        }
                        //ascending order
                        rep_row_t sortrep(_ptrade_man->ts());
                        sortrep.set(_ptrade_desc->maxsize());
                        asc_sort_buffer_t th_list(2);

                        th_list.setup(0, SQL_LONG);
                        th_list.setup(1, SQL_FIXCHAR, 4);

                        table_row_t rsb(&th_list);
                        asc_sort_man_impl th_sorter(&th_list, &sortrep);

                        TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                        e = th_iter->next(_pssm, eof, *prtradehist);
                        if (e.is_error()) { goto done; }
                        while (!eof) {
                            /* put the value into the sorted buffer */
                            myTime temp_dts;
                            char temp_stid[5]; //4

                            prtradehist->get_value(1, temp_dts);
                            prtradehist->get_value(2, temp_stid, 5);

                            rsb.set_value(0, temp_dts);
                            rsb.set_value(1, temp_stid);

                            th_sorter.add_tuple(rsb);

                            TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                            e = th_iter->next(_pssm, eof, *prtradehist);
                            if (e.is_error()) { goto done; }
                        }
                        assert (th_sorter.count());

                        asc_sort_iter_impl th_list_sort_iter(_pssm, &th_list, &th_sorter);
                        TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                        e = th_list_sort_iter.next(_pssm, eof, rsb);
                        if (e.is_error()) { goto done; }
                        for(int j = 0; j < 3 && !eof; j++) {
                            rsb.get_value(0, trade_history_dts[i][j]);
                            rsb.get_value(1, trade_history_status_id[i][j], 5);

                            TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                            e = th_list_sort_iter.next(_pssm, eof, rsb);
                            if (e.is_error()) { goto done; }
                        }
                    }
                }
                assert(num_found >= 0); //Harness control
            }
        //END FRAME2

        //BEGIN FRAME3
        if(ptlin._frame_to_execute == 3)
            {
                // Should return between 0 and max_trades rows.
                /**
                 *
                 *	select first max_trades rows
                 *		acct_id[] = T_CA_ID,
                 *		exec_name[] = T_EXEC_NAME,
                 *		is_cash[] = T_IS_CASH,
                 *		price[] = T_TRADE_PRICE,
                 *		quantity[] = T_QTY,
                 *		trade_dts[] = T_DTS,
                 *		trade_list[] = T_ID,
                 *		trade_type[] = T_TT_ID
                 *	from
                 *		TRADE
                 *	where
                 *		T_S_SYMB = symbol and
                 *		T_DTS >= start_trade_dts and
                 *		T_DTS <= end_trade_dts
                 *	order by
                 *		T_DTS asc
                 *
                 */
                //alignment resolved

                guard<index_scan_iter_impl<trade_t> > t_iter;
                {
                    index_scan_iter_impl<trade_t>* tmp_t_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TL:t-get-iter-by-idx3 %s %ld %ld \n", xct_id, ptlin._symbol,
                           ptlin._start_trade_dts, ptlin._end_trade_dts);
                    e = _ptrade_man->t_get_iter_by_index3(_pssm, tmp_t_iter, prtrade, lowrep, highrep, ptlin._symbol,
                                                          ptlin._start_trade_dts, ptlin._end_trade_dts);
                    if (e.is_error()) { goto done; }
                    t_iter = tmp_t_iter;
                }

                //already sorted in ascending order because of index
                bool eof;
                TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-next \n", xct_id);
                e = t_iter->next(_pssm, eof, *prtrade);
                if (e.is_error()) { goto done; }
                int j = 0;
                while(j < max_trades && !eof){
                    char t_s_symb[16];
                    prtrade->get_value(5, t_s_symb, 16);

                    if(strcmp(ptlin._symbol, t_s_symb) == 0){
                        prtrade->get_value(1, trade_dts[j]);
                        prtrade->get_value(8, acct_id[j]);
                        prtrade->get_value(9, exec_name[j], 50); //49
                        prtrade->get_value(4, is_cash[j]);
                        prtrade->get_value(10, trade_price[j]);
                        prtrade->get_value(6, quantity[j]);
                        prtrade->get_value(0, trade_list[j]);
                        prtrade->get_value(3, trade_type[j], 4);

                        j++;
                    }

                    TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-next \n", xct_id);
                    e = t_iter->next(_pssm, eof, *prtrade);
                    if (e.is_error()) { goto done; }
                }
                int num_found = j;

                for(int i = 0; i < num_found; i++){
                    /**
                     *	select
                     *		settlement_amount[i] = SE_AMT,
                     *		settlement_cash_due_date[i] = SE_CASH_DUE_DATE,
                     *		settlement_cash_type[i] = SE_CASH_TYPE
                     *	from
                     *		SETTLEMENT
                     *	where
                     *		SE_T_ID = trade_list[i]
                     */

                    TRACE( TRACE_TRX_FLOW, "TL: %d SE:se-idx-probe (%ld) \n", xct_id,  trade_list[i]);
                    e = _psettlement_man->se_index_probe(_pssm, prsettlement, trade_list[i]);
                    if (e.is_error()) { goto done; }

                    prsettlement->get_value(3, settlement_amount[i]);
                    prsettlement->get_value(2, settlement_cash_due_date[i]);
                    prsettlement->get_value(1, settlement_cash_type[i], 41); //40

                    if(is_cash[i]){
                        /**
                         *	select
                         *		cash_transaction_amount[i] = CT_AMT,
                         *		cash_transaction_dts[i] = CT_DTS,
                         *		cash_transaction_name[i] = CT_NAME
                         *	from
                         *		CASH_TRANSACTION
                         *	where
                         *		CT_T_ID = trade_list[i]
                         */

                        TRACE( TRACE_TRX_FLOW, "App: %d TL:ct-idx-probe (%ld) \n", xct_id,  trade_list[i]);
                        e = _pcash_transaction_man->ct_index_probe(_pssm, prcashtrans, trade_list[i]);
                        if (e.is_error()) { goto done; }

                        prcashtrans->get_value(2, cash_transaction_amount[i]);
                        prcashtrans->get_value(1, cash_transaction_dts[i]);
                        prcashtrans->get_value(3, cash_transaction_name[i], 101); //100

                        /**
                         *	select first 3 rows
                         *		trade_history_dts[i][] = TH_DTS,
                         *		trade_history_status_id[i][] = TH_ST_ID
                         *	from
                         *		TRADE_HISTORY
                         *	where
                         *		TH_T_ID = trade_list[i]
                         *	order by
                         *		TH_DTS
                         */
                        guard<index_scan_iter_impl<trade_history_t> > th_iter;
                        {
                            index_scan_iter_impl<trade_history_t>* tmp_th_iter;
                            TRACE( TRACE_TRX_FLOW, "App: %d TL:th-get-iter-by-idx %ld \n", xct_id, trade_list[i]);
                            e = _ptrade_history_man->th_get_iter_by_index(_pssm, tmp_th_iter, prtradehist, lowrep, highrep, trade_list[i]);
                            if (e.is_error()) { goto done; }
                            th_iter = tmp_th_iter;
                        }
                        //ascending order
                        rep_row_t sortrep(_ptrade_man->ts());
                        sortrep.set(_ptrade_desc->maxsize());
                        asc_sort_buffer_t th_list(2);

                        th_list.setup(0, SQL_LONG);
                        th_list.setup(1, SQL_FIXCHAR, 4);

                        table_row_t rsb(&th_list);
                        asc_sort_man_impl th_sorter(&th_list, &sortrep);
                        TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                        e = th_iter->next(_pssm, eof, *prtradehist);
                        if (e.is_error()) { goto done; }
                        while (!eof) {
                            /* put the value into the sorted buffer */
                            myTime temp_dts;
                            char temp_stid[5]; //4

                            prtradehist->get_value(1, temp_dts);
                            prtradehist->get_value(2, temp_stid, 5); //4

                            rsb.set_value(0, temp_dts);
                            rsb.set_value(1, temp_stid);

                            th_sorter.add_tuple(rsb);

                            TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                            e = th_iter->next(_pssm, eof, *prtradehist);
                            if (e.is_error()) { goto done; }
                        }
                        assert (th_sorter.count());

                        asc_sort_iter_impl th_list_sort_iter(_pssm, &th_list, &th_sorter);
                        TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                        e = th_list_sort_iter.next(_pssm, eof, rsb);
                        if (e.is_error()) { goto done; }
                        for(int j = 0; j < 3 && !eof; j++) {
                            rsb.get_value(0, trade_history_dts[i][j]);
                            rsb.get_value(1, trade_history_status_id[i][j], 5); //4

                            TRACE( TRACE_TRX_FLOW, "App: %d TL:th-iter-next \n", xct_id);
                            e = th_list_sort_iter.next(_pssm, eof, rsb);
                            if (e.is_error()) { goto done; }
                        }
                    }
                }
                assert(num_found >= 0); //Harness control
            }
        //END FRAME3

        TIdent  holding_history_id[20];
        TIdent  holding_history_trade_id[20];
        int	quantity_before[20];
        int	quantity_after[20];

        //BEGIN FRAME4
        if(ptlin._frame_to_execute == 4)
            {
                /**
                 *	select first 1 row
                 *		trade_id = T_ID
                 *	from
                 *		TRADE
                 *	where
                 *		T_CA_ID = acct_id and
                 *		T_DTS >= start_trade_dts
                 *	order by
                 *		T_DTS asc
                 */

                guard<index_scan_iter_impl<trade_t> > t_iter;
                {
                    index_scan_iter_impl<trade_t>* tmp_t_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TL:t-get-iter-by-idx2 (%ld) (%ld) (%ld) \n", xct_id, ptlin._acct_id, ptlin._start_trade_dts, MAX_DTS);
                    e = _ptrade_man->t_get_iter_by_index2(_pssm, tmp_t_iter, prtrade, lowrep, highrep, ptlin._acct_id, ptlin._start_trade_dts, MAX_DTS);
                    if (e.is_error()) { goto done; }
                    t_iter = tmp_t_iter;
                }
                //already sorted in ascending order because of index

                bool eof;
                e = t_iter->next(_pssm, eof, *prtrade);
                if (e.is_error()) { goto done; }

                TIdent trade_id;
                prtrade->get_value(1, trade_id);

                /**
                 *	select first 20 rows
                 *		holding_history_id[] = HH_H_T_ID,
                 *		holding_history_trade_id[] = HH_T_ID,
                 *		quantity_before[] = HH_BEFORE_QTY,
                 *		quantity_after[] = HH_AFTER_QTY
                 *	from
                 *		HOLDING_HISTORY
                 *	where
                 *		HH_H_T_ID in
                 *				(select
                 *					HH_H_T_ID
                 *				from
                 *					HOLDING_HISTORY
                 *				where
                 *					HH_T_ID = trade_id
                 * 				)
                 */
                guard< index_scan_iter_impl<holding_history_t> > hh_iter;
                {
                    index_scan_iter_impl<holding_history_t>* tmp_hh_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TL:hh-iter-by-idx (%ld)\n", xct_id, trade_id);
                    e = _pholding_history_man->hh_get_iter_by_index2(_pssm, tmp_hh_iter, prholdinghistory, lowrep, highrep, trade_id);
                    if (e.is_error()) { goto done; }
                    hh_iter = tmp_hh_iter;
                }

                TRACE( TRACE_TRX_FLOW, "App: %d TL:hh-iter-next \n", xct_id);
                e = hh_iter->next(_pssm, eof, *prholdinghistory);
                if (e.is_error()) { goto done; }
                for(int i = 0; i < 20 && !eof; i++) {
                    prholdinghistory->get_value(0, holding_history_id[i]);
                    prholdinghistory->get_value(1, holding_history_trade_id[i]);
                    prholdinghistory->get_value(2, quantity_before[i]);
                    prholdinghistory->get_value(3, quantity_after[i]);

                    TRACE( TRACE_TRX_FLOW, "App: %d TL:hh-iter-next \n", xct_id);
                    e = hh_iter->next(_pssm, eof, *prholdinghistory);
                    if (e.is_error()) { goto done; }
                }
            }
        //END FRAME4
    }
#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rtrade.print_tuple();
    rtradetype.print_tuple();
    rsettlement.print_tuple();
    rcashtrans.print_tuple();
    rtradehist.print_tuple();
    rholdinghistory.print_tuple();

#endif

 done:
    // return the tuples to the cache
    _ptrade_man->give_tuple(prtrade);
    _ptrade_type_man->give_tuple(prtradetype);
    _psettlement_man->give_tuple(prsettlement);
    _pcash_transaction_man->give_tuple(prcashtrans);
    _ptrade_history_man->give_tuple(prtradehist);
    _pholding_history_man->give_tuple(prholdinghistory);

    return (e);
}

w_rc_t ShoreTPCEEnv::xct_customer_position(const int xct_id, customer_position_input_t& pcpin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prcust = _pcustomer_man->get_tuple();	//280B
    assert (prcust);

    table_row_t* prcustacct = _pcustomer_account_man->get_tuple();
    assert (prcustacct);

    table_row_t* prholdsum = _pholding_summary_man->get_tuple();
    assert (prholdsum);

    table_row_t* prlasttrade = _plast_trade_man->get_tuple();
    assert (prlasttrade);

    table_row_t* prtradehist = _ptrade_history_man->get_tuple();
    assert (prtradehist); 

    table_row_t* prstatustype = _pstatus_type_man->get_tuple();
    assert (prstatustype);

    table_row_t* prtrade = _ptrade_man->get_tuple();	//149B
    assert (prtrade);

    w_rc_t e = RCOK;

    rep_row_t areprow(_pcustomer_man->ts());
    areprow.set(_pcustomer_desc->maxsize());

    prcust->_rep = &areprow;
    prcustacct->_rep = &areprow;
    prholdsum->_rep = &areprow;
    prlasttrade->_rep = &areprow;
    prtradehist->_rep = &areprow;
    prstatustype->_rep = &areprow;
    prtrade->_rep = &areprow;

    rep_row_t lowrep( _pcustomer_man->ts());
    rep_row_t highrep( _pcustomer_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_pcustomer_desc->maxsize());
    highrep.set(_pcustomer_desc->maxsize());

    {
        //pcpin.print();
        //BEGIN FRAME 1
        TIdent acct_id[10];
        int acct_len;
        {
            //printf("cust id: %ld ", pcpin._cust_id);
            TIdent cust_id = pcpin._cust_id;
            if(cust_id == 0){
                /**
                 *	select
                 *		cust_id = C_ID
                 *	from
                 *		CUSTOMER
                 *	where
                 *		C_TAX_ID = tax_id
                 */

                TRACE( TRACE_TRX_FLOW, "App: %d CP:c-get-iter-by-index2 (%s) \n", xct_id,  pcpin._tax_id);
                guard< index_scan_iter_impl<customer_t> > c_iter;
                {
                    index_scan_iter_impl<customer_t>* tmp_c_iter;
                    e = _pcustomer_man->c_get_iter_by_index2(_pssm, tmp_c_iter, prcust, lowrep, highrep, pcpin._tax_id);
                    if (e.is_error()) { goto done; }
                    c_iter = tmp_c_iter;
                }
                bool eof;
                e = c_iter->next(_pssm, eof, *prcust);
                if (e.is_error()) { goto done; }

                prcust->get_value(0, cust_id);
            }
            else{
                TRACE( TRACE_TRX_FLOW, "App: %d CP:c-idx-probe (%ld) \n", xct_id,  cust_id);
                e =  _pcustomer_man->c_index_probe(_pssm, prcust, cust_id);
                if (e.is_error()) { goto done; }			  
            }

            /**
             *	select
             *		c_st_id = C_ST_ID, //2
             *		c_l_name = C_L_NAME,
             *		c_f_name = C_F_NAME,
             *		c_m_name = C_M_NAME,
             *		c_gndr = C_GNDR,
             *		c_tier = C_TIER,
             *		c_dob = C_DOB,
             *		c_ad_id = C_AD_ID,
             *		c_ctry_1 = C_CTRY_1,
             *		c_area_1 = C_AREA_1,
             *		c_local_1 = C_LOCAL_1,
             *		c_ext_1 = C_EXT_1,
             *		c_ctry_2 = C_CTRY_2,
             *		c_area_2 = C_AREA_2,
             *		c_local_2 = C_LOCAL_2,
             *		c_ext_2 = C_EXT_2,
             *		c_ctry_3 = C_CTRY_3,
             *		c_area_3 = C_AREA_3,
             *		c_local_3 = C_LOCAL_3,
             *		c_ext_3 = C_EXT_3,
             *		c_email_1 = C_EMAIL_1,
             *		c_email_2 = C_EMAIL_2
             *	from
             *		CUSTOMER
             *	where
             *		C_ID = cust_id
             */

            char c_st_id[5]; //4
            prcust->get_value(2, c_st_id, 5);
            char c_l_name[26]; //25
            prcust->get_value(3, c_l_name, 26);
            char c_f_name[21]; //20
            prcust->get_value(4, c_f_name, 21);
            char c_m_name[2]; //1
            prcust->get_value(5, c_m_name, 2);
            char c_gndr[2]; //1
            prcust->get_value(6, c_gndr, 2);
            short c_tier;
            prcust->get_value(7, c_tier);
            myTime c_dob;
            prcust->get_value(8, c_dob);
            TIdent c_ad_id;
            prcust->get_value(9, c_ad_id);
            char c_ctry_1[4]; //3
            prcust->get_value(10, c_ctry_1, 4);
            char c_area_1[4]; //3
            prcust->get_value(11, c_area_1, 4);
            char c_local_1[11]; //10
            prcust->get_value(12, c_local_1, 11);
            char c_ext_1[6]; //5
            prcust->get_value(13, c_ext_1, 6);
            char c_ctry_2[4]; //3
            prcust->get_value(14, c_ctry_2, 4);
            char c_area_2[4]; //3
            prcust->get_value(15, c_area_2, 4);
            char c_local_2[11]; //10
            prcust->get_value(16, c_local_2, 11);
            char c_ext_2[6]; //5
            prcust->get_value(17, c_ext_2, 6);
            char c_ctry_3[4]; //3
            prcust->get_value(18, c_ctry_3, 4);
            char c_area_3[4]; //3
            prcust->get_value(19, c_area_3, 4);
            char c_local_3[11]; //10
            prcust->get_value(20, c_local_3, 11);
            char c_ext_3[6]; //5
            prcust->get_value(21, c_ext_3, 6);
            char c_email_1[51]; //50
            prcust->get_value(22, c_email_1, 51);
            char c_email_2[51]; //50
            prcust->get_value(23, c_email_2, 51);

            /**
             *	select first max_acct_len rows
             *		acct_id[ ] = CA_ID,
             *		cash_bal[ ] = CA_BAL,
             *		assets_total[ ] = ifnull((sum(HS_QTY * LT_PRICE)),0)
             *	from
             *		CUSTOMER_ACCOUNT left outer join
             *		HOLDING_SUMMARY on HS_CA_ID = CA_ID,
             *		LAST_TRADE
             *	where
             *		CA_C_ID = cust_id and
             *		LT_S_SYMB = HS_S_SYMB
             *	group by
             *		CA_ID, CA_BAL
             *	order by
             *		3 asc
             */

            guard< index_scan_iter_impl<customer_account_t> > ca_iter;
            {
                index_scan_iter_impl<customer_account_t>* tmp_ca_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d CP:ca-get-iter-by-idx2 (%ld) \n", xct_id,  cust_id);
                e = _pcustomer_account_man->ca_get_iter_by_index2(_pssm, tmp_ca_iter, prcustacct, lowrep, highrep, cust_id);
                if (e.is_error()) { goto done; }
                ca_iter = tmp_ca_iter;
            }

            //ascending order
            rep_row_t sortrep(_pcustomer_man->ts());
            sortrep.set(_pcustomer_desc->maxsize());
            asc_sort_buffer_t ca_list(3);

            ca_list.setup(0, SQL_FLOAT);
            ca_list.setup(1, SQL_FLOAT);
            ca_list.setup(2, SQL_LONG);

            table_row_t rsb(&ca_list);
            asc_sort_man_impl ca_sorter(&ca_list, &sortrep);

            int i = 0;
            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d CP:ca-iter-next \n", xct_id);
            e = ca_iter->next(_pssm, eof, *prcustacct);
            if (e.is_error()) { goto done; }
            while(!eof){
                TIdent temp_id;
                double temp_balance = 0, temp_assets = 0;

                prcustacct->get_value(0, temp_id);
                prcustacct->get_value(5, temp_balance);

                guard< index_scan_iter_impl<holding_summary_t> > hs_iter;
                {
                    index_scan_iter_impl<holding_summary_t>* tmp_hs_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d CP:hs-get-iter-by-idx (%ld) \n", xct_id,  temp_id);
                    e = _pholding_summary_man->hs_get_iter_by_index(_pssm, tmp_hs_iter, prholdsum, lowrep, highrep, temp_id);
                    if (e.is_error()) { goto done; }
                    hs_iter = tmp_hs_iter;
                }

                TRACE( TRACE_TRX_FLOW, "App: %d CP:hs-iter-next  \n", xct_id);
                e = hs_iter->next(_pssm, eof, *prholdsum);
                if (e.is_error()) {  goto done; }
                while(!eof){
                    char symbol[16]; //15
                    prholdsum->get_value(1, symbol, 16);
                    int qty;
                    prholdsum->get_value(2, qty);

                    TRACE( TRACE_TRX_FLOW, "App: %d CP:lt-idx-probe (%s) \n", xct_id,  symbol);
                    e =  _plast_trade_man->lt_index_probe(_pssm, prlasttrade, symbol);
                    if (e.is_error()) { goto done; }

                    double lt_price = 0;
                    prlasttrade->get_value(2, lt_price);
                    temp_assets += (lt_price * qty);

                    TRACE( TRACE_TRX_FLOW, "App: %d CP:hs-iter-next  \n", xct_id);
                    e = hs_iter->next(_pssm, eof, *prholdsum);
                    if (e.is_error()) { goto done; }
                }

                rsb.set_value(0, temp_assets);
                rsb.set_value(1, temp_balance);
                rsb.set_value(2, temp_id);

                TRACE( TRACE_TRX_FLOW, "App: %d CP:rsb add tuple  \n", xct_id);
                ca_sorter.add_tuple(rsb);

                TRACE( TRACE_TRX_FLOW, "App: %d CP:ca-iter-next \n", xct_id);
                e = ca_iter->next(_pssm, eof, *prcustacct);
                if (e.is_error()) { goto done; }
                i++;
            }
            assert (ca_sorter.count());
            acct_len = i;

            double cash_bal[10];
            double assets_total[10];

            asc_sort_iter_impl ca_list_sort_iter(_pssm, &ca_list, &ca_sorter);
            TRACE( TRACE_TRX_FLOW, "App: %d CP:ca-sorter-iter-next \n", xct_id);
            e = ca_list_sort_iter.next(_pssm, eof, rsb);
            if (e.is_error()) { goto done; }
            for(int j = 0; j < 10 && !eof; j++) {
                rsb.get_value(2, acct_id[j]);
                rsb.get_value(1, cash_bal[j]);
                rsb.get_value(0, assets_total[j]);

                TRACE( TRACE_TRX_FLOW, "App: %d CP:ca-sorter-iter-next \n", xct_id);
                e = ca_list_sort_iter.next(_pssm, eof, rsb);
                if (e.is_error()) { goto done; }
            }
        }
        //END OF FRAME 1
        assert(acct_len >= 1 && acct_len <= max_acct_len); //Harness control

        TIdent acctId = acct_id[pcpin._acct_id_idx];
        int hist_len;
        //BEGIN FRAME 2
        if(pcpin._get_history){
            /**
             *	select first 30 rows
             *		trade_id[] = T_ID,
             *		symbol[] = T_S_SYMB,
             *		qty[] = T_QTY,
             *		trade_status[] = ST_NAME,
             *		hist_dts[] = TH_DTS
             *	from
             *		(select first 10 rows
             *			T_ID as ID
             *		from
             *			TRADE
             *		where
             *			T_CA_ID = acct_id
             *		order by
             * 			T_DTS desc) as T,
             *		TRADE,
             *		TRADE_HISTORY,
             *		STATUS_TYPE
             *	where
             *		T_ID = ID and
             *		TH_T_ID = T_ID and
             *		ST_ID = TH_ST_ID
             *	order by
             *		TH_DTS desc
             */

            TIdent trade_id[30];
            char symbol[30][16]; //15
            int qty[30];
            char trade_status[30][11]; //10
            myTime hist_dts[30];

            TIdent id_list[10];
            {
                guard<index_scan_iter_impl<trade_t> > t_iter;
                {
                    index_scan_iter_impl<trade_t>* tmp_t_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d CP:t-iter-by-idx2 %ld \n", xct_id, acctId);
                    e = _ptrade_man->t_get_iter_by_index2(_pssm, tmp_t_iter, prtrade, lowrep, highrep, acctId, 0, MAX_DTS);
                    t_iter = tmp_t_iter;
                    if (e.is_error()) { goto done; }
                }

                //descending order
                rep_row_t sortrep(_pcustomer_man->ts());
                sortrep.set(_pcustomer_desc->maxsize());

                sort_buffer_t t_list(1);
                t_list.setup(0, SQL_LONG);

                sort_man_impl t_sorter(&t_list, &sortrep);
                table_row_t rsb(&t_list);

                bool eof;
                TRACE( TRACE_TRX_FLOW, "App: %d CP:t-iter-next \n", xct_id);
                e = t_iter->next(_pssm, eof, *prtrade);
                if (e.is_error()) { goto done; }
                while(!eof){
                    /* put the value into the sorted buffer */
                    TIdent temp_id;
                    prtrade->get_value(0, temp_id);

                    rsb.set_value(0, temp_id);
                    t_sorter.add_tuple(rsb);

                    TRACE( TRACE_TRX_FLOW, "App: %d CP:t-iter-next \n", xct_id);
                    e = t_iter->next(_pssm, eof, *prtrade);
                    if (e.is_error()) { goto done; }
                }
                assert (t_sorter.count());

                sort_iter_impl t_list_sort_iter(_pssm, &t_list, &t_sorter);
                TRACE( TRACE_TRX_FLOW, "App: %d CP:t-sorter-iter-next \n", xct_id);
                e = t_list_sort_iter.next(_pssm, eof, rsb);
                if (e.is_error()) { goto done; }
                for (int i = 0; i < 10 && !eof; i++) {
                    TIdent id;
                    rsb.get_value(0, id);

                    id_list[i] = id;

                    TRACE( TRACE_TRX_FLOW, "App: %d CP:t-sorter-iter-next \n", xct_id);
                    e = t_list_sort_iter.next(_pssm, eof, rsb);
                    if (e.is_error()) { goto done; }
                }
            }

            rep_row_t sortrep(_pcustomer_man->ts());
            sortrep.set(_pcustomer_desc->maxsize());

            sort_buffer_t t_list(5);
            t_list.setup(0, SQL_LONG); //th_dts
            t_list.setup(1, SQL_LONG);
            t_list.setup(2, SQL_FIXCHAR, 15);
            t_list.setup(3, SQL_INT);
            t_list.setup(4, SQL_FIXCHAR, 10);

            sort_man_impl t_sorter(&t_list, &sortrep);
            table_row_t rsb(&t_list);

            for (int i = 0; i < 10; i++) {
                TRACE( TRACE_TRX_FLOW, "App: %d CP:t-idx-probe (%ld) \n", xct_id,  id_list[i]);
                e =  _ptrade_man->t_index_probe(_pssm, prtrade, id_list[i]);
                if (e.is_error()) { goto done; }

                guard<index_scan_iter_impl<trade_history_t> > th_iter;
                {
                    index_scan_iter_impl<trade_history_t>* tmp_th_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d CP:th-iter-by-idx %ld \n", xct_id, id_list[i]);
                    e = _ptrade_history_man->th_get_iter_by_index(_pssm, tmp_th_iter, prtradehist, lowrep, highrep, id_list[i]);
                    if (e.is_error()) { goto done; }
                    th_iter = tmp_th_iter;
                }

                bool eof;
                e = th_iter->next(_pssm, eof, *prtradehist);
                if (e.is_error()) { goto done; }
                while(!eof){
                    char th_st_id[6]; //5
                    prtradehist->get_value(2, th_st_id, 6);

                    TRACE( TRACE_TRX_FLOW, "App: %d CP:st-idx-probe (%s) \n", xct_id,  th_st_id);
                    e =  _pstatus_type_man->st_index_probe(_pssm, prstatustype, th_st_id);
                    if (e.is_error()) { goto done; }

                    myTime th_dts;
                    prtradehist->get_value(1, th_dts);
                    rsb.set_value(0, th_dts);

                    rsb.set_value(1, id_list[i]);

                    char t_s_symb[16];
                    prtrade->get_value(5, t_s_symb, 16);
                    rsb.set_value(2, t_s_symb);

                    int t_qty;
                    prtrade->get_value(6, t_qty);
                    rsb.set_value(3, t_qty);

                    char st_name[11];
                    prstatustype->get_value(1, st_name, 11);
                    rsb.set_value(4, st_name);

                    t_sorter.add_tuple(rsb);

                    e = th_iter->next(_pssm, eof, *prtradehist);
                    if (e.is_error()) { goto done; }
                }
            }
            sort_iter_impl t_list_sort_iter(_pssm, &t_list, &t_sorter);

            bool eof;
            e = t_list_sort_iter.next(_pssm, eof, rsb);
            if (e.is_error()) { goto done; }
            int i;
            for(i = 0; i < 30 && !eof; i++){
                rsb.get_value(0, hist_dts[i]);
                rsb.get_value(1, trade_id[i]);
                rsb.get_value(2, symbol[i], 16);
                rsb.get_value(3, qty[i]);
                rsb.get_value(4, trade_status[i], 11);

                e = t_list_sort_iter.next(_pssm, eof, rsb);
                if (e.is_error()) { goto done; }
            }
            hist_len = i;			
            assert(hist_len >= 10 && hist_len <= max_hist_len); //Harness control
        }
        //END OF FRAME 2
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rcust.print_tuple();
    rcustacct.print_tuple();
    rholdsum.print_tuple();
    rlasttrade.print_tuple();
    rtradehist.print_tuple();
    rstatustype.print_tuple();
    rtrade.print_tuple();

#endif

 done:
    // return the tuples to the cache
    _pcustomer_man->give_tuple(prcust);
    _pcustomer_account_man->give_tuple(prcustacct);
    _pholding_summary_man->give_tuple(prholdsum);
    _plast_trade_man->give_tuple(prlasttrade);
    _ptrade_history_man->give_tuple(prtradehist);
    _pstatus_type_man->give_tuple(prstatustype);
    _ptrade_man->give_tuple(prtrade);
    return (e);
}


w_rc_t ShoreTPCEEnv::xct_market_feed(const int xct_id, market_feed_input_t& pmfin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prlasttrade = _plast_trade_man->get_tuple(); //48
    assert (prlasttrade);

    table_row_t* prtradereq = _ptrade_request_man->get_tuple();
    assert (prtradereq);

    table_row_t* prtrade = _ptrade_man->get_tuple(); //149B
    assert (prtrade);

    table_row_t* prtradehist = _ptrade_history_man->get_tuple();
    assert (prtradehist);

    w_rc_t e = RCOK;
    rep_row_t areprow(_ptrade_man->ts());
    areprow.set(_ptrade_desc->maxsize());

    prlasttrade->_rep = &areprow;
    prtradereq->_rep = &areprow;
    prtrade->_rep = &areprow;
    prtradehist->_rep = &areprow;

    rep_row_t lowrep( _ptrade_man->ts());
    rep_row_t highrep( _ptrade_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_ptrade_desc->maxsize());
    highrep.set(_ptrade_desc->maxsize());
    {
        myTime 		now_dts;
        double 		req_price_quote;
        TIdent		req_trade_id;
        int		req_trade_qty;
        char		req_trade_type[4]; //3
        int 		rows_updated;

        now_dts = time(NULL);
        rows_updated = 0;

        for(int i = 0; i < max_feed_len; i++){ 
            /**
               update
               LAST_TRADE
               set
               LT_PRICE = price_quote[i],
               LT_VOL = LT_VOL + trade_qty[i],
               LT_DTS = now_dts
               where
               LT_S_SYMB = symbol[i]
            */

            TRACE( TRACE_TRX_FLOW, "App: %d MF:lt-update (%s) (%d) (%d) (%ld) \n", xct_id, pmfin._symbol[i], pmfin._price_quote[i], pmfin._trade_qty[i], now_dts);
            e = _plast_trade_man->lt_update_by_index(_pssm, prlasttrade, pmfin._symbol[i], pmfin._price_quote[i], pmfin._trade_qty[i], now_dts);
            if (e.is_error()) {  goto done; }

            rows_updated++; //there is only one row per symbol

            /**
               select
               TR_T_ID,
               TR_BID_PRICE,
               TR_TT_ID,
               TR_QTY
               from
               TRADE_REQUEST
               where
               TR_S_SYMB = symbol[i] and (
               (TR_TT_ID = type_stop_loss and
               TR_BID_PRICE >= price_quote[i]) or
               (TR_TT_ID = type_limit_sell and
               TR_BID_PRICE <= price_quote[i]) or
               (TR_TT_ID = type_limit_buy and
               TR_BID_PRICE >= price_quote[i])
               )
            */
			
            trade_request_man_impl::table_iter* tr_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-get-table-iter \n", xct_id);
            e = _ptrade_request_man->tr_get_table_iter(_pssm, tr_iter, prtradereq);
            if (e.is_error()) {  goto done; }

            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-iter-next \n", xct_id);
            e = tr_iter->next(_pssm, eof, *prtradereq);
            if (e.is_error()) {  goto done; }
            while(!eof){
                char tr_s_symb[16], tr_tt_id[4]; //15,3
                double tr_bid_price;

                prtradereq->get_value(2, tr_s_symb, 16);
                prtradereq->get_value(1, tr_tt_id, 4);
                prtradereq->get_value(4, tr_bid_price);

                if(strcmp(tr_s_symb, pmfin._symbol[i]) == 0 &&
                   (
                    (strcmp(tr_tt_id, pmfin._type_stop_loss) == 0 && (tr_bid_price >= pmfin._price_quote[i])) ||
                    (strcmp(tr_tt_id, pmfin._type_limit_sell) == 0 && (tr_bid_price <= pmfin._price_quote[i])) ||
                    (strcmp(tr_tt_id, pmfin._type_limit_buy)== 0 && (tr_bid_price >= pmfin._price_quote[i]))
                    ))
                    {
                        prtradereq->get_value(0, req_trade_id);
                        prtradereq->get_value(4, req_price_quote);
                        prtradereq->get_value(1, req_trade_type, 4);
                        prtradereq->get_value(3, req_trade_qty);

                        /**
                           update
                           TRADE
                           set
                           T_DTS   = now_dts,
                           T_ST_ID = status_submitted
                           where
                           T_ID = req_trade_id
                        */

                        TRACE( TRACE_TRX_FLOW, "App: %d MF:t-update (%ld) (%ld) (%s) \n", xct_id, req_trade_id, now_dts, pmfin._status_submitted);
                        e = _ptrade_man->t_update_dts_stdid_by_index(_pssm, prtrade, req_trade_id, now_dts, pmfin._status_submitted);
                        if (e.is_error()) {  goto done; }

                        /**
                           delete
                           TRADE_REQUEST
                           where
                           current of request_list
                        */
                        TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-delete- \n", xct_id);
                        e = _ptrade_request_man->delete_tuple(_pssm, prtradereq);
                        if (e.is_error()) {  goto done; }
                        prtradereq = _ptrade_request_man->get_tuple();
                        assert (prtradereq);
                        prtradereq->_rep = &areprow;

                        /**
                           insert into
                           TRADE_HISTORY
                           values (
                           TH_T_ID = req_trade_id,
                           TH_DTS = now_dts,
                           TH_ST_ID = status_submitted)
                        */
                        prtradehist->set_value(0, req_trade_id);
                        prtradehist->set_value(1, now_dts);
                        prtradehist->set_value(2, pmfin._status_submitted);

                        TRACE( TRACE_TRX_FLOW, "App: %d MF:th-add-tuple (%ld) (%ld) (%s) \n", xct_id, req_trade_id, now_dts, pmfin._status_submitted);
                        e = _ptrade_history_man->add_tuple(_pssm, prtradehist);
                        if (e.is_error()) {  goto done; }
                    }
                TRACE( TRACE_TRX_FLOW, "App: %d MF:tr-iter-next \n", xct_id);
                e = tr_iter->next(_pssm, eof, *prtradereq);
                if (e.is_error()) {  goto done; }
            }
        }
        assert(rows_updated == max_feed_len); //Harness Control
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rlasttrade.print_tuple();
    rtradereq.print_tuple();
    rtrade.print_tuple();
    rtradehist.print_tuple();

#endif

 done:
    // return the tuples to the cache
    _plast_trade_man->give_tuple(prlasttrade);
    _ptrade_request_man->give_tuple(prtradereq);
    _ptrade_man->give_tuple(prtrade);
    _ptrade_history_man->give_tuple(prtradehist);

    return (e);
}


w_rc_t ShoreTPCEEnv::xct_trade_cleanup(const int xct_id, trade_cleanup_input_t& ptcin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prtrade = _ptrade_man->get_tuple();
    assert (prtrade);

    table_row_t* prtradehist = _ptrade_history_man->get_tuple();
    assert (prtradehist);

    table_row_t* prtradereq = _ptrade_request_man->get_tuple();
    assert (prtradereq);

    w_rc_t e = RCOK;
    rep_row_t areprow(_ptrade_man->ts());
    areprow.set(_ptrade_desc->maxsize());

    prtradereq->_rep = &areprow;
    prtrade->_rep = &areprow;
    prtradehist->_rep = &areprow;

    rep_row_t lowrep( _ptrade_man->ts());
    rep_row_t highrep( _ptrade_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_ptrade_desc->maxsize());
    highrep.set(_ptrade_desc->maxsize());

    {
        TIdent	t_id;
        TIdent 	tr_t_id;
        myTime 	now_dts;

        /**
           select
           TR_T_ID
           from
           TRADE_REQUEST
           order by
           TR_T_ID
        */

        guard< index_scan_iter_impl<trade_request_t> > tr_iter;
        {
            index_scan_iter_impl<trade_request_t>* tmp_tr_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d TC:tr-get-iter-by-idx \n", xct_id);
            e = _ptrade_request_man->tr_get_iter_by_index(_pssm, tmp_tr_iter, prtradereq, lowrep, highrep);
            if (e.is_error()) { goto done; }
            tr_iter = tmp_tr_iter;
        }

        bool eof;
        TRACE( TRACE_TRX_FLOW, "App: %d TC:tr-iter-next \n", xct_id);
        e = tr_iter->next(_pssm, eof, *prtradereq);
        if (e.is_error()) { goto done; }
        while(!eof){
            TIdent tr_t_id;
            prtradereq->get_value(0, tr_t_id);

            now_dts = time(NULL);

            /**
               insert into
               TRADE_HISTORY (
               TH_T_ID, TH_DTS, TH_ST_ID
               )
               values (
               tr_t_id,         // TH_T_ID
               now_dts,         // TH_DTS
               st_submitted_id  // TH_ST_ID
               )
            */

            prtradehist->set_value(0, tr_t_id);
            prtradehist->set_value(1, now_dts);
            prtradehist->set_value(2, ptcin._st_submitted_id);

            TRACE( TRACE_TRX_FLOW, "App: %d TC:th-add-tuple (%ld) (%ld) (%s) \n", xct_id, tr_t_id, now_dts, ptcin._st_submitted_id);
            e = _ptrade_history_man->add_tuple(_pssm, prtradehist);
            if (e.is_error()) { goto done; }

            /**
               update
               TRADE
               set
               T_ST_ID = st_canceled_id,
               T_DTS = now_dts
               where
               T_ID = tr_t_id
            */

            TRACE( TRACE_TRX_FLOW, "App: %d TC:t-update (%ld) (%ld) (%s) \n", xct_id, tr_t_id, now_dts, ptcin._st_canceled_id);
            e = _ptrade_man->t_update_dts_stdid_by_index(_pssm, prtrade, tr_t_id, now_dts, ptcin._st_canceled_id);
            if (e.is_error()) { goto done; }

            /**
               insert into
               TRADE_HISTORY (
               TH_T_ID, TH_DTS, TH_ST_ID
               )
               values (
               tr_t_id,        // TH_T_ID
               now_dts,        // TH_DTS
               st_canceled_id  // TH_ST_ID
               )
            */

            prtradehist->set_value(0, tr_t_id);
            prtradehist->set_value(1, now_dts);
            prtradehist->set_value(2, ptcin._st_canceled_id);

            TRACE( TRACE_TRX_FLOW, "App: %d TC:th-add-tuple (%ld) (%ld) (%s) \n", xct_id, tr_t_id, now_dts, ptcin._st_canceled_id);
            e = _ptrade_history_man->add_tuple(_pssm, prtradehist);
            if (e.is_error()) { goto done; }

            TRACE( TRACE_TRX_FLOW, "App: %d TC:tr-iter-next \n", xct_id);
            e = tr_iter->next(_pssm, eof, *prtradereq);
            if (e.is_error()) { goto done; }
        }

        /**
           delete
           from
           TRADE_REQUEST
        */

        index_scan_iter_impl<trade_request_t>* tmp_tr_iter;
        {
            TRACE( TRACE_TRX_FLOW, "App: %d TC:tr-get-iter-by-idx \n", xct_id);
            e = _ptrade_request_man->tr_get_iter_by_index(_pssm, tmp_tr_iter, prtradereq, lowrep, highrep);
            if (e.is_error()) { goto done; }
            tr_iter = tmp_tr_iter;
        }
        TRACE( TRACE_TRX_FLOW, "App: %d TC:tr-iter-next \n", xct_id);
        e = tr_iter->next(_pssm, eof, *prtradereq);
        if (e.is_error()) { goto done; }
        while(!eof){
            TRACE( TRACE_TRX_FLOW, "App: %d TC:tr-delete- \n", xct_id);
            e = _ptrade_request_man->delete_tuple(_pssm, prtradereq);
            if (e.is_error()) { goto done; }

            TRACE( TRACE_TRX_FLOW, "App: %d TC:tr-iter-next \n", xct_id);
            e = tr_iter->next(_pssm, eof, *prtradereq);
            if (e.is_error()) { goto done; }
        }

        /**
           select
           T_ID
           from
           TRADE
           where
           T_ID >= trade_id and
           T_ST_ID = st_submitted_id
        */
        guard<index_scan_iter_impl<trade_t> > t_iter;
        {
            index_scan_iter_impl<trade_t>* tmp_t_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d TC:t-iter-by-idx \n", xct_id);
            e = _ptrade_man->t_get_iter_by_index(_pssm, tmp_t_iter, prtrade, lowrep, highrep, ptcin._trade_id);
            if (e.is_error()) { goto done; }
            t_iter = tmp_t_iter;
        }

        TRACE( TRACE_TRX_FLOW, "App: %d TC:t-iter-next \n", xct_id);
        e = t_iter->next(_pssm, eof, *prtrade);
        if (e.is_error()) { goto done; }
        while(!eof){
            char t_st_id[5]; //4
            prtrade->get_value(2, t_st_id, 5);

            if(strcmp(t_st_id, ptcin._st_submitted_id) == 0){
                now_dts = time(NULL);
                /** 
                    update
                    TRADE
                    set
                    T_ST_ID = st_canceled_id
                    T_DTS = now_dts
                    where
                    T_ID = t_id
                */

                TRACE( TRACE_TRX_FLOW, "App: %d TC:t-update (%ld) (%ld) (%s) \n", xct_id, t_id, now_dts, ptcin._st_canceled_id);
                e = _ptrade_man->t_update_dts_stdid_by_index(_pssm, prtrade, t_id, now_dts, ptcin._st_canceled_id);
                if (e.is_error()) { goto done; }

                /**
                   insert into
                   TRADE_HISTORY (
                   TH_T_ID, TH_DTS, TH_ST_ID
                   )
                   values (
                   t_id,           // TH_T_ID
                   now_dts,        // TH_DTS
                   st_canceled_id  // TH_ST_ID
                   )
                */

                prtradehist->set_value(0, t_id);
                prtradehist->set_value(1, now_dts);
                prtradehist->set_value(2, ptcin._st_canceled_id);

                TRACE( TRACE_TRX_FLOW, "App: %d TC:th-add-tuple (%ld) (%ld) (%s) \n", xct_id, t_id, now_dts, ptcin._st_canceled_id);
                e = _ptrade_history_man->add_tuple(_pssm, prtradehist);
                if (e.is_error()) { goto done; }
            }
            TRACE( TRACE_TRX_FLOW, "App: %d TC:t-iter-next \n", xct_id);
            e = t_iter->next(_pssm, eof, *prtrade);
            if (e.is_error()) { goto done; }			
			
        }
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rtradereq.print_tuple();
    rtrade.print_tuple();
    rtradehist.print_tuple();

#endif

 done:
    // return the tuples to the cache
    _ptrade_request_man->give_tuple(prtradereq);
    _ptrade_man->give_tuple(prtrade);
    _ptrade_history_man->give_tuple(prtradehist);

    return (e);
}


w_rc_t ShoreTPCEEnv::xct_market_watch(const int xct_id, market_watch_input_t& pmwin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);
	
    //delete address
    table_row_t* praddress = _paddress_man->get_tuple(); //296B
    assert (praddress);

    table_row_t* prcompany = _pcompany_man->get_tuple(); //296B
    assert (prcompany);

    table_row_t* prdailymarket = _pdaily_market_man->get_tuple();
    assert (prdailymarket);

    table_row_t* prholdsumm = _pholding_summary_man->get_tuple();
    assert (prholdsumm);

    table_row_t* prindustry = _pindustry_man->get_tuple();
    assert (prindustry);

    table_row_t* prlasttrade = _plast_trade_man->get_tuple();
    assert (prlasttrade);

    table_row_t* prsecurity = _psecurity_man->get_tuple();
    assert (prsecurity);

    table_row_t* prwatchitem = _pwatch_item_man->get_tuple();
    assert (prwatchitem);

    table_row_t* prwatchlist = _pwatch_list_man->get_tuple();
    assert (prwatchlist);

    w_rc_t e = RCOK;
    rep_row_t areprow(_pcompany_man->ts());
    areprow.set(_pcompany_desc->maxsize());

    praddress->_rep = &areprow;
    prcompany->_rep = &areprow;
    prdailymarket->_rep = &areprow;
    prholdsumm->_rep = &areprow;
    prindustry->_rep = &areprow;
    prlasttrade->_rep = &areprow;
    prsecurity->_rep = &areprow;
    prwatchitem->_rep = &areprow;
    prwatchlist->_rep = &areprow;

    rep_row_t lowrep( _pcompany_man->ts());
    rep_row_t highrep( _pcompany_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_pcompany_desc->maxsize());
    highrep.set(_pcompany_desc->maxsize());

#ifdef TEST_EGEN
	
    //try
	
    {

        int cnt = 0;
        address_man_impl::table_iter* lt_iter;
	//	TRACE( TRACE_TRX_FLOW, "App: %d MW:lt-get-table-iter \n", xct_id);
        e = _paddress_man->ad_get_table_iter(_pssm, lt_iter);
        if (e.is_error()) { goto done; }

        bool eof;
        //TRACE( TRACE_TRX_FLOW, "App: %d MW:lt-iter-next \n", xct_id);
        e = lt_iter->next(_pssm, eof, *praddress);
        if (e.is_error()) { goto done; }
        while(!eof){
            char temp[81];
            praddress->get_value(1, temp, 81);
            //	  printf("Address	%s \n", temp);
		  
            e = lt_iter->next(_pssm, eof, *praddress);
            if (e.is_error()) { goto done; }
		  
            cnt++;
        }
        printf("Address TABLE COUNT: 	%d \n\n\n\n\n\n\n\n\n\n\n", cnt);
    }
		
	
		
    {
        int cnt = 0;
        security_man_impl::table_iter* s_iter;
	//	TRACE( TRACE_TRX_FLOW, "App: %d MW:s-get-table-iter \n", xct_id);
        e = _psecurity_man->s_get_table_iter(_pssm, s_iter);
        if (e.is_error()) { goto done; }

        bool eof;
	//	TRACE( TRACE_TRX_FLOW, "App: %d MF:s-iter-next \n", xct_id);
        e = s_iter->next(_pssm, eof, *prsecurity);
        if (e.is_error()) { goto done; }
        while(!eof){
            char temp[16];
            prsecurity->get_value(0, temp, 16);
            //  printf("S Symbol	%s \n", temp);
		  
            //TRACE( TRACE_TRX_FLOW, "App: %d MF:lt-iter-next \n", xct_id);
            e = s_iter->next(_pssm, eof, *prsecurity);
            if (e.is_error()) { goto done; }
		  
            cnt++;
        }
        printf("SEC TABLE COUNT: 	%d \n\n\n\n\n\n\n\n\n\n\n", cnt);
    }
	    
    {
        int cnt = 0;
        last_trade_man_impl::table_iter* lt_iter;
	//	TRACE( TRACE_TRX_FLOW, "App: %d MW:lt-get-table-iter \n", xct_id);
        e = _plast_trade_man->lt_get_table_iter(_pssm, lt_iter);
        if (e.is_error()) { goto done; }

        bool eof;
        //TRACE( TRACE_TRX_FLOW, "App: %d MW:lt-iter-next \n", xct_id);
        e = lt_iter->next(_pssm, eof, *prlasttrade);
        if (e.is_error()) { goto done; }
        while(!eof){
            char temp[16];
            prlasttrade->get_value(0, temp, 16);
            //	  printf("LT Symbol	%s \n", temp);
		  
            e = lt_iter->next(_pssm, eof, *prlasttrade);
            if (e.is_error()) { goto done; }
		  
            cnt++;
        }
        printf("LT TABLE COUNT: 	%d \n\n\n\n\n\n\n\n\n\n\n", cnt);
    }
    {
		
        security_man_impl::table_iter* s_iter;
        TRACE( TRACE_TRX_FLOW, "App: %d MW:s-get-table-iter \n", xct_id);
        e = _psecurity_man->s_get_table_iter(_pssm, s_iter);
        if (e.is_error()) { goto done; }

        bool eof;
        TRACE( TRACE_TRX_FLOW, "App: %d MF:s-iter-next \n", xct_id);
        e = s_iter->next(_pssm, eof, *prsecurity);
        if (e.is_error()) { goto done; }
        while(!eof){
            char temp[16];
            prsecurity->get_value(0, temp, 16);

            //	  TRACE( TRACE_TRX_FLOW, "App: %d MF:compare SEC and LT (%s) \n", xct_id, temp);
            e =  _plast_trade_man->lt_index_probe(_pssm, prlasttrade, temp);
            if(e.is_error()) { goto done; }
            //	  printf("Found \n");
		  
            //TRACE( TRACE_TRX_FLOW, "App: %d MF:lt-iter-next \n", xct_id);
            e = s_iter->next(_pssm, eof, *prsecurity);
            if (e.is_error()) { goto done; }
		  
		 
        }
    }
	
    {
        int cnt = 0;
        watch_item_man_impl::table_iter* wi_iter;
        TRACE( TRACE_TRX_FLOW, "App: %d MW:wi-get-table-iter \n", xct_id);
        e = _pwatch_item_man->wi_get_table_iter(_pssm, wi_iter);
        if (e.is_error()) { goto done; }

        bool eof;
        TRACE( TRACE_TRX_FLOW, "App: %d MF:wi-iter-next \n", xct_id);
        e = wi_iter->next(_pssm, eof, *prwatchitem);
        if (e.is_error()) { goto done; }
        while(!eof){
            char temp[16];
            prwatchitem->get_value(1, temp, 16);

            //	  TRACE( TRACE_TRX_FLOW, "App: %d MF:compare WI and LT (%s) \n", xct_id, temp);
            //e =  _plast_trade_man->lt_index_probe(_pssm, prlasttrade, temp);
            if(e.is_error()) { goto done; }
            //	  printf("Found \n");
		  
            //TRACE( TRACE_TRX_FLOW, "App: %d MF:lt-iter-next \n", xct_id);
            e = wi_iter->next(_pssm, eof, *prwatchitem);
            if (e.is_error()) { goto done; }  
            cnt++;
        }
		
        printf("WI TABLE COUNT: 	%d \n\n\n\n\n\n\n\n\n\n\n", cnt);
    }
	    
	    
    {
        int cnt = 0, notFound = 0;
        holding_summary_man_impl::table_iter* hs_iter;
        TRACE( TRACE_TRX_FLOW, "App: %d MW:hs-get-table-iter \n", xct_id);
        e = _pholding_summary_man->hs_get_table_iter(_pssm, hs_iter);
        if (e.is_error()) { goto done; }



        bool eof;
        TRACE( TRACE_TRX_FLOW, "App: %d MF:hs-iter-next \n", xct_id);
        e = hs_iter->next(_pssm, eof, *prholdsumm);
        if (e.is_error()) { goto done; }
        while(!eof){		  
		  
            char temp[16];
            prholdsumm->get_value(1, temp, 16);
	

            //TRACE( TRACE_TRX_FLOW, "App: %d MF:compare HS and LT (%s) \n", xct_id, temp);
            //e =  _plast_trade_man->lt_index_probe(_pssm, prlasttrade, temp);
            //if(e.is_error()) { printf("NOT Found in LT \n"); }
            //else printf("Found in LT \n");
		  
            //e =  _psecurity_man->s_index_probe(_pssm, prsecurity, temp);
            //if(e.is_error()){ notFound++; printf("not found %s \n ", temp); }
            //else {cnt++; printf("found %s \n ", temp);}
		  
            //TRACE( TRACE_TRX_FLOW, "App: %d MF:hs-iter-next \n", xct_id);
            e = hs_iter->next(_pssm, eof, *prholdsumm);
            if (e.is_error()) { goto done; } 
		  
        }

		
        //printf("WI TABLE COUNT: 	%d \n\n\n\n\n\n\n\n\n\n\n", cnt);
        // printf("WI TABLE NOT FOUND: 	%d \n\n\n\n\n\n\n\n\n\n\n", notFound);
    }
	    
	    
    {
        int cnt = 0, notFound = 0;
        daily_market_man_impl::table_iter* dm_iter;
        TRACE( TRACE_TRX_FLOW, "App: %d MW:dm-get-table-iter \n", xct_id);
        e = _pdaily_market_man->dm_get_table_iter(_pssm, dm_iter);
        if (e.is_error()) { goto done; }

        bool eof;
        TRACE( TRACE_TRX_FLOW, "App: %d MF:dm-iter-next \n", xct_id);
        e = dm_iter->next(_pssm, eof, *prdailymarket);
        if (e.is_error()) { goto done; }
        while(!eof){
            char temp[16];
            prdailymarket->get_value(1, temp, 16);

            e =  _psecurity_man->s_index_probe(_pssm, prsecurity, temp);
            if(e.is_error()){ notFound++; }
            else {cnt++; }
		  
            //TRACE( TRACE_TRX_FLOW, "App: %d MF:dm-iter-next \n", xct_id);
            e = dm_iter->next(_pssm, eof, *prdailymarket);
            if (e.is_error()) { goto done; }
        }
		
        printf("DM TABLE COUNT: 	%d \n\n\n\n\n\n\n\n\n\n\n", cnt);
        printf("DM TABLE NOT FOUND: 	%d \n\n\n\n\n\n\n\n\n\n\n", notFound);
    }
	    
	   
    {
        int cnt = 0;
        watch_item_man_impl::table_iter* wi_iter;
        TRACE( TRACE_TRX_FLOW, "App: %d MW:wi-get-table-iter \n", xct_id);
        e = _pwatch_item_man->wi_get_table_iter(_pssm, wi_iter);
        if (e.is_error()) { goto done; }

        bool eof;
        TRACE( TRACE_TRX_FLOW, "App: %d MF:wi-iter-next \n", xct_id);
        e = wi_iter->next(_pssm, eof, *prwatchitem);
        if (e.is_error()) { goto done; }
        while(!eof){
            char temp[16];
            prwatchitem->get_value(1, temp, 16);

            //TRACE( TRACE_TRX_FLOW, "App: %d MF:compare HS and LT (%s) \n", xct_id, temp);
            //e =  _plast_trade_man->lt_index_probe(_pssm, prlasttrade, temp);
            //if(e.is_error()) { printf("NOT Found in LT \n"); }
            //printf("Found in LT \n");
		  
            e =  _psecurity_man->s_index_probe(_pssm, prsecurity, temp);
            if(e.is_error()){ printf("WI NOT Found in SEC \n"); }
            //printf("Found in SEC \n");
		  
            e = wi_iter->next(_pssm, eof, *prwatchitem);
            if (e.is_error()) { goto done; }  
            cnt++;
        }
		
        printf("WI TABLE COUNT: 	%d \n\n\n\n\n\n\n\n\n\n\n", cnt);
    }
#endif

    {
        //pmwin.print();
        if(! ((pmwin._acct_id != 0) || (pmwin._cust_id != 0) || strcmp(pmwin._industry_name, "") != 0))
            assert(false); //Harness control

        vector<string> stock_list;
        if(pmwin._cust_id != 0){
            /**
               select
               WI_S_SYMB
               from
               WATCH_ITEM,
               WATCH_LIST
               where
               WI_WL_ID = WL_ID and
               WL_C_ID = cust_id
            */

            guard< index_scan_iter_impl<watch_list_t> > wl_iter;
            {
                index_scan_iter_impl<watch_list_t>* tmp_wl_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d MW:wl-get-iter-by-idx2 (%ld) \n", xct_id,  pmwin._cust_id);
                e = _pwatch_list_man->wl_get_iter_by_index2(_pssm, tmp_wl_iter, prwatchlist, lowrep, highrep, pmwin._cust_id);
                if (e.is_error()) { goto done; }
                wl_iter = tmp_wl_iter;
            }

            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d MW:wl-iter-next \n", xct_id);
            e = wl_iter->next(_pssm, eof, *prwatchlist);
            if (e.is_error()) { goto done; }
            while(!eof){
                TIdent wl_id;
                prwatchlist->get_value(0, wl_id);

                guard< index_scan_iter_impl<watch_item_t> > wi_iter;
                {
                    index_scan_iter_impl<watch_item_t>* tmp_wi_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d MW:wi-get-iter-by-idx (%ld) \n", xct_id,  wl_id);
                    e = _pwatch_item_man->wi_get_iter_by_index(_pssm, tmp_wi_iter, prwatchitem, lowrep, highrep, wl_id);
                    if (e.is_error()) { goto done; }
                    wi_iter = tmp_wi_iter;
                }

                TRACE( TRACE_TRX_FLOW, "App: %d MW:wi-iter-next \n", xct_id);
                e = wi_iter->next(_pssm, eof, *prwatchitem);
                if (e.is_error()) { goto done; }
                while(!eof){
                    char wi_s_symb[16]; //15
                    prwatchitem->get_value(1, wi_s_symb, 16);

                    //printf(" added symbol %s \n", wi_s_symb);
                    string symb(wi_s_symb);
                    stock_list.push_back(symb);

                    TRACE( TRACE_TRX_FLOW, "App: %d MW:wi-iter-next \n", xct_id);
                    e = wi_iter->next(_pssm, eof, *prwatchitem);
                    if (e.is_error()) { goto done; }
                }

                TRACE( TRACE_TRX_FLOW, "App: %d MW:wl-iter-next \n", xct_id);
                e = wl_iter->next(_pssm, eof, *prwatchlist);
                if (e.is_error()) { goto done; }
            }
        }

        else if(pmwin._industry_name[0] != 0){
            /**
               select
               S_SYMB
               from
               INDUSTRY, COMPANY, SECURITY
               where
               IN_NAME = industry_name and
               CO_IN_ID = IN_ID and
               CO_ID between (starting_co_id and ending_co_id) and
               S_CO_ID = CO_ID
            */
            guard< index_scan_iter_impl<security_t> > s_iter;
            {
                index_scan_iter_impl<security_t>* tmp_s_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d MW:s-get-iter-by-idx2 \n", xct_id);				
                e = _psecurity_man->s_get_iter_by_index(_pssm, tmp_s_iter, prsecurity, lowrep, highrep, "");
                s_iter = tmp_s_iter;
                if (e.is_error()) { goto done; }
            }

            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d MW:s-iter-next \n", xct_id);
            e = s_iter->next(_pssm, eof, *prsecurity);
            if (e.is_error()) { goto done; }
            while(!eof){
                TIdent s_co_id;
                prsecurity->get_value(5, s_co_id);

                //printf(" %ld %ld %ld \n", pmwin._starting_co_id, s_co_id, pmwin._ending_co_id);
                if(pmwin._starting_co_id <= s_co_id && s_co_id <= pmwin._ending_co_id){
                    TRACE( TRACE_TRX_FLOW, "App: %d MW:co-idx-probe (%ld) \n", xct_id,  s_co_id);
                    e =  _pcompany_man->co_index_probe(_pssm, prcompany, s_co_id);
                    if (e.is_error()) { goto done; }

                    char co_in_id[3]; //2
                    prcompany->get_value(3, co_in_id, 3);

                    TRACE( TRACE_TRX_FLOW, "App: %d MW:in-idx-probe (%s) \n", xct_id,  co_in_id);
                    e =  _pindustry_man->in_index_probe(_pssm, prindustry, co_in_id);
                    if(e.is_error()) { goto done; }

                    char in_name[51]; //50
                    prindustry->get_value(1, in_name, 51);
					
                    if(strcmp(in_name, pmwin._industry_name) == 0){
                        char s_symb[16]; //15
                        prsecurity->get_value(0, s_symb, 16);
						
                        string symb(s_symb);
                        stock_list.push_back(symb);
                    }
                }
                TRACE( TRACE_TRX_FLOW, "App: %d MW:s-iter-next \n", xct_id);
                e = s_iter->next(_pssm, eof, *prsecurity);
                if (e.is_error()) { goto done; }
            }
        }
        else if(pmwin._acct_id != 0){
            /**
               select
               HS_S_SYMB
               from
               HOLDING_SUMMARY
               where
               HS_CA_ID = acct_id
            */

            guard< index_scan_iter_impl<holding_summary_t> > hs_iter;
            {
                index_scan_iter_impl<holding_summary_t>* tmp_hs_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d MW:hs-iter-by-idx (%ld) \n", xct_id, pmwin._acct_id);
                e = _pholding_summary_man->hs_get_iter_by_index(_pssm, tmp_hs_iter, prholdsumm, lowrep, highrep, pmwin._acct_id);
                hs_iter = tmp_hs_iter;
                if (e.is_error()) { goto done; }
            }

            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d MW:hs-iter-next \n", xct_id);
            e = hs_iter->next(_pssm, eof, *prholdsumm);
            if (e.is_error()) { goto done; }
            while(!eof){
                char hs_s_symb[16]; //15
                prholdsumm->get_value(1, hs_s_symb, 16);

                string symb(hs_s_symb);
                stock_list.push_back(symb);
                //printf("symbol  %s \n", hs_s_symb);

                TRACE( TRACE_TRX_FLOW, "App: %d MW:hs-iter-next \n", xct_id);
                e = hs_iter->next(_pssm, eof, *prholdsumm);
                if (e.is_error()) { goto done; }
            }
        }

        double old_mkt_cap = 0;
        double new_mkt_cap = 0;
        double pct_change;

        vector<string>::iterator stock_list_iter;
        stock_list_iter = stock_list.begin();

        while(stock_list_iter != stock_list.end()){
            char symbol[16]; //15
            strcpy(symbol, (*stock_list_iter).c_str());

            /**
               select
               new_price = LT_PRICE
               from
               LAST_TRADE
               where
               LT_S_SYMB = symbol
            */
			
            TRACE( TRACE_TRX_FLOW, "App: %d MW:lt-idx-probe (%s) \n", xct_id, symbol);
            e =  _plast_trade_man->lt_index_probe(_pssm, prlasttrade, symbol);
            if(e.is_error()) { goto done; }

            double new_price;
            prlasttrade->get_value(2, new_price);

            /**
               select
               s_num_out = S_NUM_OUT
               from
               SECURITY
               where
               S_SYMB = symbol
            */
            TRACE( TRACE_TRX_FLOW, "App: %d MW:s-idx-probe (%s) \n", xct_id, symbol);
            e =  _psecurity_man->s_index_probe(_pssm, prsecurity, symbol);
            if(e.is_error()) { goto done; }

            double s_num_out;
            prsecurity->get_value(6, s_num_out);

            /**
               select
               old_price = DM_CLOSE
               from
               DAILY_MARKET
               where
               DM_S_SYMB = symbol and
               DM_DATE = start_date
            */

            TRACE( TRACE_TRX_FLOW, "App: %d MW:dm-idx1-probe (%s) (%d) \n", xct_id, symbol, pmwin._start_date);
            e =  _pdaily_market_man->dm_index1_probe(_pssm, prdailymarket, symbol, pmwin._start_date);
            if(e.is_error()) { goto done; }

            double old_price;
            prdailymarket->get_value(2, old_price);

            old_mkt_cap += (s_num_out * old_price);
            new_mkt_cap += (s_num_out * new_price);

            stock_list_iter++;
        }

        if(old_mkt_cap != 0){
            pct_change = 100 * (new_mkt_cap / old_mkt_cap - 1);
        }
        else{
            pct_change = 0;
        }
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used

    rcompany.print_tuple();
    rdailymarket.print_tuple();
    rholdsumm.print_tuple();
    rindustry.print_tuple();
    rlasttrade.print_tuple();
    rsecurity.print_tuple();
    rwatchitem.print_tuple();
    rwatchlist.print_tuple();

#endif

 done:
    // return the tuples to the cache
    _pcompany_man->give_tuple(prcompany);
    _pdaily_market_man->give_tuple(prdailymarket);
    _pholding_summary_man->give_tuple(prholdsumm);
    _pindustry_man->give_tuple(prindustry);
    _plast_trade_man->give_tuple(prlasttrade);
    _psecurity_man->give_tuple(prsecurity);
    _pwatch_item_man->give_tuple(prwatchitem);
    _pwatch_list_man->give_tuple(prwatchlist);

    return (e);
}

w_rc_t ShoreTPCEEnv::xct_data_maintenance(const int xct_id, data_maintenance_input_t& pdmin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* pracctperm = _paccount_permission_man->get_tuple();
    assert (pracctperm);

    table_row_t* praddress = _paddress_man->get_tuple();
    assert (praddress);

    table_row_t* prcompany = _pcompany_man->get_tuple();
    assert (prcompany);

    table_row_t* prcustomer = _pcustomer_man->get_tuple();
    assert (prcustomer);

    table_row_t* prcustomertaxrate = _pcustomer_taxrate_man->get_tuple();
    assert (prcustomertaxrate);

    table_row_t* prdailymarket = _pdaily_market_man->get_tuple();
    assert (prdailymarket);

    table_row_t* prexchange = _pexchange_man->get_tuple();
    assert (prexchange);

    table_row_t* prfinancial = _pfinancial_man->get_tuple();
    assert (prfinancial);

    table_row_t* prsecurity = _psecurity_man->get_tuple();
    assert (prsecurity);

    table_row_t* prnewsitem = _pnews_item_man->get_tuple();
    assert (prnewsitem);

    table_row_t* prnewsxref= _pnews_xref_man->get_tuple();
    assert (prnewsxref);

    table_row_t* prtaxrate = _ptaxrate_man->get_tuple();
    assert (prtaxrate);

    table_row_t* prwatchitem = _pwatch_item_man->get_tuple();
    assert (prwatchitem);

    table_row_t* prwatchlist = _pwatch_list_man->get_tuple();
    assert (prwatchlist);

    w_rc_t e = RCOK;
    rep_row_t areprow(_pnews_item_man->ts());
    areprow.set(_pnews_item_desc->maxsize());

    pracctperm->_rep = &areprow;
    praddress->_rep = &areprow;
    prcompany->_rep = &areprow;
    prcustomer->_rep = &areprow;
    prcustomertaxrate->_rep = &areprow;
    prdailymarket->_rep = &areprow;
    prexchange->_rep = &areprow;
    prfinancial->_rep = &areprow;
    prsecurity->_rep = &areprow;
    prnewsitem->_rep = &areprow;
    prnewsxref->_rep = &areprow;
    prtaxrate->_rep = &areprow;
    prwatchitem->_rep = &areprow;
    prwatchlist->_rep = &areprow;

    rep_row_t lowrep( _pnews_item_man->ts());
    rep_row_t highrep( _pnews_item_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_pnews_item_desc->maxsize());
    highrep.set(_pnews_item_desc->maxsize());
    {
        if(strcmp(pdmin._table_name, "ACCOUNT_PERMISSION") == 0){
            /**
               select first 1 row
               acl = AP_ACL
               from
               ACCOUNT_PERMISSION
               where
               AP_CA_ID = acct_id
               order by
               AP_ACL DESC
            */
            guard<index_scan_iter_impl<account_permission_t> > ap_iter;
            {
                index_scan_iter_impl<account_permission_t>* tmp_ap_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-get-iter-by-idx %ld \n", xct_id, pdmin._acct_id);
                e = _paccount_permission_man->ap_get_iter_by_index(_pssm, tmp_ap_iter, pracctperm, lowrep, highrep, pdmin._acct_id);
                if (e.is_error()) { goto done; }
                ap_iter = tmp_ap_iter;
            }
            //descending order
            rep_row_t sortrep(_pnews_item_man->ts());
            sortrep.set(_pnews_item_desc->maxsize());

            sort_buffer_t ap_list(1);
			
            ap_list.setup(0, SQL_FIXCHAR, 4);

            table_row_t rsb(&ap_list);
            sort_man_impl ap_sorter(&ap_list, &sortrep);
			
            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-iter-next \n", xct_id);
            e = ap_iter->next(_pssm, eof, *pracctperm);
            if (e.is_error()) {  goto done; }
            while(!eof){
                char acl[5]; //4
                pracctperm->get_value(1, acl, 5);

                rsb.set_value(0, acl);
                ap_sorter.add_tuple(rsb);

                TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-iter-next \n", xct_id);
                e = ap_iter->next(_pssm, eof, *pracctperm);
                if (e.is_error()) {  goto done; }
            }
            assert (ap_sorter.count());

            sort_iter_impl ap_list_sort_iter(_pssm, &ap_list, &ap_sorter);
			
            TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-sort-iter-next \n", xct_id);
            e = ap_list_sort_iter.next(_pssm, eof, rsb);
            if (e.is_error()) {  goto done; }

            char acl[5]; //4
            rsb.get_value(0, acl, 5);

            if(strcmp(acl, "1111") == 0){
                /**
                   update
                   ACCOUNT_PERMISSION
                   set
                   AP_ACL=”1111”
                   where
                   AP_CA_ID = acct_id and
                   AP_ACL = acl
                */
                {
                    index_scan_iter_impl<account_permission_t>* tmp_ap_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-get-iter-by-idx %ld \n", xct_id, pdmin._acct_id);
                    e = _paccount_permission_man->ap_get_iter_by_index(_pssm, tmp_ap_iter, pracctperm, lowrep, highrep, pdmin._acct_id);
                    if (e.is_error()) {   goto done; }
                    ap_iter = tmp_ap_iter;
                }
				
                TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-iter-next \n", xct_id);
                e = ap_iter->next(_pssm, eof, *pracctperm);
                if (e.is_error()) {  goto done; }
                while(!eof){
                    TIdent ap_ca_id;
                    char ap_acl[5]; //4

                    pracctperm->get_value(0, ap_ca_id);
                    pracctperm->get_value(1, ap_acl, 5);

                    if(ap_ca_id == pdmin._acct_id && strcmp(ap_acl, acl) == 0){
                        char new_acl[5] = "1111"; //4
                        TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-update (%s) \n", xct_id, new_acl);
                        e = _paccount_permission_man->ap_update_acl(_pssm, pracctperm, new_acl);
                        if (e.is_error()) {  goto done; }
                    }

                    TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-iter-next \n", xct_id);
                    e = ap_iter->next(_pssm, eof, *pracctperm);
                    if (e.is_error()) {  goto done; }
                }
            }
            else{
                /**
                   update
                   ACCOUNT_PERMISSION
                   set
                   AP_ACL = ”0011”
                   where
                   AP_CA_ID = acct_id and
                   AP_ACL = acl
                */
                {
                    index_scan_iter_impl<account_permission_t>* tmp_ap_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-get-iter-by-idx \n", xct_id);
                    e = _paccount_permission_man->ap_get_iter_by_index(_pssm, tmp_ap_iter, pracctperm, lowrep, highrep, pdmin._acct_id);
                    if (e.is_error()) {  goto done; }
                    ap_iter = tmp_ap_iter;
                }
                TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-iter-next \n", xct_id);
                e = ap_iter->next(_pssm, eof, *pracctperm);
                if (e.is_error()) {  goto done; }
                while(!eof){
                    char ap_acl[5]; //4
                    pracctperm->get_value(1, ap_acl, 5);

                    if(strcmp(ap_acl, acl) == 0){
                        char new_acl[5] = "0011"; //4

                        TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-update (%s) \n", xct_id, new_acl);
                        e = _paccount_permission_man->ap_update_acl(_pssm, pracctperm, new_acl);
                        if (e.is_error()) {  goto done; }
                    }

                    TRACE( TRACE_TRX_FLOW, "App: %d DM:ap-iter-next \n", xct_id);
                    e = ap_iter->next(_pssm, eof, *pracctperm);
                    if (e.is_error()) {  goto done; }
                }
            }
        }
        else if(strcmp(pdmin._table_name, "ADDRESS") == 0){
            char line2[81] = "\0"; //80
            TIdent ad_id = 0;

            if(pdmin._c_id != 0){
                /**
                   select
                   line2 = AD_LINE2,
                   ad_id = AD_ID
                   from
                   ADDRESS, CUSTOMER
                   where
                   AD_ID = C_AD_ID and
                   C_ID = c_id
                */

                TRACE( TRACE_TRX_FLOW, "App: %d DM:c-idx-probe (%ld) \n", xct_id,  pdmin._c_id);
                e =  _pcustomer_man->c_index_probe(_pssm, prcustomer, pdmin._c_id);
                if (e.is_error()) {  goto done; }

                TIdent c_ad_id;
                prcustomer->get_value(9, c_ad_id);

                TRACE( TRACE_TRX_FLOW, "App: %d DM:ad-idx-probe-for-update (%d) \n", xct_id,  c_ad_id);
                e =  _paddress_man->ad_index_probe_forupdate(_pssm, praddress, c_ad_id);
                if (e.is_error()) {  goto done; }

                praddress->get_value(2, line2, 81);
                praddress->get_value(0, ad_id);
            }
            else{
                /**
                   select
                   line2 = AD_LINE2,
                   ad_id = AD_ID
                   from
                   ADDRESS, COMPANY
                   where
                   AD_ID = CO_AD_ID and
                   CO_ID = co_id
                */

                TRACE( TRACE_TRX_FLOW, "App: %d DM:co-idx-probe (%ld) \n", xct_id,  pdmin._co_id);
                e =  _pcompany_man->co_index_probe(_pssm, prcompany, pdmin._co_id);
                if (e.is_error()) {  goto done; }

                TIdent co_ad_id;
                prcompany->get_value(6, co_ad_id);

                TRACE( TRACE_TRX_FLOW, "App: %d DM:ad-idx-probe (%ld) \n", xct_id,  co_ad_id);
                e =  _paddress_man->ad_index_probe_forupdate(_pssm, praddress, co_ad_id);
                if (e.is_error()) {  goto done; }

                praddress->get_value(2, line2, 81);
                praddress->get_value(0, ad_id);
            }
            if(strcmp(line2, "Apt. 10C") != 0){
                /**
                   update
                   ADDRESS
                   set
                   AD_LINE2 = “Apt. 10C”
                   where
                   AD_ID = ad_id
                */

                char new_line2[81] = "Apt. 10C" ;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:ad-update (%ld) (%s) \n", xct_id, ad_id, new_line2);
                e = _paddress_man->ad_update_line2(_pssm, praddress, new_line2);
                if (e.is_error()) {  goto done; }
            }
            else{
                /**
                   update
                   ADDRESS
                   set
                   AD_LINE2 = “Apt. 22”
                   where
                   AD_ID = ad_id
                */

                char new_line2[81] = "Apt. 22";
                TRACE( TRACE_TRX_FLOW, "App: %d DM:ad-update (%ld) (%s) \n", xct_id, ad_id, new_line2);
                e = _paddress_man->ad_update_line2(_pssm, praddress, new_line2);
                if (e.is_error()) {  goto done; }
            }
        }
        else if(strcmp(pdmin._table_name, "COMPANY") == 0){
            char sprate[5] = "\0"; //4

            TRACE( TRACE_TRX_FLOW, "App: %d DM:co-idx-probe (%ld) \n", xct_id,  pdmin._co_id);
            e =  _pcompany_man->co_index_probe_forupdate(_pssm, prcompany, pdmin._co_id);
            if(e.is_error()) {  goto done; }

            prcompany->get_value(4, sprate, 5);

            if(strcmp(sprate, "ABA") != 0){
                /**
                   update
                   COMPANY
                   set
                   CO_SP_RATE = “ABA”
                   where
                   CO_ID = co_id
                */

                char new_sprate[5] = "ABA";
                TRACE( TRACE_TRX_FLOW, "App: %d DM:co-update (%s) \n", xct_id, new_sprate);
                e = _pcompany_man->co_update_sprate(_pssm, prcompany, new_sprate);
                if (e.is_error()) {  goto done; }
            }
            else{
                /**
                   update
                   COMPANY
                   set
                   CO_SP_RATE = “AAA”
                   where
                   CO_ID = co_id
                */

                char new_sprate[5] = "AAA";
                TRACE( TRACE_TRX_FLOW, "App: %d DM:co-update (%s) \n", xct_id,new_sprate);
                e = _pcompany_man->co_update_sprate(_pssm, prcompany, new_sprate);
                if (e.is_error()) {   goto done; }
            }
        }
        else if(strcmp(pdmin._table_name, "CUSTOMER") == 0){
            char email2[51] = "\0"; //50
            int len = 0;
            int lenMindspring = strlen("@mindspring.com");

            /**
               select
               email2 = C_EMAIL_2
               from
               CUSTOMER
               where
               C_ID = c_id
            */

            TRACE( TRACE_TRX_FLOW, "App: %d DM:c-idx-probe (%d) \n", xct_id,  pdmin._c_id);
            e =  _pcustomer_man->c_index_probe_forupdate(_pssm, prcustomer, pdmin._c_id);
            if (e.is_error()) {  goto done; }

            prcustomer->get_value(23, email2, 51);

            len = strlen(email2);
            string temp_email2(email2);
            if( ((len - lenMindspring) > 0 && (temp_email2.substr(len-lenMindspring, lenMindspring).compare("@mindspring.com") == 0))){
                /**
                   update
                   CUSTOMER
                   set
                   C_EMAIL_2 = substring(C_EMAIL_2, 1,
                   charindex(“@”,C_EMAIL_2) ) + „earthlink.com‟
                   where
                   C_ID = c_id
                */

                char new_email2[51];
                string temp_new_email2 = temp_email2.substr(1, temp_email2.find_first_of('@')) + "earthlink.com";
                strcpy(new_email2, temp_new_email2.c_str());

                TRACE( TRACE_TRX_FLOW, "App: %d DM:c-update (%s) \n", xct_id, new_email2);
                e = _pcustomer_man->c_update_email2(_pssm, prcustomer, new_email2);
                if (e.is_error()) {  goto done; }
            }
            else{
                /**
                   update
                   CUSTOMER
                   set
                   C_EMAIL_2 = substring(C_EMAIL_2, 1,
                   charindex(“@”,C_EMAIL_2) ) + „mindspring.com‟
                   where
                   C_ID = c_id
                */
                char new_email2[51]; //50
                string temp_new_email2 = temp_email2.substr(1, temp_email2.find_first_of('@')) + "mindspring.com";
                strcpy(new_email2, temp_new_email2.c_str());

                TRACE( TRACE_TRX_FLOW, "App: %d DM:c-update (%s) \n", xct_id, new_email2);
                e = _pcustomer_man->c_update_email2(_pssm, prcustomer, new_email2);
                if (e.is_error()) {  goto done; }
            }
        }
        else if(strcmp(pdmin._table_name, "CUSTOMER_TAXRATE") == 0){
#warning Djole CHECK
            char old_tax_rate[5];//4
            char new_tax_rate[5];//4
            int tax_num;

            /**
               select
               old_tax_rate = CX_TX_ID
               from
               CUSTOMER_TAXRATE
               where
               CX_C_ID = c_id and (CX_TX_ID like “US%” or CX_TX_ID like “CN%”)
            */
            guard< index_scan_iter_impl<customer_taxrate_t> > cx_iter;
            {
                index_scan_iter_impl<customer_taxrate_t>* tmp_cx_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:cx-get-iter-by-idx (%ld) \n", xct_id, pdmin._c_id);
                e = _pcustomer_taxrate_man->cx_get_iter_by_index(_pssm, tmp_cx_iter, prcustomertaxrate, lowrep, highrep, pdmin._c_id);
                if (e.is_error()) {  goto done; }
                cx_iter = tmp_cx_iter;
            }

            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d DM:cx-iter-next \n", xct_id);
            e = cx_iter->next(_pssm, eof, *prcustomertaxrate);
            if (e.is_error()) {  goto done; }
            while(!eof){
                char cx_tx_id[5];//4
                prcustomertaxrate->get_value(0, cx_tx_id, 5);
				
                string temp_cx_tx_id(cx_tx_id);
                if(temp_cx_tx_id.substr(0,2).compare("US") == 0 || temp_cx_tx_id.substr(0,2).compare("CN") == 0 ){
                    strcpy(old_tax_rate, cx_tx_id);				

                    string temp_old_tax_rate(old_tax_rate);
                    if(temp_old_tax_rate.substr(0,2).compare("US") == 0){
                        if(temp_old_tax_rate.compare("US5") == 0){
                            strcpy(new_tax_rate, "US1");
                        }
                        else{
                            tax_num = atoi(temp_old_tax_rate.substr(2, 1).c_str());
                            stringstream temp_new_tax_rate;
                            temp_new_tax_rate << "US" << (++tax_num);
                            strcpy(new_tax_rate, temp_new_tax_rate.str().c_str());
                        }
                    }
                    else{
                        if(temp_old_tax_rate.compare("CN4") == 0){
                            strcpy(new_tax_rate, "CN1");
                        }
                        else{
                            tax_num = atoi(temp_old_tax_rate.substr(2, 1).c_str());
                            stringstream temp_new_tax_rate;
                            temp_new_tax_rate << "CN" << (++tax_num);
                            strcpy(new_tax_rate, temp_new_tax_rate.str().c_str());
                        }
                    }

                    /**
                       update
                       CUSTOMER_TAXRATE
                       set
                       CX_TX_ID = new_tax_rate
                       where
                       CX_C_ID = c_id and
                       CX_TX_ID = old_tax_rate
                    */

                    TRACE( TRACE_TRX_FLOW, "App: %d DM:cx-update (%s) \n", xct_id, new_tax_rate);
                    e = _pcustomer_taxrate_man->cx_update_txid(_pssm, prcustomertaxrate, new_tax_rate);
                    if (e.is_error()) {  goto done; } //buggy, I suppose there is problem when trying to update a FIXCHAR field which is in index
                }
                TRACE( TRACE_TRX_FLOW, "App: %d DM:cx-iter-next \n", xct_id);
                e = cx_iter->next(_pssm, eof, *prcustomertaxrate);
                if (e.is_error()) {  goto done; }
            }
        }
        else if(strcmp(pdmin._table_name, "DAILY_MARKET") == 0){
            /**
               update
               DAILY_MARKET
               set
               DM_VOL = DM_VOL + vol_incr
               where
               DM_S_SYMB = symbol
               and substring ((convert(char(8),DM_DATE,3),1,2) = day_of_month
            */
			
            guard< index_scan_iter_impl<daily_market_t> > dm_iter;
            {
                index_scan_iter_impl<daily_market_t>* tmp_dm_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:dm-get-iter-by-idx4 (%s) \n", xct_id, pdmin._symbol);
                e = _pdaily_market_man->dm_get_iter_by_index4(_pssm, tmp_dm_iter, prdailymarket, lowrep, highrep, pdmin._symbol);
                if (e.is_error()) {  goto done; }
                dm_iter = tmp_dm_iter;
            }

            bool eof;
            e = dm_iter->next(_pssm, eof, *prdailymarket);
            if (e.is_error()) { goto done; }
            while(!eof){
                myTime dm_date;
                prdailymarket->get_value(0, dm_date);

                if(dayOfMonth(dm_date) == pdmin._day_of_month){
                    TRACE( TRACE_TRX_FLOW, "App: %d MD:dm-update (%d) \n", xct_id, pdmin._vol_incr);
                    e = _pdaily_market_man->dm_update_vol(_pssm, prdailymarket, pdmin._vol_incr);
                    if (e.is_error()) {  goto done; }
                }

                e = dm_iter->next(_pssm, eof, *prdailymarket);
                if (e.is_error()) {  goto done; }
            }			
        }
        else if(strcmp(pdmin._table_name, "EXCHANGE") == 0){
            int rowcount = 0;
            /**
               select
               rowcount = count(*)
               from
               EXCHANGE
               where
               EX_DESC like “%LAST UPDATED%”
            */

            guard<index_scan_iter_impl<exchange_t> > ex_iter;
            {
                index_scan_iter_impl<exchange_t>* tmp_ex_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:ex-get-iter-by-idx \n", xct_id);
                e = _pexchange_man->ex_get_iter_by_index(_pssm, tmp_ex_iter, prexchange, lowrep, highrep);
                if (e.is_error()) {  goto done; }
                ex_iter = tmp_ex_iter;
            }

            bool eof;
            e = ex_iter->next(_pssm, eof, *prexchange);
            if (e.is_error()) { goto done; }
            while(!eof){
                char ex_desc[151]; //150
                prexchange->get_value(5, ex_desc, 151);

                string temp_ex_desc(ex_desc);
                if(temp_ex_desc.find("LAST UPDATED") != -1){
                    rowcount++;
                }

                e = ex_iter->next(_pssm, eof, *prexchange);
                if (e.is_error()) {  goto done; }
            }
            if(rowcount == 0){
                /**
                   update
                   EXCHANGE
                   set
                   EX_DESC = EX_DESC + “ LAST UPDATED “ + getdatetime()
                */
                {
                    index_scan_iter_impl<exchange_t>* tmp_ex_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d DM:ex-get-iter-by-idx \n", xct_id);
                    e = _pexchange_man->ex_get_iter_by_index(_pssm, tmp_ex_iter, prexchange, lowrep, highrep);
                    if (e.is_error()) {  goto done; }
                    ex_iter = tmp_ex_iter;
                }
                e = ex_iter->next(_pssm, eof, *prexchange);
                if (e.is_error()) {  goto done; }
                while(!eof){
                    char ex_desc[151]; //150
                    prexchange->get_value(5, ex_desc, 151);

                    string new_desc(ex_desc);
                    stringstream ss;
                    ss << "" << new_desc << " LAST UPDATED " << time(NULL) << "";
					
                    TRACE( TRACE_TRX_FLOW, "App: %d MD:ex-update (%s) \n", xct_id, ss.str().c_str());
                    e = _pexchange_man->ex_update_desc(_pssm, prexchange, ss.str().c_str());
                    if (e.is_error()) {  goto done; }

                    e = ex_iter->next(_pssm, eof, *prexchange);
                    if (e.is_error()) {  goto done; }
                }
            }
            else{
                /**
                   update
                   EXCHANGE
                   set
                   EX_DESC = substring(EX_DESC,1,
                   len(EX_DESC)-len(getdatetime())) + getdatetime()
                */
                index_scan_iter_impl<exchange_t>* tmp_ex_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:ex-get-iter-by-idx \n", xct_id);
                e = _pexchange_man->ex_get_iter_by_index(_pssm, tmp_ex_iter, prexchange, lowrep, highrep);
                if (e.is_error()) { goto done; }
                ex_iter = tmp_ex_iter;

                e = ex_iter->next(_pssm, eof, *prexchange);
                if (e.is_error()) {  goto done; }
                while(!eof){
                    ex_iter = tmp_ex_iter;
                    char ex_desc[151]; //150
                    prexchange->get_value(5, ex_desc, 151);

                    string temp(ex_desc), new_desc;
                    new_desc = temp.substr(0, temp.find_last_of(" ") + 1);
                    stringstream ss;
                    ss << "" << new_desc << time(NULL);

                    TRACE( TRACE_TRX_FLOW, "App: %d MD:ex-update (%s) \n", xct_id, ss.str().c_str());
                    e = _pexchange_man->ex_update_desc(_pssm, prexchange, ss.str().c_str());
                    if (e.is_error()) { goto done; }

                    e = ex_iter->next(_pssm, eof, *prexchange);
                    if (e.is_error()) { goto done; }
                }
            }
        }
        else if(strcmp(pdmin._table_name, "FINANCIAL") == 0){
            int rowcount = 0;

            /**
               select
               rowcount = count(*)
               from
               FINANCIAL
               where
               FI_CO_ID = co_id and
               substring(convert(char(8),
               FI_QTR_START_DATE,2),7,2) = “01”
            */

            guard< index_scan_iter_impl<financial_t> > fi_iter;
            {
                index_scan_iter_impl<financial_t>* tmp_fi_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:fi-get-iter-by-idx (%ld) \n", xct_id,  pdmin._co_id);
                e = _pfinancial_man->fi_get_iter_by_index(_pssm, tmp_fi_iter, prfinancial, lowrep, highrep, pdmin._co_id);
                if (e.is_error()) {  goto done; }
                fi_iter = tmp_fi_iter;
            }

            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d DM:fi-iter-next \n", xct_id);
            e = fi_iter->next(_pssm, eof, *prfinancial);
            if (e.is_error()) {  goto done; }
            while(!eof){
                myTime fi_qtr_start_date;
                prfinancial->get_value(3, fi_qtr_start_date);

                if(dayOfMonth(fi_qtr_start_date) == 1){
                    rowcount++;
                }
				
                TRACE( TRACE_TRX_FLOW, "App: %d DM:fi-iter-next \n", xct_id);
                e = fi_iter->next(_pssm, eof, *prfinancial);
                if (e.is_error()) {  goto done; }
            }
            if(rowcount > 0){
                /**
                   update
                   FINANCIAL
                   set
                   FI_QTR_START_DATE = FI_QTR_START_DATE + 1 day
                   where
                   FI_CO_ID = co_id
                */
                {
                    index_scan_iter_impl<financial_t>* tmp_fi_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d DM:fi-get-iter-by-idx (%ld) \n", xct_id,  pdmin._co_id);
                    e = _pfinancial_man->fi_get_iter_by_index(_pssm, tmp_fi_iter, prfinancial, lowrep, highrep, pdmin._co_id);
                    if (e.is_error()) {  goto done; }
                    fi_iter = tmp_fi_iter;
                }
                TRACE( TRACE_TRX_FLOW, "App: %d DM:fi-iter-next \n", xct_id);
                e = fi_iter->next(_pssm, eof, *prfinancial);
                if (e.is_error()) { goto done; }
                while(!eof){
                    myTime fi_qtr_start_date;
                    prfinancial->get_value(3, fi_qtr_start_date);

                    fi_qtr_start_date += (60*60*24); // add 1 day

                    TRACE( TRACE_TRX_FLOW, "App: %d MD:fi-update (%ld) \n", xct_id, fi_qtr_start_date);
                    e = _pfinancial_man->fi_update_desc(_pssm, prfinancial, fi_qtr_start_date);
                    if (e.is_error()) {  goto done; }
					
                    TRACE( TRACE_TRX_FLOW, "App: %d DM:fi-iter-next \n", xct_id);
                    e = fi_iter->next(_pssm, eof, *prfinancial);
                    if (e.is_error()) {  goto done; }
                }
            }
            else{
                /**
                   update
                   FINANCIAL
                   set
                   FI_QTR_START_DATE = FI_QTR_START_DATE – 1 day
                   where
                   FI_CO_ID = co_id
                */
                {
                    index_scan_iter_impl<financial_t>* tmp_fi_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d DM:fi-get-iter-by-idx (%ld) \n", xct_id,  pdmin._co_id);
                    e = _pfinancial_man->fi_get_iter_by_index(_pssm, tmp_fi_iter, prfinancial, lowrep, highrep, pdmin._co_id);
                    if (e.is_error()) { goto done; }
                    fi_iter = tmp_fi_iter;
                }
                TRACE( TRACE_TRX_FLOW, "App: %d DM:fi-iter-next \n", xct_id);
                e = fi_iter->next(_pssm, eof, *prfinancial);
                if (e.is_error()) {  goto done; }
                while(!eof){
                    myTime fi_qtr_start_date;
                    prfinancial->get_value(3, fi_qtr_start_date);

                    fi_qtr_start_date -= (60*60*24);

                    TRACE( TRACE_TRX_FLOW, "App: %d MD:fi-update-desc (%ld) \n", xct_id, fi_qtr_start_date);
                    e = _pfinancial_man->fi_update_desc(_pssm, prfinancial, fi_qtr_start_date);
                    if (e.is_error()) {  goto done; }
					
                    TRACE( TRACE_TRX_FLOW, "App: %d DM:fi-iter-next \n", xct_id);
                    e = fi_iter->next(_pssm, eof, *prfinancial);
                    if (e.is_error()) {  goto done; }
                }
            }
        }
        else if(strcmp(pdmin._table_name, "NEWS_ITEM") == 0){
            /**
               update
               NEWS_ITEM
               set
               NI_DTS = NI_DTS + 1day
               where
               NI_ID = (
               select
               NX_NI_ID
               from
               NEWS_XREF
               where
               NX_CO_ID = @co_id)
            */
            guard< index_scan_iter_impl<news_xref_t> > nx_iter;
            {
                index_scan_iter_impl<news_xref_t>* tmp_nx_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:nx-get-iter-by-idx (%ld) \n", xct_id, pdmin._co_id);
                e = _pnews_xref_man->nx_get_iter_by_index(_pssm, tmp_nx_iter, prnewsxref, lowrep, highrep, pdmin._co_id);
                if (e.is_error()) {  goto done; }
                nx_iter = tmp_nx_iter;
            }

            bool eof;
            e = nx_iter->next(_pssm, eof, *prnewsxref);
            if (e.is_error()) {  goto done; }
            while(!eof){
                TIdent nx_ni_id;
                prnewsxref->get_value(0, nx_ni_id);

                TRACE( TRACE_TRX_FLOW, "App: %d DM:nx-idx-probe (%d) \n", xct_id, nx_ni_id);
                e =  _pnews_item_man->ni_index_probe_forupdate(_pssm, prnewsitem, nx_ni_id);
                if (e.is_error()) {  goto done; }

                myTime ni_dts;
                prnewsitem->get_value(4, ni_dts);

                ni_dts += (60 * 60 * 24);

                TRACE( TRACE_TRX_FLOW, "App: %d DM:nx-update-nidts (%d) \n", xct_id, ni_dts);
                e =  _pnews_item_man->ni_update_dts_by_index(_pssm, prnewsitem, ni_dts);
                if (e.is_error()) {  goto done; }

                e = nx_iter->next(_pssm, eof, *prnewsxref);
                if (e.is_error()) {  goto done; }
            }
        }
        else if(strcmp(pdmin._table_name, "SECURITY") == 0){
            /**
               update
               SECURITY
               set
               S_EXCH_DATE = S_EXCH_DATE + 1day
               where
               S_SYMB = symbol
            */

            TRACE( TRACE_TRX_FLOW, "App: %d DM:s-idx-probe (%s) \n", xct_id, pdmin._symbol);
            e =  _psecurity_man->s_index_probe_forupdate(_pssm, prsecurity, pdmin._symbol);
            if (e.is_error()) {  goto done; }

            myTime s_exch_date;
            prsecurity->get_value(8, s_exch_date);

            s_exch_date += (60 * 60 * 24);

            TRACE( TRACE_TRX_FLOW, "App: %d DM:s-update-ed (%d) \n", xct_id, s_exch_date );
            e =  _psecurity_man->s_update_ed(_pssm, prsecurity, s_exch_date);
            if (e.is_error()) {  goto done; }
        }
        else if(strcmp(pdmin._table_name, "TAXRATE") == 0){
            /**
               select
               tx_name = TX_NAME
               from
               TAXRATE
               where
               TX_ID = tx_id	
            */
            /*
              char tx_name[51]; //50

              TRACE( TRACE_TRX_FLOW, "App: %d DM:tx-idx-probe (%d) \n", xct_id, pdmin._tx_id);
              e =  _ptaxrate_man->tx_index_probe_forupdate(_pssm, prtaxrate, pdmin._tx_id);
              if (e.is_error()) {  goto done; }

              prtaxrate->get_value(1, tx_name, 51);

              //printf("Tax Name %s \n", tx_name);
              string temp(tx_name);
              size_t index = temp.find("Tax");
              if(index != string::npos){
              temp.replace(index, 3, "tax");
              }
              else{
              index = temp.find("tax");
              temp.replace(index, 3, "Tax");
              }
              strcpy(tx_name, temp.c_str());

              /**
              update
              TAXRATE
              set
              TX_NAME = tx_name
              where
              TX_ID = tx_id
            */
            /*
              TRACE( TRACE_TRX_FLOW, "App: %d DM:tx-update-name (%s) \n", xct_id, tx_name);
              e =  _ptaxrate_man->tx_update_name(_pssm, prtaxrate, tx_name);
              if (e.is_error()) {  goto done; }
            */
        }
        else if(strcmp(pdmin._table_name, "WATCH_ITEM") == 0){
            /**
               select
               cnt = count(*)     // number of rows is [50..150]
               from
               WATCH_ITEM,
               WATCH_LIST
               where
               WL_C_ID = c_id and
               WI_WL_ID = WL_ID
            */

            int cnt = 0;
            char old_symbol[16], new_symbol[16] = "\0"; //15, 15

            guard< index_scan_iter_impl<watch_list_t> > wl_iter;
            {
                index_scan_iter_impl<watch_list_t>* tmp_wl_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:wl-get-iter-by-idx2 (%ld) \n", xct_id,  pdmin._c_id);
                e = _pwatch_list_man->wl_get_iter_by_index2(_pssm, tmp_wl_iter, prwatchlist, lowrep, highrep, pdmin._c_id);
                if (e.is_error()) {  goto done; }
                wl_iter = tmp_wl_iter;
            }

            bool eof;
            e = wl_iter->next(_pssm, eof, *prwatchlist);
            if (e.is_error()) {  goto done; }
            while(!eof){
                TIdent wl_id;
                prwatchlist->get_value(0, wl_id);

                guard< index_scan_iter_impl<watch_item_t> > wi_iter;
                {
                    index_scan_iter_impl<watch_item_t>* tmp_wi_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d DM:wi-get-iter-by-idx (%ld) \n", xct_id,  wl_id);
                    e = _pwatch_item_man->wi_get_iter_by_index(_pssm, tmp_wi_iter, prwatchitem, lowrep, highrep, wl_id);
                    if (e.is_error()) {  goto done; }
                    wi_iter = tmp_wi_iter;
                }

                e = wi_iter->next(_pssm, eof, *prwatchitem);
                if (e.is_error()) {  goto done; }
                while(!eof){
                    cnt++;

                    char wi_s_symbol[16]; //15
                    prwatchitem->get_value(1, wi_s_symbol, 16);

                    e = wi_iter->next(_pssm, eof, *prwatchitem);
                    if (e.is_error()) {  goto done; }
                }

                e = wl_iter->next(_pssm, eof, *prwatchlist);
                if (e.is_error()) {  goto done; }
            }

            cnt = (cnt+1)/2;

            /**
               select
               old_symbol = WI_S_SYMB
               from
               ( select
               ROWNUM,
               WI_S_SYMB
               from
               WATCH_ITEM,
               WATCH_LIST
               where
               WL_C_ID = c_id and
               WI_WL_ID = WL_ID and
               order by
               WI_S_SYMB asc
               )
               where
               rownum = cnt
            */
            //already sorted in ascending order because of its index

            index_scan_iter_impl<watch_list_t>* tmp_wl_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d DM:wl-get-iter-by-idx2 (%ld) \n", xct_id,  pdmin._c_id);
            e = _pwatch_list_man->wl_get_iter_by_index2(_pssm, tmp_wl_iter, prwatchlist, lowrep, highrep, pdmin._c_id);
            if (e.is_error()) {  goto done; }
            wl_iter = tmp_wl_iter;

            e = wl_iter->next(_pssm, eof, *prwatchlist);
            if (e.is_error()) { goto done; }
            while(!eof && cnt > 0){
                TIdent wl_id;
                prwatchlist->get_value(0, wl_id);

                guard< index_scan_iter_impl<watch_item_t> > wi_iter;
                {
                    index_scan_iter_impl<watch_item_t>* tmp_wi_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d DM:wi-get-iter-by-idx (%ld) \n", xct_id,  wl_id);
                    e = _pwatch_item_man->wi_get_iter_by_index(_pssm, tmp_wi_iter, prwatchitem, lowrep, highrep, wl_id);
                    if (e.is_error()) {  goto done; }
                    wi_iter = tmp_wi_iter;
                }

                e = wi_iter->next(_pssm, eof, *prwatchitem);
                if (e.is_error()) {  goto done; }
                while(!eof){
                    cnt--;
                    if(cnt == 0){
                        prwatchitem->get_value(1, old_symbol, 16);
                        break;
                    }

                    e = wi_iter->next(_pssm, eof, *prwatchitem);
                    if (e.is_error()) {  goto done; }
                }

                e = wl_iter->next(_pssm, eof, *prwatchlist);
                if (e.is_error()) {  goto done; }
            }

            /**
               select first 1
               new_symbol = S_SYMB
               from
               SECURITY
               where
               S_SYMB > old_symbol and
               S_SYMB not in (
               select
               WI_S_SYMB
               from
               WATCH_ITEM,
               WATCH_LIST
               where
               WL_C_ID = c_id and
               WI_WL_ID = WL_ID
               )
               order by
               S_SYMB asc
            */
            //already sorted in ascending order because of its index

            guard< index_scan_iter_impl<security_t> > s_iter;
            {
                index_scan_iter_impl<security_t>* tmp_s_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:s-get-iter-by-idx (%s) \n", xct_id, old_symbol);
                e = _psecurity_man->s_get_iter_by_index(_pssm, tmp_s_iter, prsecurity, lowrep, highrep, old_symbol);
                if (e.is_error()) {  goto done; }
                s_iter = tmp_s_iter;
            }

            e = s_iter->next(_pssm, eof, *prsecurity);
            if (e.is_error()) {  goto done; }
            while(!eof){
                char s_symb[16]; //15
                prsecurity->get_value(0, s_symb, 16);

                index_scan_iter_impl<watch_list_t>* tmp_wl_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d DM:wl-get-iter-by-idx2 (%ld) \n", xct_id,  pdmin._c_id);
                e = _pwatch_list_man->wl_get_iter_by_index2(_pssm, tmp_wl_iter, prwatchlist, lowrep, highrep, pdmin._c_id);
                if (e.is_error()) {  goto done; }
                wl_iter = tmp_wl_iter;

                e = wl_iter->next(_pssm, eof, *prwatchlist);
                if (e.is_error()) {  goto done; }
                while(!eof){
                    TIdent wl_id;
                    prwatchlist->get_value(0, wl_id);

                    guard< index_scan_iter_impl<watch_item_t> > wi_iter;
                    {
                        index_scan_iter_impl<watch_item_t>* tmp_wi_iter;
                        TRACE( TRACE_TRX_FLOW, "App: %d DM:wi-get-iter-by-idx (%ld) \n", xct_id,  wl_id);
                        e = _pwatch_item_man->wi_get_iter_by_index(_pssm, tmp_wi_iter, prwatchitem, lowrep, highrep, wl_id);
                        if (e.is_error()) {  goto done; }
                        wi_iter = tmp_wi_iter;
                    }

                    e = wi_iter->next(_pssm, eof, *prwatchitem);
                    if (e.is_error()) {  goto done; }
                    while(!eof){
                        char wi_s_symb[16]; //15
                        prwatchitem->get_value(1, wi_s_symb, 16);

                        if(strcmp(s_symb, wi_s_symb) != 0){
                            strcpy(new_symbol, s_symb);
                            break;
                        }

                        e = wi_iter->next(_pssm, eof, *prwatchitem);
                        if (e.is_error()) {  goto done; }
                    }
                    if(strcmp(new_symbol, "\0") != 0)
                        break;

                    e = wl_iter->next(_pssm, eof, *prwatchlist);
                    if (e.is_error()) {  goto done; }
                }
                if(strcmp(new_symbol, "\0") != 0)
                    break;

                e = s_iter->next(_pssm, eof, *prsecurity);
                if (e.is_error()) {  goto done; }
            }

            /**
               update
               WATCH_ITEM
               set
               WI_S_SYMB = new_symbol
               from
               WATCH_LIST
               where
               WL_C_ID = c_id and
               WI_WL_ID = WL_ID and
               WI_S_SYMB = old_symbol
            */

            TRACE( TRACE_TRX_FLOW, "App: %d DM:wl-get-iter-by-idx2 (%ld) \n", xct_id,  pdmin._c_id);
            e = _pwatch_list_man->wl_get_iter_by_index2(_pssm, tmp_wl_iter, prwatchlist, lowrep, highrep, pdmin._c_id);
            if (e.is_error()) {  goto done; }
            wl_iter = tmp_wl_iter;

            e = wl_iter->next(_pssm, eof, *prwatchlist);
            if (e.is_error()) {  goto done; }
            while(!eof && cnt > 0){
                TIdent wl_id;
                prwatchlist->get_value(0, wl_id);

                TRACE( TRACE_TRX_FLOW, "App: %d DM:wi-update-symb (%ld) (%s) (%s) \n", xct_id, wl_id, old_symbol, new_symbol);
                e =  _pwatch_item_man->wi_update_symb(_pssm, prwatchitem, wl_id, old_symbol, new_symbol);
                if(e.is_error()) {  goto done; }

                e = wl_iter->next(_pssm, eof, *prwatchlist);
                if (e.is_error()) {  goto done; }
            }
        }
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    racctperm.print_tuple();
    raddress.print_tuple();
    rcompany.print_tuple();
    rcustomer.print_tuple();
    rcustomertaxrate.print_tuple();
    rdailymarket.print_tuple();
    rexchange.print_tuple();
    rfinancial.print_tuple();
    rsecurity.print_tuple();
    rnewsitem.print_tuple();
    rnewsxref.print_tuple();
    rtaxrate.print_tuple();
    rwatchitem.print_tuple();
    rwatchlist.print_tuple();

#endif

 done:
    // return the tuples to the cache

    _paccount_permission_man->give_tuple(pracctperm);
    _paddress_man->give_tuple(praddress);
    _pcompany_man->give_tuple(prcompany);
    _pcustomer_man->give_tuple(prcustomer);
    _ptaxrate_man->give_tuple(prtaxrate);
    _pdaily_market_man->give_tuple(prdailymarket);
    _pexchange_man->give_tuple(prexchange);
    _pfinancial_man->give_tuple(prfinancial);
    _psecurity_man->give_tuple(prsecurity);
    _pnews_item_man->give_tuple(prnewsitem);
    _pnews_xref_man->give_tuple(prnewsxref);
    _ptaxrate_man->give_tuple(prtaxrate);
    _pwatch_item_man->give_tuple(prwatchitem);
    _pwatch_list_man->give_tuple(prwatchlist);



    return (e);
}

//SECURITY_DETAIL

w_rc_t ShoreTPCEEnv::xct_security_detail(const int xct_id, security_detail_input_t& psdin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* praddress = _paddress_man->get_tuple();
    assert (praddress);

    table_row_t* prcompany = _pcompany_man->get_tuple(); //296
    assert (prcompany);

    table_row_t* prcompanycomp = _pcompany_competitor_man->get_tuple();
    assert (prcompanycomp);

    table_row_t* prdailymarket = _pdaily_market_man->get_tuple();
    assert (prdailymarket);

    table_row_t* prexchange = _pexchange_man->get_tuple();
    assert (prexchange);

    table_row_t* prfinancial = _pfinancial_man->get_tuple();
    assert (prfinancial);

    table_row_t* prindustry = _pindustry_man->get_tuple();
    assert (prindustry);

    table_row_t* prlasttrade = _plast_trade_man->get_tuple();
    assert (prlasttrade);

    table_row_t* prnewsitem = _pnews_item_man->get_tuple();
    assert (prnewsitem);

    table_row_t* prnewsxref= _pnews_xref_man->get_tuple();
    assert (prnewsxref);

    table_row_t* prsecurity = _psecurity_man->get_tuple();
    assert (prsecurity);

    table_row_t* przipcode = _pzip_code_man->get_tuple();
    assert (przipcode);

    w_rc_t e = RCOK;
    rep_row_t areprow(_pnews_item_man->ts());
    areprow.set(_pnews_item_desc->maxsize());

    praddress->_rep = &areprow;
    prcompany->_rep = &areprow;
    prcompanycomp->_rep = &areprow;
    prdailymarket->_rep = &areprow;
    prexchange->_rep = &areprow;
    prfinancial->_rep = &areprow;
    prindustry->_rep = &areprow;
    prlasttrade->_rep = &areprow;
    prnewsitem->_rep = &areprow;
    prnewsxref->_rep = &areprow;
    prsecurity->_rep = &areprow;
    przipcode->_rep = &areprow;

    rep_row_t lowrep( _pnews_item_man->ts());
    rep_row_t highrep( _pnews_item_man->ts());

    {
        //trial

    }
    // allocate space for the biggest of the table representations
    lowrep.set(_pnews_item_desc->maxsize());
    highrep.set(_pnews_item_desc->maxsize());
    {
        //psdin.print();
        /**
           select
           s_name          = S_NAME,
           co_id           = CO_ID,
           co_name         = CO_NAME,
           sp_rate         = CO_SP_RATE
           ceo_name        = CO_CEO,
           co_desc         = CO_DESC,
           open_date       = CO_OPEN_DATE,
           co_st_id        = CO_ST_ID,
           co_ad_line1     = CA.AD_LINE1,
           co_ad_line2     = CA.AD_LINE2,
           co_ad_town      = ZCA.ZC_TOWN,
           co_ad_div       = ZCA.ZC_DIV,
           co_ad_zip       = CA.AD_ZC_CODE,
           co_ad_ctry      = CA.AD_CTRY,
           num_out         = S_NUM_OUT,
           start_date      = S_START_DATE,
           exch_date       = S_EXCH_DATE,
           pe_ratio        = S_PE,
           52_wk_high      = S_52WK_HIGH,
           52_wk_high_date = S_52WK_HIGH_DATE,
           52_wk_low       = S_52WK_LOW,
           52_wk_low_date  = S_52WK_LOW_DATE,
           divid           = S_DIVIDEND,
           yield           = S_YIELD,
           ex_ad_div       = ZEA.ZC_DIV,
           ex_ad_ctry      = EA.AD_CTRY
           ex_ad_line1     = EA.AD_LINE1,
           ex_ad_line2     = EA.AD_LINE2,
           ex_ad_town      = ZEA.ZC_TOWN,
           ex_ad_zip       = EA.AD_ZC_CODE,
           ex_close        = EX_CLOSE,
           ex_desc         = EX_DESC,
           ex_name         = EX_NAME,
           ex_num_symb     = EX_NUM_SYMB,
           ex_open         = EX_OPEN
           from
           SECURITY,
           COMPANY,
           ADDRESS CA,
           ADDRESS EA,
           ZIP_CODE ZCA,
           ZIP_CODE ZEA,
           EXCHANGE
           where
           S_SYMB = symbol and
           CO_ID = S_CO_ID and
           CA.AD_ID = CO_AD_ID and
           EA.AD_ID = EX_AD_ID and
           EX_ID = S_EX_ID and
           ca.ad_zc_code = zca.zc_code and
           ea.ad_zc_code =zea.zc_code
        */

        char s_name[71]; //70
        TIdent co_id;
        char co_name[61]; //60
        char sp_rate[5];  //4
        char ceo_name[47]; //46
        char co_desc[151]; //150
        myTime open_date;
        char co_st_id[5]; //4
        char co_ad_line1[81]; //80
        char co_ad_line2[81]; //80
        char co_ad_town[81]; //80
        char co_ad_div[81]; //80
        char co_ad_zip[13]; //12
        char co_ad_ctry[81]; //80
        double num_out;
        myTime start_date;
        myTime exch_date;
        double pe_ratio;
        double S52_wk_high;
        myTime S52_wk_high_date;
        double S52_wk_low;
        myTime S52_wk_low_date;
        double divid;
        double yield;
        char ex_ad_div[81]; //80
        char ex_ad_ctry[81]; //80
        char ex_ad_line1[81]; //80
        char ex_ad_line2[81]; //80
        char ex_ad_town[81]; //80
        char ex_ad_zip[13]; //12
        int ex_close;
        char ex_desc[151]; //151
        char ex_name[101]; //100
        int ex_num_symb;
        int ex_open;

        TIdent s_co_id;
        char s_ex_id[7]; //6

        TRACE( TRACE_TRX_FLOW, "App: %d SD:s-idx-probe (%s) \n", xct_id, psdin._symbol);
        e =  _psecurity_man->s_index_probe(_pssm, prsecurity, psdin._symbol);
        if(e.is_error()) { goto done; }

        prsecurity->get_value(5, s_co_id);
        prsecurity->get_value(4, s_ex_id, 7);

        prsecurity->get_value(3, s_name, 71);
        prsecurity->get_value(6, num_out);
        prsecurity->get_value(7, start_date);
        prsecurity->get_value(8, exch_date);
        prsecurity->get_value(9, pe_ratio);
        prsecurity->get_value(10, S52_wk_high);
        prsecurity->get_value(11, S52_wk_high_date);
        prsecurity->get_value(12, S52_wk_low);
        prsecurity->get_value(13, S52_wk_low_date);
        prsecurity->get_value(14, divid);
        prsecurity->get_value(15, yield);


        TRACE( TRACE_TRX_FLOW, "App: %d SD:co-idx-probe (%ld) \n", xct_id,  s_co_id);
        e =  _pcompany_man->co_index_probe(_pssm, prcompany, s_co_id);
        if(e.is_error()) { goto done; }

        //get company address and zip code
        TIdent co_ad_id;
        prcompany->get_value(6, co_ad_id);

        TRACE( TRACE_TRX_FLOW, "App: %d SD:ad-idx-probe (%ld) \n", xct_id,  co_ad_id);
        e =  _paddress_man->ad_index_probe(_pssm, praddress, co_ad_id);
        if (e.is_error()) { goto done; }

        char ca_ad_zc_code[13]; //12
        praddress->get_value(3, ca_ad_zc_code, 13);

        TRACE( TRACE_TRX_FLOW, "App: %d SD:zc-idx-probe (%s) \n", xct_id,  ca_ad_zc_code);
        e =  _pzip_code_man->zc_index_probe(_pssm, przipcode, ca_ad_zc_code);
        if (e.is_error()) { goto done; }

        prcompany->get_value(0, co_id);
        prcompany->get_value(2, co_name, 61);
        prcompany->get_value(4, sp_rate, 5);
        prcompany->get_value(5, ceo_name, 47);
        prcompany->get_value(7, co_desc, 151);
        prcompany->get_value(8, open_date);
        prcompany->get_value(1, co_st_id, 5);

        praddress->get_value(1, co_ad_line1, 81);
        praddress->get_value(2, co_ad_line2, 81);

        przipcode->get_value(1, co_ad_town, 81);
        przipcode->get_value(2, co_ad_div, 81);

        praddress->get_value(3, co_ad_zip, 13);
        praddress->get_value(4, co_ad_ctry, 81);

        TRACE( TRACE_TRX_FLOW, "App: %d SD:ex-idx-probe (%s) \n", xct_id, s_ex_id);
        e =  _pexchange_man->ex_index_probe(_pssm, prexchange, s_ex_id);
        if(e.is_error()) { goto done; }

        TIdent ex_ad_id;
        prexchange->get_value(6, ex_ad_id);

        //get exchange address and zip code
        TRACE( TRACE_TRX_FLOW, "App: %d SD:ad-idx-probe (%ld) \n", xct_id,  ex_ad_id);
        e =  _paddress_man->ad_index_probe(_pssm, praddress, ex_ad_id);
        if (e.is_error()) { goto done; }

        char ea_ad_zc_code[13]; //12
        praddress->get_value(3, ea_ad_zc_code, 13);

        TRACE( TRACE_TRX_FLOW, "App: %d SD:zc-idx-probe (%s) \n", xct_id,  ea_ad_zc_code);
        e =  _pzip_code_man->zc_index_probe(_pssm, przipcode, ea_ad_zc_code);
        if (e.is_error()) { goto done; }

        przipcode->get_value(2, ex_ad_div, 81);

        praddress->get_value(4, ex_ad_ctry, 81);
        praddress->get_value(1, ex_ad_line1, 81);
        praddress->get_value(2, ex_ad_line2, 81);
        przipcode->get_value(1, ex_ad_town, 81);

        praddress->get_value(3, ex_ad_zip, 13);

        prexchange->get_value(4, ex_close);
        prexchange->get_value(5, ex_desc, 151);
        prexchange->get_value(1, ex_name, 101);
        prexchange->get_value(2, ex_num_symb);
        prexchange->get_value(3, ex_open);

        /**
           select first max_comp_len rows
           cp_co_name[] = CO_NAME,
           cp_in_name[] = IN_NAME
           from
           COMPANY_COMPETITOR, COMPANY, INDUSTRY
           where
           CP_CO_ID = co_id and
           CO_ID = CP_COMP_CO_ID and
           IN_ID = CP_IN_ID
        */
        char cp_co_name[3][61]; //60
        char in_name[3][51]; //50

        guard< index_scan_iter_impl<company_competitor_t> > cc_iter;
        {
            index_scan_iter_impl<company_competitor_t>* tmp_cc_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d SD:cc-get-iter-by-index2 (%ld) \n", xct_id,  co_id);
            e = _pcompany_competitor_man->cc_get_iter_by_index2(_pssm, tmp_cc_iter, prcompanycomp, lowrep, highrep, co_id);
            if (e.is_error()) { goto done; }
            cc_iter = tmp_cc_iter;
        }

        bool eof;
        TRACE( TRACE_TRX_FLOW, "App: %d SD:cc-iter-next \n", xct_id);
        e = cc_iter->next(_pssm, eof, *prcompanycomp);
        if (e.is_error()) { goto done; }
        for(int i = 0; i < 3 && !eof; i++){
            TIdent cp_comp_co_id;
            char cp_in_id[3]; //2

            prcompanycomp->get_value(2, cp_in_id, 3);
            prcompanycomp->get_value(1, cp_comp_co_id);

            TRACE( TRACE_TRX_FLOW, "App: %d SD:in-idx-probe (%s) \n", xct_id,  cp_in_id);
            e =  _pindustry_man->in_index_probe(_pssm, prindustry, cp_in_id);
            if(e.is_error()) { goto done; }

            prindustry->get_value(1, in_name[i], 51);

            TRACE( TRACE_TRX_FLOW, "App: %d SD:co-idx-probe (%ld) \n", xct_id,  cp_comp_co_id);
            e =  _pcompany_man->co_index_probe(_pssm, prcompany, cp_comp_co_id);
            if(e.is_error()) { goto done; }

            prcompany->get_value(2, cp_co_name[i], 61);

            TRACE( TRACE_TRX_FLOW, "App: %d SD:cc-iter-next \n", xct_id);
            e = cc_iter->next(_pssm, eof, *prcompanycomp);
            if (e.is_error()) { goto done; }
        }

        /**
           select first max_fin_len rows
           fin[].year        = FI_YEAR,
           fin[].qtr         = FI_QTR,
           fin[].strart_date = FI_QTR_START_DATE,
           fin[].rev         = FI_REVENUE,
           fin[].net_earn    = FI_NET_EARN,
           fin[].basic_eps   = FI_BASIC_EPS,
           fin[].dilut_eps   = FI_DILUT_EPS,
           fin[].margin      = FI_MARGIN,
           fin[].invent      = FI_INVENTORY,
           fin[].assets      = FI_ASSETS,
           fin[].liab        = FI_LIABILITY,
           fin[].out_basic   = FI_OUT_BASIC,
           fin[].out_dilut   = FI_OUT_DILUT
           from
           FINANCIAL
           where
           FI_CO_ID = co_id
           order by
           FI_YEAR asc,
           FI_QTR
        */
        int fin_year[20];
        int fin_qtr[20];
        myTime fin_start_date[20];
        double fin_rev[20];
        int fin_net_earn[20];
        int fin_basic_eps[20];
        double fin_dilut_eps[20];
        double fin_margin[20];
        double fin_invent[20];
        double fin_assets[20];
        double fin_liab[20];
        double fin_out_basic[20];
        double fin_out_dilut[20];

        guard< index_scan_iter_impl<financial_t> > fi_iter;
        {
            index_scan_iter_impl<financial_t>* tmp_fi_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d SD:fi-get-iter-by-idx (%ld) \n", xct_id,  co_id);
            e = _pfinancial_man->fi_get_iter_by_index(_pssm, tmp_fi_iter, prfinancial, lowrep, highrep, co_id);
            if (e.is_error()) { goto done; }
            fi_iter = tmp_fi_iter;
        }
        //already sorted because of the index

        TRACE( TRACE_TRX_FLOW, "App: %d SD:fi-iter-next \n", xct_id);
        e = fi_iter->next(_pssm, eof, *prfinancial);
        if (e.is_error()) { goto done; }
        int i;
        for(i = 0; i < 20 && !eof; i++){
            int fi_year;
            prfinancial->get_value(1, fi_year);
            fin_year[i] = fi_year;

            short fi_qtr;
            prfinancial->get_value(2, fi_qtr);
            fin_qtr[i] = fi_qtr;

            myTime fi_qtr_start_date;
            prfinancial->get_value(3, fi_qtr_start_date);
            fin_start_date[i] = fi_qtr_start_date;

            double fi_revenue;
            prfinancial->get_value(4, fi_revenue);
            fin_rev[i] = fi_revenue;

            double fi_net_earn;
            prfinancial->get_value(5, fi_net_earn);
            fin_net_earn[i] = fi_net_earn;

            double fi_basic_eps;
            prfinancial->get_value(6, fi_basic_eps);
            fin_basic_eps[i] = fi_basic_eps;

            double fi_dilut_eps;
            prfinancial->get_value(7, fi_dilut_eps);
            fin_dilut_eps[i] = fi_dilut_eps;

            double fi_margin;
            prfinancial->get_value(8, fi_margin);
            fin_margin[i] = fi_margin;

            double fi_inventory;
            prfinancial->get_value(9, fi_inventory);
            fin_invent[i] = fi_inventory;

            double fi_assets;
            prfinancial->get_value(10, fi_assets);
            fin_assets[i] = fi_assets;

            double fi_liability;
            prfinancial->get_value(11, fi_liability);
            fin_liab[i] = fi_liability;

            double fi_out_basics;
            prfinancial->get_value(12, fi_out_basics);
            fin_out_basic[i] = fi_out_basics;

            double fi_out_dilut;
            prfinancial->get_value(13, fi_out_dilut);
            fin_out_dilut[i] = fi_out_dilut;

            TRACE( TRACE_TRX_FLOW, "App: %d SD:fi-iter-next \n", xct_id);
            e = fi_iter->next(_pssm, eof, *prfinancial);
            if (e.is_error()) { goto done; }
        }

        int fin_len = i;

        assert(fin_len == max_fin_len); //Harness control

        /**
           select first max_rows_to_return rows
           day[].date  = DM_DATE,
           day[].close = DM_CLOSE,
           day[].high  = DM_HIGH,
           day[].low   = DM_LOW,
           day[].vol   = DM_VOL
           from
           DAILY_MARKET
           where
           DM_S_SYMB = symbol and
           DM_DATE >= start_day
           order by
           DM_DATE asc

           day_len = row_count
        */

        array_guard_t<myTime> day_date = new myTime[psdin._max_rows_to_return];
        array_guard_t<double> day_close = new double[psdin._max_rows_to_return];
        array_guard_t<double> day_high = new double[psdin._max_rows_to_return];
        array_guard_t<double> day_low = new double[psdin._max_rows_to_return];
        array_guard_t<int> day_vol = new int[psdin._max_rows_to_return];

        guard< index_scan_iter_impl<daily_market_t> > dm_iter;
        {
            index_scan_iter_impl<daily_market_t>* tmp_dm_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d SD:dm-get-iter-by-idx (%s) (%ld) \n", xct_id, psdin._symbol, psdin._start_day);
            e = _pdaily_market_man->dm_get_iter_by_index(_pssm, tmp_dm_iter, prdailymarket,
                                                         lowrep, highrep, psdin._symbol, psdin._start_day);
            if (e.is_error()) { goto done; }
            dm_iter = tmp_dm_iter;
        }

        TRACE( TRACE_TRX_FLOW, "App: %d SD:dm-iter-next \n", xct_id);
        e = dm_iter->next(_pssm, eof, *prdailymarket);
        if (e.is_error()) { goto done; }
        i = 0;
        while(i < psdin._max_rows_to_return && !eof){
            char dm_s_symb[16];
            prdailymarket->get_value(1, dm_s_symb, 16);
			
            if(strcmp(dm_s_symb, psdin._symbol) == 0){		  
                prdailymarket->get_value(0, day_date[i]);
                prdailymarket->get_value(2, day_close[i]);
                prdailymarket->get_value(3, day_high[i]);
                prdailymarket->get_value(4, day_low[i]);
                prdailymarket->get_value(5, day_vol[i]);
                i++;
            }		

            //TRACE( TRACE_TRX_FLOW, "App: %d SD:dm-iter-next \n", xct_id);
            e = dm_iter->next(_pssm, eof, *prdailymarket);
            if (e.is_error()) { goto done; }
        }

        int day_len = i;
        //assert(day_len >= min_day_len && day_len <= max_day_len); //Harness control

        /**
           select
           last_price = LT_PRICE,
           last_open  = LT_OPEN_PRICE,
           last_vol   = LT_VOL
           from
           LAST_TRADE
           where
           LT_S_SYMB = symbol
        */

        TRACE( TRACE_TRX_FLOW, "App: %d SD:lt-idx-probe (%s) \n", xct_id, psdin._symbol);
        e =  _plast_trade_man->lt_index_probe(_pssm, prlasttrade, psdin._symbol);
        if(e.is_error()) { goto done; }

        double last_price, last_open, last_vol;
        prlasttrade->get_value(2, last_price);
        prlasttrade->get_value(3, last_open);
        prlasttrade->get_value(4, last_vol);

        char news_item[2][max_news_item_size+1]; //10000
        myTime news_dts[2];
        char news_src[2][31]; //30
        char news_auth[2][31]; //30
        char news_headline[2][81]; //80
        char news_summary[2][256]; //255

        if(psdin._access_lob_flag){
            /**
               select first max_news_len rows
               news[].item     = NI_ITEM,
               news[].dts      = NI_DTS,
               news[].src      = NI_SOURCE,
               news[].auth     = NI_AUTHOR,
               news[].headline = “”,
               news[].summary  = “”
               from
               NEWS_XREF,
               NEWS_ITEM
               where
               NI_ID = NX_NI_ID and
               NX_CO_ID = co_id
            */

            guard< index_scan_iter_impl<news_xref_t> > nx_iter;
            {
                index_scan_iter_impl<news_xref_t>* tmp_nx_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d SD:nx-get-iter-by-idx (%ld) \n", xct_id, co_id);
                e = _pnews_xref_man->nx_get_iter_by_index(_pssm, tmp_nx_iter, prnewsxref, lowrep, highrep, co_id);
                if (e.is_error()) { goto done; }
                nx_iter = tmp_nx_iter;
            }

            TRACE( TRACE_TRX_FLOW, "App: %d SD:nx-iter-next \n", xct_id);
            e = nx_iter->next(_pssm, eof, *prnewsxref);
            if (e.is_error()) { goto done; }
            for(i = 0; i < 2 && !eof; i++){
                TIdent nx_ni_id;
                prnewsxref->get_value(0, nx_ni_id);

                TRACE( TRACE_TRX_FLOW, "App: %d SD:nx-idx-probe (%ld) \n", xct_id, nx_ni_id);
                e =  _pnews_item_man->ni_index_probe(_pssm, prnewsitem, nx_ni_id);
                if (e.is_error()) { goto done; }

                prnewsitem->get_value(3, news_item[i], max_news_item_size+1);
                prnewsitem->get_value(4, news_dts[i]);
                prnewsitem->get_value(5, news_src[i], 31);
                prnewsitem->get_value(6, news_auth[i], 31);
                strcpy(news_headline[i], "");
                strcpy(news_summary[i], "");

                TRACE( TRACE_TRX_FLOW, "App: %d SD:nx-iter-next \n", xct_id);
                e = nx_iter->next(_pssm, eof, *prnewsxref);
                if (e.is_error()) { goto done; }
            }
        }
        else{
            /**
               select first max_news_len rows
               news[].item     = “”,
               news[].dts      = NI_DTS,
               news[].src      = NI_SOURCE,
               news[].auth     = NI_AUTHOR,
               news[].headline = NI_HEADLINE,
               news[].summary  = NI_SUMMARY
               from
               NEWS_XREF,
               NEWS_ITEM
               where
               NI_ID = NX_NI_ID and
               NX_CO_ID = co_id
            */

            guard< index_scan_iter_impl<news_xref_t> > nx_iter;
            {
                index_scan_iter_impl<news_xref_t>* tmp_nx_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d SD:nx-get-iter-by-idx (%ld) \n", xct_id, co_id);
                e = _pnews_xref_man->nx_get_iter_by_index(_pssm, tmp_nx_iter, prnewsxref, lowrep, highrep, co_id);
                if (e.is_error()) { goto done; }
                nx_iter = tmp_nx_iter;
            }

            TRACE( TRACE_TRX_FLOW, "App: %d SD:nx-iter-next \n", xct_id);
            e = nx_iter->next(_pssm, eof, *prnewsxref);
            if (e.is_error()) { goto done; }
            for(i = 0; i < 2 && !eof; i++){
                TIdent nx_ni_id;
                prnewsxref->get_value(0, nx_ni_id);

                TRACE( TRACE_TRX_FLOW, "App: %d SD:nx-idx-probe (%ld) \n", xct_id, nx_ni_id);
                e =  _pnews_item_man->ni_index_probe(_pssm, prnewsitem, nx_ni_id);
                if (e.is_error()) { goto done; }

                strcpy(news_item[i], "");
                prnewsitem->get_value(4, news_dts[i]);
                prnewsitem->get_value(5, news_src[i], 31);
                prnewsitem->get_value(6, news_auth[i], 31);
                prnewsitem->get_value(1, news_headline[i], 81);
                prnewsitem->get_value(2, news_summary[i], 256);

                TRACE( TRACE_TRX_FLOW, "App: %d SD:nx-iter-next \n", xct_id);
                e = nx_iter->next(_pssm, eof, *prnewsxref);
                if (e.is_error()) { goto done; }
            }
        }
        int news_len = i;

        assert(news_len == max_news_len); //Harness control
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used

    raddress.print_tuple();
    rcompany.print_tuple();
    rcompanycomp.print_tuple();
    rdailymarket.print_tuple();
    rexchange.print_tuple();
    rfinancial.print_tuple();
    rindustry.print_tuple();
    rlasttrade.print_tuple();
    rnewsitem.print_tuple();
    rnewsxref.print_tuple();
    rsecurity.print_tuple();
    rzipcode.print_tuple();

#endif

 done:
    // return the tuples to the cache

    _paddress_man->give_tuple(praddress);
    _pcompany_man->give_tuple(prcompany);
    _pcompany_competitor_man->give_tuple(prcompanycomp);
    _pdaily_market_man->give_tuple(prdailymarket);
    _pexchange_man->give_tuple(prexchange);
    _pfinancial_man->give_tuple(prfinancial);
    _pindustry_man->give_tuple(prindustry);
    _plast_trade_man->give_tuple(prlasttrade);
    _pnews_item_man->give_tuple(prnewsitem);
    _pnews_xref_man->give_tuple(prnewsxref);
    _psecurity_man->give_tuple(prsecurity);
    _pzip_code_man->give_tuple(przipcode);
    //
    return (e);
}

//TRADE_STATUS

w_rc_t ShoreTPCEEnv::xct_trade_status(const int xct_id, trade_status_input_t& ptsin)
{
    //ptsin.print();
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prbroker = _pbroker_man->get_tuple();
    assert (prbroker);

    table_row_t* prcustomer = _pcustomer_man->get_tuple(); //280
    assert (prcustomer);

    table_row_t* prcustacct = _pcustomer_account_man->get_tuple();
    assert (prcustacct);

    table_row_t* prexchange = _pexchange_man->get_tuple(); //291B
    assert (prexchange);

    table_row_t* prsecurity = _psecurity_man->get_tuple();
    assert (prsecurity);

    table_row_t* prstatustype = _pstatus_type_man->get_tuple();
    assert (prstatustype);

    table_row_t* prtrade= _ptrade_man->get_tuple();
    assert (prtrade);

    table_row_t* prtradetype = _ptrade_type_man->get_tuple();
    assert (prtradetype);

    w_rc_t e = RCOK;
    rep_row_t areprow(_ptrade_man->ts());
    areprow.set(_ptrade_desc->maxsize());

    prbroker->_rep = &areprow;
    prcustomer->_rep = &areprow;
    prcustacct->_rep = &areprow;
    prexchange->_rep = &areprow;
    prsecurity->_rep = &areprow;
    prstatustype->_rep = &areprow;
    prtrade->_rep = &areprow;
    prtradetype->_rep = &areprow;

    rep_row_t lowrep( _pexchange_man->ts());
    rep_row_t highrep( _pexchange_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_pexchange_desc->maxsize());
    highrep.set(_pexchange_desc->maxsize());

    //trial part
    {
        /*
          char temp[16] = "ARDNA";
          TRACE( TRACE_TRX_FLOW, "App: %d TS:s-idx-probe (%s) \n", xct_id, temp);
          e =  _psecurity_man->s_index_probe(_pssm, prsecurity, temp);
          if(e.is_error()) { goto done; }

          char temp2[16] = "AMEX";
          TRACE( TRACE_TRX_FLOW, "App: %d TS:ex-idx-probe (%s) \n", xct_id, temp2);
          e =  _pexchange_man->ex_index_probe(_pssm, prexchange, temp2);
          if(e.is_error()) { goto done; }
        */
    }
    {
        TIdent trade_id[50];
        myTime trade_dts[50];
        char status_name[50][11]; //10
        char type_name[50][13]; //12
        char symbol[50][16]; //15
        int trade_qty[50];
        char exec_name[50][50]; //49
        double charge[50];
        char s_name[50][71]; //70
        char ex_name[50][101]; //100

        /**
           select first 50 row
           trade_id[]    = T_ID,
           trade_dts[]   = T_DTS,
           status_name[] = ST_NAME,
           type_name[]   = TT_NAME,
           symbol[]      = T_S_SYMB,
           trade_qty[]   = T_QTY,
           exec_name[]   = T_EXEC_NAME,
           charge[]      = T_CHRG,
           s_name[]      = S_NAME,
           ex_name[]     = EX_NAME
           from
           TRADE,
           STATUS_TYPE,
           TRADE_TYPE,
           SECURITY,
           EXCHANGE
           where
           T_CA_ID = acct_id and
           ST_ID = T_ST_ID and
           TT_ID = T_TT_ID and
           S_SYMB = T_S_SYMB and
           EX_ID = S_EX_ID
           order by
           T_DTS desc
        */

        //descending order
        rep_row_t sortrep(_pexchange_man->ts());
        sortrep.set(_pexchange_desc->maxsize());
        sort_buffer_t t_list(10);

        t_list.setup(0, SQL_LONG);
        t_list.setup(1, SQL_LONG);
        t_list.setup(2, SQL_FIXCHAR, 10);
        t_list.setup(3, SQL_FIXCHAR, 12);
        t_list.setup(4, SQL_FIXCHAR, 15);
        t_list.setup(5, SQL_INT);
        t_list.setup(6, SQL_FIXCHAR, 49);
        t_list.setup(7, SQL_FLOAT);
        t_list.setup(8, SQL_FIXCHAR, 70);
        t_list.setup(9, SQL_FIXCHAR, 100);

        table_row_t rsb(&t_list);
        sort_man_impl t_sorter(&t_list, &sortrep);
        guard< index_scan_iter_impl<trade_t> > t_iter;
        {
            index_scan_iter_impl<trade_t>* tmp_t_iter;
            TRACE( TRACE_TRX_FLOW, "App: %d TS:t-iter-by-idx2 (%ld) (%ld) (%ld) \n", xct_id, ptsin._acct_id, 0, MAX_DTS);
            e = _ptrade_man->t_get_iter_by_index2 (_pssm, tmp_t_iter, prtrade, lowrep, highrep, ptsin._acct_id, 0, MAX_DTS);
            if (e.is_error()) { goto done; }
            t_iter = tmp_t_iter;
        }
        bool eof;
        TRACE( TRACE_TRX_FLOW, "App: %d TS:t-iter-next \n", xct_id);
        e = t_iter->next(_pssm, eof, *prtrade);
        if (e.is_error()) { goto done; }
        while(!eof){
            char t_st_id[5], t_tt_id[4], t_s_symb[16]; //4, 3, 15

            prtrade->get_value(2, t_st_id, 5);
            prtrade->get_value(3, t_tt_id, 4);
            prtrade->get_value(5, t_s_symb, 16);

            TRACE( TRACE_TRX_FLOW, "App: %d TS:st-idx-probe (%s) \n", xct_id,  t_st_id);
            e =  _pstatus_type_man->st_index_probe(_pssm, prstatustype, t_st_id);
            if (e.is_error()) { goto done; }

            TRACE( TRACE_TRX_FLOW, "App: %d TS:tt-idx-probe (%s) \n", xct_id, t_tt_id);
            e =  _ptrade_type_man->tt_index_probe(_pssm, prtradetype, t_tt_id);
            if (e.is_error()) { goto done; }
			
            TRACE( TRACE_TRX_FLOW, "App: %d TS:s-idx-probe (%s) \n", xct_id, t_s_symb);
            e =  _psecurity_man->s_index_probe(_pssm, prsecurity, t_s_symb);
            if(e.is_error()) { goto done; }

            char s_ex_id[7]; //6
            prsecurity->get_value(4, s_ex_id, 7);

            TRACE( TRACE_TRX_FLOW, "App: %d TS:ex-idx-probe (%s) \n", xct_id, s_ex_id);
            e =  _pexchange_man->ex_index_probe(_pssm, prexchange, s_ex_id);
            if(e.is_error()) { goto done; }

            myTime t_dts;
            prtrade->get_value(1, t_dts);
            rsb.set_value(0, t_dts);
			
            TIdent t_id;
            prtrade->get_value(0, t_id);
            rsb.set_value(1, t_id);

            char st_name[11]; //10
            prstatustype->get_value(1, st_name, 11);
            rsb.set_value(2, st_name);

            char tt_name[13]; //12
            prstatustype->get_value(1, tt_name, 13);
            rsb.set_value(3, tt_name);

            char t_s_symbol[16]; //15
            prtrade->get_value(5, t_s_symbol, 16);
            rsb.set_value(4, t_s_symbol);

            int t_qty;
            prtrade->get_value(6, t_qty);
            rsb.set_value(5, t_qty);

            char t_exec_name[50]; //49
            prtrade->get_value(9, t_exec_name, 50);
            rsb.set_value(6, t_exec_name);

            double t_chrg;
            prtrade->get_value(11, t_chrg);
            rsb.set_value(7, t_chrg);

            char s_name[71]; //70
            prsecurity->get_value(3, s_name, 71);
            rsb.set_value(8, s_name);

            char ex_name[101]; //100
            prexchange->get_value(1, ex_name, 101);
            rsb.set_value(9, ex_name);

            t_sorter.add_tuple(rsb);

            TRACE( TRACE_TRX_FLOW, "App: %d TS:t-iter-next \n", xct_id);
            e = t_iter->next(_pssm, eof, *prtrade);
            if (e.is_error()) { goto done; }
        }
        assert (t_sorter.count());

        sort_iter_impl t_list_sort_iter(_pssm, &t_list, &t_sorter);
        TRACE( TRACE_TRX_FLOW, "App: %d TS:t-sort-iter-next \n", xct_id);
        e = t_list_sort_iter.next(_pssm, eof, rsb);
        if (e.is_error()) { goto done; }
        int i = 0;
        for(; i < max_trade_status_len && !eof; i++){
            rsb.get_value(0, trade_dts[i]);
            rsb.get_value(1, trade_id[i]);
            rsb.get_value(2, status_name[i], 11);
            rsb.get_value(3, type_name[i], 13);
            rsb.get_value(4, symbol[i], 16);
            rsb.get_value(5, trade_qty[i]);
            rsb.get_value(6, exec_name[i], 50);
            rsb.get_value(7, charge[i]);
            rsb.get_value(8, s_name[i], 71);
            rsb.get_value(9, ex_name[i], 101);

            TRACE( TRACE_TRX_FLOW, "App: %d TS:t-sort-iter-next \n", xct_id);
            e = t_list_sort_iter.next(_pssm, eof, rsb);
            if (e.is_error()) { goto done; }
        }

        
        assert(i == max_trade_status_len); //Harness control		

        /**
           select
           cust_l_name = C_L_NAME,
           cust_f_name = C_F_NAME,
           broker_name = B_NAME
           from
           CUSTOMER_ACCOUNT,
           CUSTOMER,
           BROKER
           where
           CA_ID = acct_id and
           C_ID = CA_C_ID and
           B_ID = CA_B_ID
        */

        TRACE( TRACE_TRX_FLOW, "App: %d TS:ca-idx-probe (%ld) \n", xct_id,  ptsin._acct_id);
        e =  _pcustomer_account_man->ca_index_probe(_pssm, prcustacct, ptsin._acct_id);
        if (e.is_error()) { goto done; }

        TIdent ca_c_id, ca_b_id;

        prcustacct->get_value(2, ca_c_id);
        prcustacct->get_value(1, ca_b_id);

        TRACE( TRACE_TRX_FLOW, "App: %d TS:b-idx-probe (%ld) \n", xct_id, ca_b_id);
        e =  _pbroker_man->b_index_probe(_pssm, prbroker, ca_b_id);
        if (e.is_error()) { goto done; }

        TRACE( TRACE_TRX_FLOW, "App: %d TS:c-idx-probe (%ld) \n", xct_id,  ca_c_id);
        e =  _pcustomer_man->c_index_probe(_pssm, prcustomer, ca_c_id);
        if (e.is_error()) { goto done; }

        char cust_l_name[26]; //25
        char cust_f_name[21]; //20
        char broker_name[50]; //49

        prcustomer->get_value(3, cust_l_name, 26);
        prcustomer->get_value(4, cust_f_name, 21);
        prbroker->get_value(2, broker_name, 50);
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rbroker.print_tuple();
    rcustacct.print_tuple();
    rcustomer.print_tuple();
    rexchange.print_tuple();
    rsecurity.print_tuple();
    rstatustype.print_tuple();
    rtrade.print_tuple();
    rtradetype.print_tuple();

#endif

 done:
    // return the tuples to the cache

    _pbroker_man->give_tuple(prbroker);
    _pcustomer_man->give_tuple(prcustomer);
    _pcustomer_account_man->give_tuple(prcustacct);
    _pexchange_man->give_tuple(prexchange);
    _psecurity_man->give_tuple(prsecurity);
    _pstatus_type_man->give_tuple(prstatustype);
    _ptrade_man->give_tuple(prtrade);
    _ptrade_type_man->give_tuple(prtradetype);

    return (e);
}

//BROKER_VOLUME
w_rc_t ShoreTPCEEnv::xct_broker_volume(const int xct_id, broker_volume_input_t& pbvin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prbroker = _pbroker_man->get_tuple();
    assert (prbroker);

    table_row_t* prcompany = _pcompany_man->get_tuple();
    assert (prcompany);

    table_row_t* prindustry = _pindustry_man->get_tuple();
    assert (prindustry);

    table_row_t* prsector = _psector_man->get_tuple();
    assert (prsector);

    table_row_t* prsecurity = _psecurity_man->get_tuple();
    assert (prsecurity);

    table_row_t* prtradereq = _ptrade_request_man->get_tuple();
    assert (prtradereq);

    w_rc_t e = RCOK;
    rep_row_t areprow(_pcompany_man->ts());
    areprow.set(_pcompany_desc->maxsize());

    prbroker->_rep = &areprow;
    prcompany->_rep = &areprow;
    prindustry->_rep = &areprow;
    prsector->_rep = &areprow;
    prsecurity->_rep = &areprow;
    prtradereq->_rep = &areprow;

    rep_row_t lowrep( _pcompany_man->ts());
    rep_row_t highrep( _pcompany_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_pcompany_desc->maxsize());
    highrep.set(_pcompany_desc->maxsize());
    {
        /**
           select
           broker_name[] = B_NAME,
           volume[]      = sum(TR_QTY * TR_BID_PRICE)
           from
           TRADE_REQUEST,
           SECTOR,
           INDUSTRY
           COMPANY,
           BROKER,
           SECURITY
           where
           TR_B_ID = B_ID and
           TR_S_SYMB = S_SYMB and
           S_CO_ID = CO_ID and
           CO_IN_ID = IN_ID and
           SC_ID = IN_SC_ID and
           B_NAME in (broker_list) and
           SC_NAME = sector_name
           group by
           B_NAME
           order by
           2 DESC
        */

        //descending order
        rep_row_t sortrep(_pcompany_man->ts());
        sortrep.set(_pcompany_desc->maxsize());

        sort_buffer_t tr_list(2);
        tr_list.setup(0, SQL_FLOAT);
        tr_list.setup(1, SQL_FIXCHAR);

        table_row_t rsb(&tr_list);
        sort_man_impl tr_sorter(&tr_list, &sortrep);
        
        unsigned int i;
        unsigned int size = 0;

        for(i = 0; strcmp(pbvin._broker_list[i], "\0") != 0; i++){
          size++;
        }

        for(i = 0; i < size ; i++){
            guard< index_scan_iter_impl<broker_t> > b_iter;
            {
                index_scan_iter_impl<broker_t>* tmp_b_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d BV:b-get-iter-by-idx2 (%s) \n", xct_id,  pbvin._broker_list[i]);
                e = _pbroker_man->b_get_iter_by_index2(_pssm, tmp_b_iter, prbroker, lowrep, highrep, pbvin._broker_list[i]);
                if (e.is_error()) { goto done; }
                b_iter = tmp_b_iter;
            }

            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d BV:b-iter-next \n", xct_id);
            e = b_iter->next(_pssm, eof, *prbroker);
            if (e.is_error()) { goto done; }

            TIdent b_id;
            prbroker->get_value(0, b_id);

            guard< index_scan_iter_impl<trade_request_t> > tr_iter;
            {
                index_scan_iter_impl<trade_request_t>* tmp_tr_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d BV:tr-get-iter-by-idx (%ld) \n", xct_id,  b_id);
                e = _ptrade_request_man->tr_get_iter_by_index2(_pssm, tmp_tr_iter, prtradereq, lowrep, highrep, b_id);
                if (e.is_error()) { goto done; }
                tr_iter = tmp_tr_iter;
            }

            TRACE( TRACE_TRX_FLOW, "App: %d BV:tr-iter-next \n", xct_id);
            e = tr_iter->next(_pssm, eof, *prtradereq);
            if (e.is_error()) { goto done; }
            double volume = 0;
            while(!eof){
                char tr_s_symb[16]; //15
                prtradereq->get_value(2, tr_s_symb, 16);

                TRACE( TRACE_TRX_FLOW, "App: %d BV:s-idx-probe (%s) \n", xct_id, tr_s_symb);
                e =  _psecurity_man->s_index_probe(_pssm, prsecurity, tr_s_symb);
                if(e.is_error()) { goto done; }

                TIdent s_co_id;
                prsecurity->get_value(5, s_co_id);

                TRACE( TRACE_TRX_FLOW, "App: %d BV:co-idx-probe (%ld) \n", xct_id,  s_co_id);
                e =  _pcompany_man->co_index_probe(_pssm, prcompany, s_co_id);
                if(e.is_error()) { goto done; }

                char co_in_id[3]; //2
                prcompany->get_value(3, co_in_id, 3);

                TRACE( TRACE_TRX_FLOW, "App: %d BV:in-idx-probe (%s) \n", xct_id,  co_in_id);
                e =  _pindustry_man->in_index_probe(_pssm, prindustry, co_in_id);
                if(e.is_error()) { goto done; }

                char in_sc_id[3]; //2
                prindustry->get_value(2, in_sc_id, 3);

                TRACE( TRACE_TRX_FLOW, "App: %d BV:sc-idx-probe (%s) \n", xct_id,  in_sc_id);
                e =  _psector_man->sc_index_probe(_pssm, prsector, in_sc_id);
                if(e.is_error()) { goto done; }

                char sc_name[31]; //30
                prsector->get_value(1, sc_name, 31);

                if(strcmp(sc_name, pbvin._sector_name) == 0){
                    int tr_qty;
                    double tr_bid_price;

                    prtradereq->get_value(3, tr_qty);
                    prtradereq->get_value(4, tr_bid_price);

                    volume += (tr_qty * tr_bid_price);
                }

                TRACE( TRACE_TRX_FLOW, "App: %d BV:tr-iter-next \n", xct_id);
                e = tr_iter->next(_pssm, eof, *prtradereq);
                if (e.is_error()) { goto done; }
            }
            if(volume != 0){
                rsb.set_value(0, volume);
                rsb.set_value(1, pbvin._broker_list[i]);

                tr_sorter.add_tuple(rsb);
            }
        }
        //assert (tr_sorter.count()); //harness control

        array_guard_t< char[50] > broker_name = new char[size][50]; //49
        array_guard_t<double> volume = new double[size];

        sort_iter_impl tr_list_sort_iter(_pssm, &tr_list, &tr_sorter);
        bool eof;
        e = tr_list_sort_iter.next(_pssm, eof, rsb);
        if (e.is_error()) { goto done; }
        for(int j = 0; j < i && !eof; j++){
            rsb.get_value(0, volume[j]);
            rsb.get_value(1, broker_name[j], 50);

            e = tr_list_sort_iter.next(_pssm, eof, rsb);
            if (e.is_error()) { goto done; }
        }
    }

#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rbroker.print_tuple();
    rcompany.print_tuple();
    rindustry.print_tuple();
    rsector.print_tuple();
    rsecurity.print_tuple();
    rtradereq.print_tuple();

#endif

 done:
    // return the tuples to the cache

    _pbroker_man->give_tuple(prbroker);
    _pcompany_man->give_tuple(prcompany);
    _pindustry_man->give_tuple(prindustry);
    _psector_man->give_tuple(prsector);
    _psecurity_man->give_tuple(prsecurity);
    _ptrade_request_man->give_tuple(prtradereq);

    return (e);
}

//TRADE_UPDATE
w_rc_t ShoreTPCEEnv::xct_trade_update(const int xct_id, trade_update_input_t& ptuin)
{
    // ensure a valid environment
    assert (_pssm);
    assert (_initialized);
    assert (_loaded);

    table_row_t* prcashtrans = _pcash_transaction_man->get_tuple();
    assert (prcashtrans);

    table_row_t* prsecurity = _psecurity_man->get_tuple();
    assert (prsecurity);

    table_row_t* prsettlement = _psettlement_man->get_tuple();
    assert (prsettlement);

    table_row_t* prtrade = _ptrade_man->get_tuple();
    assert (prtrade);

    table_row_t* prtradetype = _ptrade_type_man->get_tuple();
    assert (prtradetype);

    table_row_t* prtradehist = _ptrade_history_man->get_tuple();
    assert (prtradehist);

    w_rc_t e = RCOK;
    rep_row_t areprow(_psecurity_man->ts());
    areprow.set(_psecurity_desc->maxsize());

    prcashtrans->_rep = &areprow;
    prsecurity->_rep = &areprow;
    prsettlement->_rep = &areprow;
    prtrade->_rep = &areprow;
    prtradetype->_rep = &areprow;
    prtradehist->_rep = &areprow;

    rep_row_t lowrep( _psecurity_man->ts());
    rep_row_t highrep( _psecurity_man->ts());

    // allocate space for the biggest of the table representations
    lowrep.set(_psecurity_desc->maxsize());
    highrep.set(_psecurity_desc->maxsize());

    {
        //ptuin.print();
        unsigned int max_trades = 20; //instead of ptuin._max_trades
        int num_found = 0;
        int num_updated = 0;

        if(ptuin._frame_to_execute == 1){
            int i;
            char ex_name[50]; //49

            array_guard_t<double> bid_price = new double[max_trades];
            array_guard_t< char[50] > exec_name = new char[max_trades][50]; //49
            array_guard_t<bool> is_cash = new bool[max_trades];
            array_guard_t<bool>	is_market = new bool[max_trades];
            array_guard_t<double> trade_price = new double[max_trades];

            array_guard_t<double> settlement_amount = new double[max_trades];
            array_guard_t<myTime> settlement_cash_due_date = new myTime[max_trades];
            array_guard_t< char[41] > settlement_cash_type = new char[max_trades][41]; //40

            array_guard_t<double> cash_transaction_amount = new double[max_trades];
            array_guard_t<myTime> cash_transaction_dts = new myTime[max_trades];
            array_guard_t< char[101] > cash_transaction_name = new char[max_trades][101]; //100

            array_guard_t< myTime[3] > trade_history_dts = new myTime[max_trades][3];
            array_guard_t< char[3][5] > trade_history_status_id = new char[max_trades][3][5]; //4

            for(i = 0; i < max_trades; i++){
                if(num_updated < ptuin._max_updates){
                    /**
                       select
                       ex_name = T_EXEC_NAME
                       from
                       TRADE
                       where
                       T_ID = trade_id[i]
                       num_found = num_found + row_count
                    */

                    TRACE( TRACE_TRX_FLOW, "App: %d TU:t-idx-probe-for-update (%ld) \n", xct_id,  ptuin._trade_id[i]);
                    e =  _ptrade_man->t_index_probe_forupdate(_pssm, prtrade, ptuin._trade_id[i]);
                    if (e.is_error()) { goto done; }

                    prtrade->get_value(9, ex_name, 50);

                    num_found++;

                    string temp_exec_name(ex_name);
                    size_t index = temp_exec_name.find(" X ");
                    if(index != string::npos){
                        temp_exec_name.replace(index, 3, " ");
                    }
                    else{
                        index = temp_exec_name.find(" ");
                        temp_exec_name.replace(index, 1, " X ");
                    }
                    strcpy(ex_name, temp_exec_name.c_str());

                    /**
                       update
                       TRADE
                       set
                       T_EXEC_NAME = ex_name
                       where
                       T_ID = trade_id[i]
                    */
                    TRACE( TRACE_TRX_FLOW, "App: %d TU:t-update-exec-name (%s) \n", xct_id, ex_name);
                    e =  _ptrade_man->t_update_name(_pssm, prtrade, ex_name);
                    if (e.is_error()) { goto done; }

                    num_updated++;
                }

                /**
                   select
                   bid_price[i] = T_BID_PRICE,
                   exec_name[i] = T_EXEC_NAME,
                   is_cash[i]   = T_IS_CASH,
                   is_market[i] = TT_IS_MRKT,
                   trade_price[i] = T_TRADE_PRICE
                   from
                   TRADE,
                   TRADE_TYPE
                   where
                   T_ID = trade_id[i] and
                   T_TT_ID = TT_ID
                */
                TRACE( TRACE_TRX_FLOW, "TL: %d TU:t-idx-probe (%ld) \n", xct_id,  ptuin._trade_id[i]);
                e = _ptrade_man->t_index_probe(_pssm, prtrade, ptuin._trade_id[i]);
                if (e.is_error()) { goto done; }

                char tt_id[4]; //3
                prtrade->get_value(3, tt_id, 4);

                TRACE( TRACE_TRX_FLOW, "App: %d TU:tt-idx-probe (%s) \n", xct_id,  tt_id);
                e = _ptrade_type_man->tt_index_probe(_pssm, prtradetype, tt_id);
                if (e.is_error()) { goto done; }

                prtrade->get_value(7, bid_price[i]);
                prtrade->get_value(9, exec_name[i], 50);
                prtrade->get_value(4, is_cash[i]);
                prtradetype->get_value(3, is_market[i]);
                prtrade->get_value(10, trade_price[i]);

                /**
                   select
                   settlement_amount[i]        = SE_AMT,
                   settlement_cash_due_date[i] = SE_CASH_DUE_DATE,
                   settlement_cash_type[i]     = SE_CASH_TYPE
                   from
                   SETTLEMENT
                   where
                   SE_T_ID = trade_id[i]
                */

                TRACE( TRACE_TRX_FLOW, "App: %d TU:settlement-idx-probe (%ld) \n", xct_id,  ptuin._trade_id[i]);
                e = _psettlement_man->se_index_probe(_pssm, prsettlement, ptuin._trade_id[i]);
                if (e.is_error()) { goto done; }

                prsettlement->get_value(3, settlement_amount[i]);
                prsettlement->get_value(2, settlement_cash_due_date[i]);
                prsettlement->get_value(1, settlement_cash_type[i], 41);

                if(is_cash[i]){
                    /**
                       select
                       cash_transaction_amount[i] = CT_AMT,
                       cash_transaction_dts[i]    = CT_DTS,
                       cash_transaction_name[i]   = CT_NAME
                       from
                       CASH_TRANSACTION
                       where
                       CT_T_ID = trade_id[i]
                    */

                    TRACE( TRACE_TRX_FLOW, "App: %d TU:cash-transaction-idx-probe (%ld) \n", xct_id,  ptuin._trade_id[i]);
                    e = _pcash_transaction_man->ct_index_probe(_pssm, prcashtrans, ptuin._trade_id[i]);
                    if (e.is_error()) { goto done; }

                    prcashtrans->get_value(2, cash_transaction_amount[i]);
                    prcashtrans->get_value(1, cash_transaction_dts[i]);
                    prcashtrans->get_value(3, cash_transaction_name[i], 101);
                }
                /**
                   select first 3 rows
                   trade_history_dts[i][]       = TH_DTS,
                   trade_history_status_id[i][] = TH_ST_ID
                   from
                   TRADE_HISTORY
                   where
                   TH_T_ID = trade_id[i]
                   order by
                   TH_DTS
                */
                guard<index_scan_iter_impl<trade_history_t> > th_iter;
                {
                    index_scan_iter_impl<trade_history_t>* tmp_th_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TU:th-iter-by-trade-idx \n", xct_id);
                    e = _ptrade_history_man->th_get_iter_by_index(_pssm, tmp_th_iter, prtradehist, lowrep, highrep, ptuin._trade_id[i]);
                    if (e.is_error()) { goto done; }
                    th_iter = tmp_th_iter;
                }
                //ascending order
                rep_row_t sortrep(_psecurity_man->ts());
                sortrep.set(_psecurity_desc->maxsize());
                asc_sort_buffer_t th_list(2);

                th_list.setup(0, SQL_LONG);
                th_list.setup(1, SQL_FIXCHAR, 4);
				
                table_row_t rsb(&th_list);
                asc_sort_man_impl th_sorter(&th_list, &sortrep);
                bool eof;
                TRACE( TRACE_TRX_FLOW, "App: %d TU:th-iter-next \n", xct_id);
                e = th_iter->next(_pssm, eof, *prtradehist);
                if (e.is_error()) { goto done; }
                while (!eof) {
                    /* put the value into the sorted buffer */
                    myTime temp_dts;
                    char temp_stid[5]; //4
					
                    prtradehist->get_value(1, temp_dts);
                    prtradehist->get_value(2, temp_stid, 5);
					
                    rsb.set_value(0, temp_dts);
                    rsb.set_value(1, temp_stid);

                    th_sorter.add_tuple(rsb);

                    TRACE( TRACE_TRX_FLOW, "App: %d TU:th-iter-next \n", xct_id);
                    e = th_iter->next(_pssm, eof, *prtradehist);
                    if (e.is_error()) { goto done; }
                }
                assert (th_sorter.count());

                asc_sort_iter_impl th_list_sort_iter(_pssm, &th_list, &th_sorter);
                TRACE( TRACE_TRX_FLOW, "App: %d TU:th-sort-next \n", xct_id);
                e = th_list_sort_iter.next(_pssm, eof, rsb);
                if (e.is_error()) { goto done; }
                for(int j = 0; j < 3 && !eof; j++) {
                    rsb.get_value(0, trade_history_dts[i][j]);
                    rsb.get_value(1, trade_history_status_id[i][j], 5);

                    TRACE( TRACE_TRX_FLOW, "App: %d TU:th-sort-next \n", xct_id);
                    e = th_list_sort_iter.next(_pssm, eof, rsb);
                    if (e.is_error()) { goto done; }
                }
            }
            assert(num_found == max_trades && num_updated == ptuin._max_updates); //Harness control
        }
        else if(ptuin._frame_to_execute == 2){
            int i;

            array_guard_t<double> bid_price = new double[max_trades];
            array_guard_t< char[50] > exec_name = new char[max_trades][50]; //49
            array_guard_t<bool> is_cash = new bool[max_trades];
            array_guard_t<TIdent> trade_list = new TIdent[max_trades];
            array_guard_t<double> trade_price = new double[max_trades];

            array_guard_t<double> settlement_amount = new double[max_trades];
            array_guard_t<myTime> settlement_cash_due_date = new myTime[max_trades];
            array_guard_t< char[41] > settlement_cash_type = new char[max_trades][41]; //40

            array_guard_t<double> cash_transaction_amount = new double[max_trades];
            array_guard_t<myTime> cash_transaction_dts = new myTime[max_trades];
            array_guard_t< char[101] > cash_transaction_name = new char[max_trades][101]; //100

            array_guard_t< myTime[3] > trade_history_dts = new myTime[max_trades][3];
            array_guard_t< char[3][5] > trade_history_status_id = new char[max_trades][3][5]; //4

            /**
               select first max_trades rows
               bid_price[]   = T_BID_PRICE,
               exec_name[]   = T_EXEC_NAME,
               is_cash[]     = T_IS_CASH,
               trade_list[]  = T_ID,
               trade_price[] = T_TRADE_PRICE
               from
               TRADE
               where
               T_CA_ID = acct_id and
               T_DTS >= start_trade_dts and
               T_DTS <= end_trade_dts
               order by
               T_DTS asc
            */

            guard<index_scan_iter_impl<trade_t> > t_iter;
            {
                index_scan_iter_impl<trade_t>* tmp_t_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-by-idx2 %ld %ld %ld \n", xct_id, ptuin._acct_id, ptuin._start_trade_dts, ptuin._end_trade_dts);
                e = _ptrade_man->t_get_iter_by_index2(_pssm, tmp_t_iter, prtrade, lowrep, highrep, ptuin._acct_id, ptuin._start_trade_dts, ptuin._end_trade_dts);
                if (e.is_error()) { goto done; }
                t_iter = tmp_t_iter;
            }

            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-next \n", xct_id);
            e = t_iter->next(_pssm, eof, *prtrade);
            if (e.is_error()) { goto done; }
            for(int j = 0; j < max_trades && !eof; j++){
                prtrade->get_value(7, bid_price[j]);
                prtrade->get_value(9, exec_name[j], 50); //49
                prtrade->get_value(4, is_cash[j]);
                prtrade->get_value(0, trade_list[j]);
                prtrade->get_value(10, trade_price[j]);

                num_found++;
				
                TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-next \n", xct_id);
                e = t_iter->next(_pssm, eof, *prtrade);
                if (e.is_error()) { goto done; }
            }
            for(i = 0; i < num_found; i++) {
                if(num_updated < ptuin._max_updates){
                    /**
                       select
                       cash_type = SE_CASH_TYPE
                       from
                       SETTLEMENT
                       where
                       SE_T_ID = trade_list[i]
                    */

                    TRACE( TRACE_TRX_FLOW, "App: %d TU:se-idx-probe-for-update (%ld) \n", xct_id,  trade_list[i]);
                    e =  _psettlement_man->se_index_probe_forupdate(_pssm, prsettlement, trade_list[i]);
                    if (e.is_error()) { goto done; }

                    char se_cash_type[41]; //40
                    prsettlement->get_value(1, se_cash_type, 41);

                    string temp_cash_type;
                    if(is_cash[i]){
                        if(strcmp(se_cash_type, "Cash Account") == 0){
                            temp_cash_type = "Cash";
                        }
                        else{
                            temp_cash_type = "Cash Account";
                        }
                    }
                    else{
                        if(strcmp(se_cash_type, "Margin Account") == 0){
                            temp_cash_type = "Margin";
                        }
                        else{
                            temp_cash_type = "Margin Account";
                        }
                    }

                    /**
                       update
                       SETTLEMENT
                       set
                       SE_CASH_TYPE = cash_type
                       where
                       SE_T_ID = trade_list[i]
                    */

                    TRACE( TRACE_TRX_FLOW, "App: %d TU:se-update-exec-name (%s) \n", xct_id, temp_cash_type.c_str());
                    e =  _psettlement_man->se_update_name(_pssm, prsettlement, temp_cash_type.c_str());
                    if (e.is_error()) { goto done; }

                    num_updated++;
                }

                /**
                   select
                   settlement_amount[i]        = SE_AMT,
                   settlement_cash_due_date[i] = SE_CASH_DUE_DATE,
                   settlement_cash_type[i]     = SE_CASH_TYPE
                   from
                   SETTLEMENT
                   where
                   SE_T_ID = trade_list[i]
                */

                TRACE( TRACE_TRX_FLOW, "App: %d TU:se-idx-probe (%ld) \n", xct_id,  trade_list[i]);
                e = _psettlement_man->se_index_probe(_pssm, prsettlement, trade_list[i]);
                if (e.is_error()) { goto done; }

                prsettlement->get_value(3, settlement_amount[i]);
                prsettlement->get_value(2, settlement_cash_due_date[i]);
                prsettlement->get_value(1, settlement_cash_type[i], 41);

                if(is_cash[i]){
                    /**
                       select
                       cash_transaction_amount[i] = CT_AMT,
                       cash_transaction_dts[i]    = CT_DTS
                       cash_transaction_name[i]   = CT_NAME
                       from
                       CASH_TRANSACTION
                       where
                       CT_T_ID = trade_list[i]
                    */
                    TRACE( TRACE_TRX_FLOW, "App: %d TU:cash-transaction-idx-probe (%ld) \n", xct_id,  trade_list[i]);
                    e = _pcash_transaction_man->ct_index_probe(_pssm, prcashtrans, trade_list[i]);
                    if (e.is_error()) { goto done; }

                    prcashtrans->get_value(2, cash_transaction_amount[i]);
                    prcashtrans->get_value(1, cash_transaction_dts[i]);
                    prcashtrans->get_value(3, cash_transaction_name[i], 101);
                }

                /**
                   select first 3 rows
                   trade_history_dts[i][]       = TH_DTS,
                   trade_history_status_id[i][] = TH_ST_ID
                   from
                   TRADE_HISTORY
                   where
                   TH_T_ID = trade_list[i]
                   order by
                   TH_DTS
                */
                guard<index_scan_iter_impl<trade_history_t> > th_iter;
                {
                    index_scan_iter_impl<trade_history_t>* tmp_th_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TU:th-iter-by-trade-idx \n", xct_id);
                    e = _ptrade_history_man->th_get_iter_by_index(_pssm, tmp_th_iter, prtradehist, lowrep, highrep, trade_list[i]);
                    if (e.is_error()) { goto done; }
                    th_iter = tmp_th_iter;
                }

                //ascending order
                rep_row_t sortrep(_psecurity_man->ts());
                sortrep.set(_psecurity_desc->maxsize());
                asc_sort_buffer_t th_list(2);

                th_list.setup(0, SQL_LONG);
                th_list.setup(1, SQL_FIXCHAR, 4);
				
                table_row_t rsb(&th_list);
                asc_sort_man_impl th_sorter(&th_list, &sortrep);
                bool eof;
                TRACE( TRACE_TRX_FLOW, "App: %d TU:th-iter-next \n", xct_id);
                e = th_iter->next(_pssm, eof, *prtradehist);
                if (e.is_error()) { goto done; }
                while (!eof) {
                    /* put the value into the sorted buffer */
                    myTime temp_dts;
                    char temp_stid[5]; //4

                    prtradehist->get_value(1, temp_dts);
                    prtradehist->get_value(2, temp_stid, 5);

                    rsb.set_value(0, temp_dts);
                    rsb.set_value(1, temp_stid);

                    th_sorter.add_tuple(rsb);

                    TRACE( TRACE_TRX_FLOW, "App: %d TU:th-iter-next \n", xct_id);
                    e = th_iter->next(_pssm, eof, *prtradehist);
                    if (e.is_error()) { goto done; }
                }
                assert (th_sorter.count());

                asc_sort_iter_impl th_list_sort_iter(_pssm, &th_list, &th_sorter);
                TRACE( TRACE_TRX_FLOW, "App: %d TL:th-sort-iter \n", xct_id);
                e = th_list_sort_iter.next(_pssm, eof, rsb);
                if (e.is_error()) { goto done; }
                for(int j = 0; j < 3 && !eof; j++) {
                    rsb.get_value(0, trade_history_dts[i][j]);
                    rsb.get_value(1, trade_history_status_id[i][j], 5);

                    TRACE( TRACE_TRX_FLOW, "App: %d TL:th-sort-iter \n", xct_id);
                    e = th_list_sort_iter.next(_pssm, eof, rsb);
                    if (e.is_error()) { goto done; }
                }
            }
            assert(num_updated == num_found && num_updated >= 0 && num_found <= max_trades);
        }
        else if(ptuin._frame_to_execute == 3){

            int i;
            array_guard_t<TIdent> acct_id = new TIdent[max_trades];
            array_guard_t< char[50] > exec_name = new char[max_trades][50]; //49
            array_guard_t<bool> is_cash = new bool[max_trades];
            array_guard_t<double> price = new double[max_trades];
            array_guard_t<int> quantity = new int[max_trades];
            array_guard_t< char[71] > s_name = new char[max_trades][71]; //70
            array_guard_t<myTime> trade_dts = new myTime[max_trades];
            array_guard_t<TIdent> trade_list = new TIdent[max_trades];
            array_guard_t< char[4] > trade_type = new char[max_trades][4]; //3
            array_guard_t< char[13] > type_name = new char[max_trades][13]; //12


            array_guard_t<double> settlement_amount = new double[max_trades];
            array_guard_t<myTime> settlement_cash_due_date = new myTime[max_trades];
            array_guard_t< char[41] > settlement_cash_type = new char[max_trades][41]; //40

            array_guard_t<double> cash_transaction_amount = new double[max_trades];
            array_guard_t<myTime> cash_transaction_dts = new myTime[max_trades];
            array_guard_t< char[101] > cash_transaction_name = new char[max_trades][101]; //100

            array_guard_t< myTime[3] > trade_history_dts = new myTime[max_trades][3];
            array_guard_t< char[3][5] > trade_history_status_id = new char[max_trades][3][5]; //4

            /**
               select first max_trades rows
               acct_id[]    = T_CA_ID,
               exec_name[]  = T_EXEC_NAME,
               is_cash[]    = T_IS_CASH,
               price[]      = T_TRADE_PRICE,
               quantity[]   = T_QTY,
               s_name[]     = S_NAME,
               trade_dts[]  = T_DTS,
               trade_list[] = T_ID,
               trade_type[] = T_TT_ID,
               type_name[]  = TT_NAME
               from
               TRADE,
               TRADE_TYPE,
               SECURITY
               where
               T_S_SYMB = symbol and
               T_DTS >= start_trade_dts and
               T_DTS <= end_trade_dts and
               TT_ID = T_TT_ID and
               S_SYMB = T_S_SYMB
               order by
               T_DTS asc	    
            */
            guard<index_scan_iter_impl<trade_t> > t_iter;
            {
                index_scan_iter_impl<trade_t>* tmp_t_iter;
                TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-by-idx3 %s %ld %ld \n", xct_id,  ptuin._symbol, ptuin._start_trade_dts, ptuin._end_trade_dts);
                e = _ptrade_man->t_get_iter_by_index3(_pssm, tmp_t_iter, prtrade, lowrep, highrep, ptuin._symbol, ptuin._start_trade_dts, ptuin._end_trade_dts);
                if (e.is_error()) { goto done; }
                t_iter = tmp_t_iter;
            }

            bool eof;
            TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-next \n", xct_id);
            e = t_iter->next(_pssm, eof, *prtrade);
            if (e.is_error()) { goto done; }
            for(int j = 0; j < max_trades && !eof; j++){
                prtrade->get_value(8, acct_id[j]);
                prtrade->get_value(9, exec_name[j], 50);
                prtrade->get_value(4, is_cash[j]);
                prtrade->get_value(10, price[j]);
                prtrade->get_value(6, quantity[j]);

                char t_s_symb[16]; //15
                prtrade->get_value(5, t_s_symb, 16);

                TRACE( TRACE_TRX_FLOW, "App: %d TU:s-idx-probe (%s) \n", xct_id, t_s_symb);
                e =  _psecurity_man->s_index_probe(_pssm, prsecurity, t_s_symb);
                if(e.is_error()) { goto done; }

                prsecurity->get_value(3, s_name[j], 71);
				
                prtrade->get_value(1, trade_dts[j]);
                prtrade->get_value(0, trade_list[j]);
                prtrade->get_value(3, trade_type[j], 4);

                TRACE( TRACE_TRX_FLOW, "App: %d TU:tt-idx-probe (%s) \n", xct_id,  trade_type[j]);
                e =  _ptrade_type_man->tt_index_probe(_pssm, prtradetype, trade_type[j]);
                if (e.is_error()) { goto done; }

                prtradetype->get_value(1, type_name[j], 13);

                num_found++;

                TRACE( TRACE_TRX_FLOW, "App: %d TL:t-iter-next \n", xct_id);
                e = t_iter->next(_pssm, eof, *prtrade);
                if (e.is_error()) { goto done; }
            }

            for(i = 0; i < num_found; i++) {
                /**
                   select
                   settlement_amount[i]        = SE_AMT,
                   settlement_cash_due_date[i] = SE_CASH_DUE_DATE,
                   settlement_cash_type[i]     = SE_CASH_TYPE
                   from
                   SETTLEMENT
                   where
                   SE_T_ID = trade_list[i]
                */

                TRACE( TRACE_TRX_FLOW, "App: %d TU:se-idx-probe (%ld) \n", xct_id,  trade_list[i]);
                e = _psettlement_man->se_index_probe(_pssm, prsettlement, trade_list[i]);
                if (e.is_error()) { goto done; }

                prsettlement->get_value(3, settlement_amount[i]);
                prsettlement->get_value(2, settlement_cash_due_date[i]);
                prsettlement->get_value(1, settlement_cash_type[i], 41);

                if(is_cash[i]){
                    if(num_updated < ptuin._max_updates){
                        /**
                           select
                           ct_name = CT_NAME
                           from
                           CASH_TRANSACTION
                           where
                           CT_T_ID = trade_list[i]
                        */

                        TRACE( TRACE_TRX_FLOW, "App: %d TU:cash-transaction-index-probe-forupdate (%ld) \n", xct_id,  trade_list[i]);
                        e = _pcash_transaction_man->ct_index_probe_forupdate(_pssm, prcashtrans, trade_list[i]);
                        if (e.is_error()) { goto done; }

                        char ct_name[101]; //100
                        prcashtrans->get_value(3, ct_name, 101);

                        string temp_ct_name(ct_name);
                        size_t index = temp_ct_name.find(" shares of ");
                        if(index != string::npos){
                            stringstream ss;
                            ss << type_name[i] <<  " " << quantity[i] << " Shares of " << s_name[i];
                            temp_ct_name = ss.str();
                        }
                        else{
                            stringstream ss;
                            ss << type_name[i] <<  " " << quantity[i] << " shares of " << s_name[i];
                            temp_ct_name = ss.str();
                        }
                        strcpy(ct_name, temp_ct_name.c_str());

                        /**
                           update
                           CASH_TRANSACTION
                           set
                           CT_NAME = ct_name
                           where
                           CT_T_ID = trade_list[i]		      
                        */

                        TRACE( TRACE_TRX_FLOW, "App: %d TU:ct-update-ct-name (%s) \n", xct_id, ct_name);
                        e =  _pcash_transaction_man->ct_update_name(_pssm, prcashtrans, ct_name);
                        if (e.is_error()) { goto done; }

                        num_updated++;
                    }

                    /**
                       select
                       cash_transaction_amount[i] = CT_AMT,
                       cash_transaction_dts[i]    = CT_DTS
                       cash_transaction_name[i]   = CT_NAME
                       from
                       CASH_TRANSACTION
                       where
                       CT_T_ID = trade_list[i]	    
                    */

                    TRACE( TRACE_TRX_FLOW, "App: %d TU:cash-transaction-idx-probe (%ld) \n", xct_id,  trade_list[i]);
                    e = _pcash_transaction_man->ct_index_probe(_pssm, prcashtrans, trade_list[i]);
                    if (e.is_error()) { goto done; }

                    prcashtrans->get_value(2, cash_transaction_amount[i]);
                    prcashtrans->get_value(1, cash_transaction_dts[i]);
                    prcashtrans->get_value(3, cash_transaction_name[i], 101);
                }

                /**
                   select first 3 rows
                   trade_history_dts[i][]       = TH_DTS,
                   trade_history_status_id[i][] = TH_ST_ID
                   from
                   TRADE_HISTORY
                   where
                   TH_T_ID = trade_list[i]
                   order by
                   TH_DTS asc		  
                */
                guard<index_scan_iter_impl<trade_history_t> > th_iter;
                {
                    index_scan_iter_impl<trade_history_t>* tmp_th_iter;
                    TRACE( TRACE_TRX_FLOW, "App: %d TU:th-iter-by-trade-idx %ld \n", xct_id, trade_list[i]);
                    e = _ptrade_history_man->th_get_iter_by_index(_pssm, tmp_th_iter, prtradehist, lowrep, highrep, trade_list[i]);
                    if (e.is_error()) { goto done; }
                    th_iter = tmp_th_iter;
                }

                //ascending order
                rep_row_t sortrep(_psecurity_man->ts());
                sortrep.set(_psecurity_desc->maxsize());
                asc_sort_buffer_t th_list(2);

                th_list.setup(0, SQL_LONG);
                th_list.setup(1, SQL_FIXCHAR, 4);

                table_row_t rsb(&th_list);
                asc_sort_man_impl th_sorter(&th_list, &sortrep);
                bool eof;
                TRACE( TRACE_TRX_FLOW, "App: %d TU:th-iter-next \n", xct_id);
                e = th_iter->next(_pssm, eof, *prtradehist);
                if (e.is_error()) { goto done; }
                while (!eof) {
                    /* put the value into the sorted buffer */
                    myTime temp_dts;
                    char temp_stid[5]; //4

                    prtradehist->get_value(1, temp_dts);
                    prtradehist->get_value(2, temp_stid, 5);

                    rsb.set_value(0, temp_dts);
                    rsb.set_value(1, temp_stid);

                    th_sorter.add_tuple(rsb);
					
                    TRACE( TRACE_TRX_FLOW, "App: %d TU:th-iter-next \n", xct_id);
                    e = th_iter->next(_pssm, eof, *prtradehist);
                    if (e.is_error()) { goto done; }
                }
                assert (th_sorter.count());

                asc_sort_iter_impl th_list_sort_iter(_pssm, &th_list, &th_sorter);
                TRACE( TRACE_TRX_FLOW, "App: %d TU:th-sort-iter-next \n", xct_id);
                e = th_list_sort_iter.next(_pssm, eof, rsb);
                if (e.is_error()) { goto done; }
                for(int j = 0; j < 3 && !eof; j++) {
                    rsb.get_value(0, trade_history_dts[i][j]);
                    rsb.get_value(1, trade_history_status_id[i][j], 5);

                    TRACE( TRACE_TRX_FLOW, "App: %d TU:th-sort-iter-next \n", xct_id);
                    e = th_list_sort_iter.next(_pssm, eof, rsb);
                    if (e.is_error()) { goto done; }
                }
            }
        }		
    }
#ifdef PRINT_TRX_RESULTS
    // at the end of the transaction
    // dumps the status of all the table rows used
    rcashtrans.print_tuple();
    rsecurity.print_tuple();
    rsettlement.print_tuple();
    rtrade.print_tuple();
    rtradetype.print_tuple();
    rtradehist.print_tuple();

#endif

 done:
    // return the tuples to the cache

    _pcash_transaction_man->give_tuple(prcashtrans);
    _psecurity_man->give_tuple(prsecurity);
    _psettlement_man->give_tuple(prsettlement);
    _ptrade_man->give_tuple(prtrade);
    _ptrade_type_man->give_tuple(prtradetype);
    _ptrade_history_man->give_tuple(prtradehist);

    return (e);
}

EXIT_NAMESPACE(tpce);    
