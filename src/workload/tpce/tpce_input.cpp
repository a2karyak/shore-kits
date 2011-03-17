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

/** @file:  tpce_input.cpp
 *
 *  @brief: Implementation of the (common) inputs for the TPCE trxs
 */


#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif


#include "workload/tpce/tpce_input.h"

#include "workload/tpce/egen/CE.h"


using namespace TPCE;

ENTER_NAMESPACE(tpce);


//Converts EGEN TIME representation to time_t structure
myTime EgenTimeToTimeT(CDateTime &cdt)
{ 
    struct tm ts;
    int msec; 
    cdt.GetYMDHMS(&ts.tm_year, &ts.tm_mon, &ts.tm_mday, &ts.tm_hour, &ts.tm_min, &ts.tm_sec, &msec);
    ts.tm_year -= 1900; // counts after 1900;
    ts.tm_mon -= 1; // expects zero based month
    ts.tm_isdst=1; // daylight saving time

    time_t x = mktime (&ts);
//    printf("returned time %s\n ", ctime(&x));
    return (myTime)x;
}

//Converts EGEN TIMESTAMP representation to time_t structure
myTime EgenTimeStampToTimeT(TIMESTAMP_STRUCT &tss)
{ 
    struct tm ts;
    ts.tm_year = tss.year -1900;
    ts.tm_mon = tss.month-1;
    ts.tm_mday =  tss.day; 
    ts.tm_hour =tss.hour;
    ts.tm_min =tss. minute;
    ts.tm_sec = tss.second; 
    ts.tm_isdst=1; // daylight saving time
    time_t x = mktime (&ts);
//    printf("returned time %s\n ", ctime(&x));
    return (myTime)x;
}

int dayOfMonth(myTime& t)
{  
  struct tm* ts=localtime((time_t*)&t);
  return ts->tm_mday;
}


int random_xct_type(const double idx)
{
  double sum = 0;

    sum += PROB_TPCE_DATA_MAINTENANCE;
    if (idx < sum){
	printf(" idx %lf  sum %lf\n", idx, sum); assert(false);
	return XCT_TPCE_DATA_MAINTENANCE;
     }
/* sum += PROB_TPCE_MARKET_FEED;
  if (idx < sum)
    return XCT_TPCE_MARKET_FEED;*/

    sum += PROB_TPCE_TRADE_UPDATE;
    if (idx < sum)
	return XCT_TPCE_TRADE_UPDATE;

    sum += PROB_TPCE_BROKER_VOLUME;
    if (idx < sum)
	return XCT_TPCE_BROKER_VOLUME;
  
    sum += PROB_TPCE_TRADE_LOOKUP;
    if (idx < sum)
	return XCT_TPCE_TRADE_LOOKUP;
    
 /*   sum += PROB_TPCE_TRADE_RESULT;
    if (idx < sum)
	return XCT_TPCE_TRADE_RESULT;*/
    
    sum += PROB_TPCE_TRADE_ORDER;
    if (idx < sum)
	return XCT_TPCE_TRADE_ORDER;

    sum += PROB_TPCE_CUSTOMER_POSITION;
    if (idx < sum)
	return XCT_TPCE_CUSTOMER_POSITION;
    
    sum += PROB_TPCE_SECURITY_DETAIL;
    if (idx < sum)
	return XCT_TPCE_SECURITY_DETAIL;
    
    sum += PROB_TPCE_MARKET_WATCH;
    if (idx < sum)
	return XCT_TPCE_MARKET_WATCH;
    
    sum += PROB_TPCE_TRADE_STATUS;
    if (idx <= sum)
	return XCT_TPCE_TRADE_STATUS;
  printf("************sum %lf**********\n ", sum);
    return -1;
};


//broker volume
broker_volume_input_t     create_broker_volume_input(int sf, int specificIdx) 
{ 
    broker_volume_input_t abvi;
    TBrokerVolumeTxnInput		m_BrokerVolumeTxnInput;
    m_TxnInputGenerator->GenerateBrokerVolumeInput( m_BrokerVolumeTxnInput );
    for(int i=0; i<40; i++)
      memcpy(abvi._broker_list[i], m_BrokerVolumeTxnInput.broker_list[i], 50);
    memcpy(abvi._sector_name, m_BrokerVolumeTxnInput.sector_name, 31);
    return (abvi);
};

