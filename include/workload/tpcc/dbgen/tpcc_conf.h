/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_conf.h
 *
 *  @brief Important configuratio variables and values
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __TPCC_CONF_H
#define __TPCC_CONF_H


/** Note: ITEMS must be = STOCK_PER_WAREHOUSE or else you will get
 *        sqlcode = 100 in neword.
 *
 *        NU_ORDERS_PER_DISTRICT should be < CUSTOMERS_PER_DISTRICT
 *        if not, the load of New-Order table will be based on 1/3
 *        of the CUSTOMERS_PER_DISTRICT.
 */

#define WAREHOUSES                              10
//#define WAREHOUSES                              100
#define DISTRICTS_PER_WAREHOUSE                 10
#define CUSTOMERS_PER_DISTRICT                  3000
#define ITEMS                                   100000
#define STOCK_PER_WAREHOUSE                     ITEMS
#define MIN_OL_PER_ORDER                        5
#define MAX_OL_PER_ORDER                        15
#define NU_ORDERS_PER_DISTRICT                  900



/* *********************************************************************** */
/* Transaction Return Codes (s_transtatus)                                 */
/* *********************************************************************** */

#define INVALID_ITEM            100
#define TRAN_OK                 0
#define FATAL_SQLERROR          -1

/* *********************************************************************** */
/* Definition of Unused and Bad Items                                      */
/* *********************************************************************** */
/* Define unused item ID to be 0. This allows the SUT to determine the     */
/* number of items in the order as required by 2.4.1.3 and 2.4.2.2 since   */
/* the assumption that any item with OL_I_ID = 0 is unused will be true.   */
/* This in turn requires that the value used for an invalid item is        */
/* equal to ITEMS + 1.                                                     */
/* *********************************************************************** */

#define INVALID_ITEM_ID (2 * ITEMS) + 1                           //@d274273jnh
#define UNUSED_ITEM_ID 0                                          //@d274273jnh

#define MIN_WAREHOUSE 1                                           //@d274273jnh
#define MAX_WAREHOUSE WAREHOUSES                                  //@d274273jnh

/***************************************************************************/
/* NURand Constants                                                        */
/* C_C_LAST_RUN and C_C_LAST_LOAD must adhere to clause 2.1.6.             */
/* Analysis indicates that a C_LAST delta of 85 is optimal.                */
/***************************************************************************/
#define C_C_LAST_RUN	88
#define C_C_LAST_LOAD	173
#define C_C_ID		319                                       //@d283859mte
#define C_OL_I_ID	3849                                      //@d283859mte
#define A_C_LAST	255
#define A_C_ID		1023
#define A_OL_I_ID	8191

/***************************************************************************/
/* Transaction Type Identifiers                                            */
/***************************************************************************/

#define CLIENT_SQL   0                                            //@d299530mte
#define NEWORD_SQL   1
#define PAYMENT_SQL  2                                            //@d251704mte
#define ORDSTAT_SQL  3                                            //@d251704mte
#define DELIVERY_SQL 4                                            //@d251704mte
#define STOCKLEV_SQL 5

#endif
