/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file gendata.h
 *
 *  @brief Interface for the table data generation functions
 *  for the TPC-C benchmark
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __TPCC_DBGEN_GENDATA_H
#define __TPCC_DBGEN_GENDATA_H


// Int64
typedef int Int64;


// PROTOTYPES
int gen_dist_tbl( void );                                      
int gen_cust_tbl( void );
int gen_hist_tbl( void );
int gen_nu_ord_tbl( void );
int gen_ordr_tbl( void );
int gen_item_tbl( void );
int gen_stock_tbl( void );
int gen_ware_tbl( void );                                      

// method declaration
void print_tpcc_dbgen_usage(void);

// format of the various tables
char fmtWare[]    = "%d|%s|%s|%s|%s|%s|%s|%f|%f|\n";            
char fmtDist[]    = "%d|%d|%s|%s|%s|%s|%s|%s|%f|%f|%d\n";       
char fmtItem[]    = "%d|%d|%s|%d|%s\n";
char fmtStock[]   = "%d|%d|%d|%d|%d|%d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|\n";

char fmtCust[]    = "%d|%d|%d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%lld|%s|%f|%f|%d|%f|%f|%d|%s|%s\n";
char fmtHist[]    = "%d|%d|%d|%d|%d|%lld|%d|%s\n";
char fmtOrdr[]    = "%d|%d|%d|%d|%lld|%d|%d|%d|\n";
char fmtOLine[]   = "%d|%d|%d|%d|%d|%d|%lld|%d|%d|%s\n";        

char fmtNewOrd[]  = "%d|%d|%d\n";                               


#endif