void broker_volume_input_t::print()
{	
	  printf("\nbroker_volume_input\n");
	  for (int i=0; i<40; i++) printf(" %s\n", _broker_list[i]);
	  printf(" %s\n", _sector_name);
}


//customer position
customer_position_input_t create_customer_position_input(int sf, int specificIdx) 
{ 
    customer_position_input_t acpi;
    TCustomerPositionTxnInput	m_CustomerPositionTxnInput;
    m_TxnInputGenerator->GenerateCustomerPositionInput( m_CustomerPositionTxnInput );

    acpi._get_history=m_CustomerPositionTxnInput.get_history;
    acpi._acct_id_idx=m_CustomerPositionTxnInput.acct_id_idx;
    acpi._cust_id=m_CustomerPositionTxnInput.cust_id;
    memcpy(acpi._tax_id, m_CustomerPositionTxnInput.tax_id, cTAX_ID_len+1); /// added 1

    return (acpi);
};

void customer_position_input_t::print(){
	  printf("\nCustomer position input\n");
	  printf(" %ld\n", _acct_id_idx);
	  printf(" %ld\n", _cust_id);
	  printf(" %d\n", _get_history);
	  printf(" %s\n", _tax_id);
}

//trade order
trade_order_input_t    create_trade_order_input(int sf, int specificIdx) 
{ 
    trade_order_input_t ati;
    TTradeOrderTxnInput		m_TradeOrderTxnInput;
    bool	bExecutorIsAccountOwner;
    INT32	iTradeType;
    m_TxnInputGenerator->GenerateTradeOrderInput( m_TradeOrderTxnInput, iTradeType, bExecutorIsAccountOwner );

    ati._acct_id=m_TradeOrderTxnInput.acct_id;
    ati._requested_price=m_TradeOrderTxnInput.requested_price;
    ati._roll_it_back=(m_TradeOrderTxnInput.roll_it_back !=0);
    ati._is_lifo=(m_TradeOrderTxnInput.is_lifo!=0);
    ati._type_is_margin=(m_TradeOrderTxnInput.type_is_margin!=0);
    ati._trade_qty=m_TradeOrderTxnInput.trade_qty;

    memcpy(ati._co_name, m_TradeOrderTxnInput.co_name, 61); 
    memcpy(ati._exec_f_name, m_TradeOrderTxnInput.exec_f_name, 21); 
    memcpy(ati._exec_l_name, m_TradeOrderTxnInput.exec_l_name, 26); 
    memcpy(ati._exec_tax_id, m_TradeOrderTxnInput.exec_tax_id, 21); 
    memcpy(ati._issue, m_TradeOrderTxnInput.issue, 7); 
    memcpy(ati._st_pending_id, m_TradeOrderTxnInput.st_pending_id, 5); 
    memcpy(ati._st_submitted_id, m_TradeOrderTxnInput.st_submitted_id, 5); 
    memcpy(ati._symbol, m_TradeOrderTxnInput.symbol, 16); 
    memcpy(ati._trade_type_id, m_TradeOrderTxnInput.trade_type_id, 4); 

    return (ati);
};

void trade_order_input_t::print(){
	  printf("\ntrade order input\n");
	  printf(" %ld\n", _acct_id);
	  printf(" %s\n", _co_name);
	  printf(" %s\n", _exec_f_name);
	  printf(" %s\n", _exec_l_name);
	  printf(" %s\n", _exec_tax_id);
	  printf(" %d\n", _is_lifo);
	  printf(" %s\n", _issue);
	  printf(" %lf\n", _requested_price);
	  printf(" %d\n", _roll_it_back);
	  printf(" %s\n", _st_pending_id);
	  printf(" %s\n", _st_submitted_id);
	  printf(" %s\n", _symbol);
	  printf(" %d\n", _trade_qty);
	  printf(" %s\n", _trade_type_id);
	  printf(" %d\n", _type_is_margin);
}

//trade lookup
trade_lookup_input_t      create_trade_lookup_input(int sf, int specificIdx) 
{ 
    trade_lookup_input_t atli;
    TTradeLookupTxnInput		m_TradeLookupTxnInput;
    m_TxnInputGenerator->GenerateTradeLookupInput( m_TradeLookupTxnInput );


    atli._acct_id=m_TradeLookupTxnInput.acct_id;
    atli._frame_to_execute=m_TradeLookupTxnInput.frame_to_execute;
    atli._max_trades=m_TradeLookupTxnInput.max_trades;
    atli._max_acct_id=m_TradeLookupTxnInput.max_acct_id;
  
    memcpy(atli._symbol, m_TradeLookupTxnInput.symbol, 16); 
    memcpy(atli._trade_id, m_TradeLookupTxnInput.trade_id, 30*sizeof(TIdent)); 
    atli._start_trade_dts = EgenTimeStampToTimeT(m_TradeLookupTxnInput.start_trade_dts); 
    atli._end_trade_dts = EgenTimeStampToTimeT(m_TradeLookupTxnInput.end_trade_dts); 
    
    return (atli);
};

void trade_lookup_input_t::print(){
	  printf("\ntrade lookup input\n");
	  printf(" %ld\n", _acct_id);
	  printf(" %d\n", _frame_to_execute);
	  printf(" %ld\n", _max_acct_id);
	  printf(" %d\n", _max_trades);
	  printf(" %ld (%s)\n", _start_trade_dts,  ctime((const time_t*)&_start_trade_dts));
	  printf(" %ld (%s)\n", _end_trade_dts,  ctime((const time_t*)&_end_trade_dts)); 
	  printf(" %s\n", _symbol);
	  printf(" %ld\n", _trade_id);
}

//trade result
trade_result_input_t      create_trade_result_input(int sf, int specificIdx) 
{ 
    trade_result_input_t atri;
    TTradeResultTxnInput* input =TradeResultInputBuffer->get();
    assert(input!=NULL);   
    atri._trade_id=input->trade_id;
    atri._trade_price=input->trade_price;
    return (atri);

//this is just for testing]!!!
/*    trade_result_input_t atri;
    atri._trade_id=200000800000125;
    atri._trade_price=24.69;
    return (atri);*/
};

void trade_result_input_t::print(){
	  printf("\ntrade_result_input\n");
	  printf(" %ld\n", _trade_id);
	  printf(" %lf\n", _trade_price);
}


//market watch
market_watch_input_t      create_market_watch_input(int sf, int specificIdx) 
{ 

    market_watch_input_t amwi;
    TMarketWatchTxnInput		m_MarketWatchTxnInput;
    m_TxnInputGenerator->GenerateMarketWatchInput( m_MarketWatchTxnInput );

    amwi._acct_id=m_MarketWatchTxnInput.acct_id;
    amwi._cust_id=m_MarketWatchTxnInput.c_id;
    amwi._starting_co_id=m_MarketWatchTxnInput.starting_co_id;
    amwi._ending_co_id=m_MarketWatchTxnInput.ending_co_id;
  
    memcpy(amwi._industry_name, m_MarketWatchTxnInput.industry_name, 51); 

    amwi._start_date=EgenTimeStampToTimeT(m_MarketWatchTxnInput.start_day);

    return (amwi);
};

void market_watch_input_t::print(){
	  printf("\nmarket_watch_input\n");
	  printf(" %ld\n", _acct_id);
	  printf(" %ld\n", _cust_id);
	  printf(" %ld\n", _starting_co_id);
	  printf(" %ld\n", _ending_co_id);
	  printf(" %s\n", _industry_name); 
	  printf(" %ld (%s)\n", _start_date,  ctime((const time_t*)&_start_date));
}


//security detail
security_detail_input_t   create_security_detail_input(int sf, int specificIdx) 
{ 
    security_detail_input_t asdi;
    TSecurityDetailTxnInput		m_SecurityDetailTxnInput;
    m_TxnInputGenerator->GenerateSecurityDetailInput( m_SecurityDetailTxnInput );  
    asdi._access_lob_flag=m_SecurityDetailTxnInput.access_lob_flag;
    asdi._max_rows_to_return=m_SecurityDetailTxnInput.max_rows_to_return;   
    asdi._start_day=EgenTimeStampToTimeT(m_SecurityDetailTxnInput.start_day);
    memcpy(asdi._symbol, m_SecurityDetailTxnInput.symbol, 16 ); 

    return (asdi);
};


void security_detail_input_t::print(){
	  printf("\nsecurity_detail_input\n");
	  printf(" %d\n", _access_lob_flag);
	  printf(" %s\n", _symbol);
	  printf(" %ld (%s)\n", _start_day,  ctime((const time_t*)&_start_day));
	  printf(" %d\n", _max_rows_to_return);
}


//trade status
trade_status_input_t      create_trade_status_input(int sf, int specificIdx) 
{ 
    trade_status_input_t atsi;	
    TTradeStatusTxnInput		m_TradeStatusTxnInput;
    m_TxnInputGenerator->GenerateTradeStatusInput( m_TradeStatusTxnInput );
    atsi._acct_id = m_TradeStatusTxnInput.acct_id;
    return (atsi);
};


void trade_status_input_t::print(){
	  printf("\ntrade_status_input\n");
	  printf(" %ld\n",_acct_id);
}

//trade update
trade_update_input_t      create_trade_update_input(int sf, int specificIdx) 
{ 
    trade_update_input_t atui;
    TTradeUpdateTxnInput		m_TradeUpdateTxnInput;
    m_TxnInputGenerator->GenerateTradeUpdateInput( m_TradeUpdateTxnInput );

    atui._acct_id=m_TradeUpdateTxnInput.acct_id;
    atui._frame_to_execute=m_TradeUpdateTxnInput.frame_to_execute;
    atui._max_trades=m_TradeUpdateTxnInput.max_trades;
    atui._max_updates=m_TradeUpdateTxnInput.max_updates;
    atui._max_acct_id=m_TradeUpdateTxnInput.max_acct_id;

    atui._start_trade_dts = EgenTimeStampToTimeT(m_TradeUpdateTxnInput.start_trade_dts); 
    atui._end_trade_dts = EgenTimeStampToTimeT(m_TradeUpdateTxnInput.end_trade_dts); 
  

    memcpy(atui._symbol, m_TradeUpdateTxnInput.symbol, 16 ); 
    memcpy(atui._trade_id, m_TradeUpdateTxnInput.trade_id, TradeUpdateFrame1MaxRows*sizeof(TIdent) ); 

    return (atui);
};

void trade_update_input_t::print(){
	  printf("\ntrade_update_input\n");
	  printf(" %ld\n", _acct_id);
	  printf(" %d\n", _frame_to_execute);
	  printf(" %d\n",_max_trades);
	  printf(" %d\n", _max_updates);

	  printf(" %s\n", _symbol);
	  printf(" %ld (%s)\n", _start_trade_dts,  ctime((const time_t*)&_start_trade_dts));
	  printf(" %ld (%s)\n", _end_trade_dts,  ctime((const time_t*)&_end_trade_dts));
	  for(int i=0; i<TradeUpdateFrame1MaxRows; i++) printf(" %ld\n", _trade_id[i]);
	  printf(" %ld\n", _max_acct_id);
}


//market feed
market_feed_input_t create_market_feed_input(int sf, int specificIdx) 
{ 
    
    market_feed_input_t amfi;
    TMarketFeedTxnInput* input= MarketFeedInputBuffer->get();    

    assert (input);    
    memcpy(amfi._status_submitted,input->StatusAndTradeType.status_submitted,5);
    memcpy(amfi._type_limit_buy,input->StatusAndTradeType.type_limit_buy,4);
    memcpy(amfi._type_limit_sell,input->StatusAndTradeType.type_limit_sell,4);
    memcpy(amfi._type_stop_loss,input->StatusAndTradeType.type_stop_loss,4);
    for(int i=0; i<max_feed_len; i++)
      {
        amfi._trade_qty[i] = input->Entries[i].trade_qty;
        memcpy(amfi._symbol[i], input->Entries[i].symbol, 16);
        amfi._price_quote[i] = input->Entries[i].price_quote;
      }
    delete input;
    return (amfi);


//this is for testing purposes
/*
   market_feed_input_t amfi;
    
    strcpy(amfi._status_submitted,"SBMT");
    strcpy(amfi._type_limit_buy,"TLB");
    strcpy(amfi._type_limit_sell,"TLS");
    strcpy(amfi._type_stop_loss,"TSL");

    
    for(int i=0; i<20; i++)
      {
        amfi._trade_qty[i] = 234+i;        
        amfi._price_quote[i] = 22.34+i;
      }
    strcpy(amfi._symbol[0], "A");
    strcpy(amfi._symbol[1], "AA");
    strcpy(amfi._symbol[2], "AACB");
    strcpy(amfi._symbol[3], "AALA");
    strcpy(amfi._symbol[4], "AAPL");
    strcpy(amfi._symbol[5], "ACTI");
    strcpy(amfi._symbol[6], "ACTT");
    strcpy(amfi._symbol[7], "ACV");
    strcpy(amfi._symbol[8], "ADM");
    strcpy(amfi._symbol[9], "ADMPRA");
    strcpy(amfi._symbol[10], "ADPRA");
    strcpy(amfi._symbol[11], "ADPRB");
    strcpy(amfi._symbol[12], "AEG");
    strcpy(amfi._symbol[13], "AGAM");
    strcpy(amfi._symbol[14], "AIB");
    strcpy(amfi._symbol[15], "AIF");
    strcpy(amfi._symbol[16], "ALA");
    strcpy(amfi._symbol[17], "AMFH");
    strcpy(amfi._symbol[18], "BWG");
    strcpy(amfi._symbol[19], "BXP");

    return (amfi);
*/
};

void market_feed_input_t::print()
{
      	  printf("\nmarket_feed_input\n");
	  for(int i=0; i<max_feed_len; i++ ) printf(" %ld ", _price_quote[i]); printf("\n");
	  printf(" %s\n", _status_submitted);
	  for(int i=0; i<max_feed_len; i++ ) printf(" %s ", _symbol[i]); printf("\n");
	  for(int i=0; i<max_feed_len; i++ ) printf(" %d ", _trade_qty[i]);
	  printf(" %s\n", _type_limit_buy);
	  printf(" %s\n", _type_limit_sell);
	  printf(" %s\n", _type_stop_loss);
}


//data maintenance
data_maintenance_input_t  create_data_maintenance_input(int sf, int specificIdx) 
{ 
    data_maintenance_input_t admi;
    TDataMaintenanceTxnInput*	m_TxnInput = m_CDM->createDMInput();
    
    admi._acct_id=m_TxnInput->acct_id;
    admi._c_id=m_TxnInput->c_id;
    admi._co_id=m_TxnInput->co_id;
    admi._day_of_month=m_TxnInput->day_of_month;
    admi._vol_incr=m_TxnInput->vol_incr;
    memcpy(admi._symbol, m_TxnInput->symbol, 16 ); 
    memcpy(admi._table_name, m_TxnInput->table_name, 31 ); 
    memcpy(admi._tx_id, m_TxnInput->tx_id, 21 ); 
    delete m_TxnInput;
    return (admi);
};

void data_maintenance_input_t::print(){
	  printf("\ndata_maintenance_input\n");
	  printf(" %ld\n", _acct_id);
	  printf(" %ld\n", _c_id);
	  printf(" %ld\n", _co_id);
	  printf(" %d \n", _day_of_month);
	  printf(" %s\n", _symbol);
	  printf(" %s\n", _table_name);
	  printf(" %s\n", _tx_id);
	  printf(" %d\n", _vol_incr);
}


//trade cleanup
trade_cleanup_input_t     create_trade_cleanup_input(int sf, int specificIdx) 
{ 
    trade_cleanup_input_t atci;
    TTradeCleanupTxnInput*  m_CleanupTxnInput = m_CDM->createTCInput();

    atci._trade_id=m_CleanupTxnInput->start_trade_id;
    memcpy(atci._st_canceled_id, m_CleanupTxnInput->st_canceled_id, 5 ); 
    memcpy(atci._st_pending_id, m_CleanupTxnInput->st_pending_id, 5 ); 
    memcpy(atci._st_submitted_id, m_CleanupTxnInput->st_submitted_id, 5 ); 
    delete m_CleanupTxnInput;
    return (atci);
}

void trade_cleanup_input_t::print(){
	  printf("\ntrade_cleanup_input\n");
	  printf(" %ld \n", _trade_id);
	  printf(" %s\n", _st_canceled_id);
	  printf(" %s\n", _st_pending_id);
	  printf(" %s\n", _st_submitted_id);
}

EXIT_NAMESPACE(tpce);

