/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file gendata.cpp
 *
 *  @brief Generates table data for the TPC-C benchmark
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h> 
#include <ctype.h>

#include "util.h"

#include "workload/tpcc/dbgen/gendata.h"    
#include "workload/tpcc/dbgen/platform_io.h"
#include "workload/tpcc/dbgen/platform_sem.h"
#include "workload/tpcc/dbgen/tpcc_rnd.h" 
#include "workload/tpcc/dbgen/tpcc_misc.h"
#include "workload/tpcc/dbgen/tpcc_conf.h"    
#include "workload/tpcc/dbgen/tpcc_datastr.h"    

// global variabkes
int i, j;
double timestamp1, timestamp2, elapse;
int rc, rc1, rc2;

int using_range = 0;
int using_npipe = 0;
int using_rct = 0;
int quiet_mode = 0;

// warehouse ranges
int ware_start=-1, ware_end=-1;
int maxLocalWarehouse=-1;

void InitFormatStrings(char delim);
void ScalingReport(void);

int outtype1 = 0;
int outtype2 = 0; 
char *outname1 = NULL;
char *outname2 = NULL;



/** fn: main */
int main (int argc, char *argv[]) {
   int option = -1;
   char *delim = NULL;            


   TRACE( TRACE_ALWAYS, "*** Should make it multi-threaded!\n");


   // Compute Warehouse Ranges                                      
   ware_start = 1;          
   ware_end = WAREHOUSES;
   maxLocalWarehouse = WAREHOUSES;

   // Process Command Line Arguments

   /* Valid Command Line Options
    * --------------------------
    * Table Option:             -t <table>         (-t 3 for warehouse)
    * Output Column Delimiter:  -d <char>          (-d ' ', -d '|', etc)
    * Output to File:           -f[n] <file>       (-f customer.dat)
    * Output to Pipe:           -p[n] <pipename>   (-p tpccpipe.000)
    * Warehouse Range:          -r <start> <end>   (-r 1 100)
    * Scaling Report:           -s
    * Quiet Mode:               -q
    *
    * The -f[n] and/or -p[n] options are required.
    * The -t, -d, -r, -s and -q options are optional.
    *
    * If -d is omitted, the vertical bar (pipe) symbol ('|') will be used.
    * If -r is omitted, the range [1..WAREHOUSES] will be used.
    *
    * Due to the TPC-C spec requiring that orders and orderline be
    * generated at the same time, there is an extension to the -f and -p
    * options to specify one of the two output streams for each argument.
    *
    * -f1 orders.dat -f2 orderline.dat will output to two files
    * -f1 orders.dat -p2 tpccpipe.000 will output to a file and a pipe
    *
    * -f1/-p1 specifies the destination for the orders table
    * -f2/-p2 specifies the destination for the orderline table
    *
    */

   // Read Arguments
   for (i=1; i<argc; i++)
   {
      if (strcmp(argv[i], "-h") == 0) {
          print_tpcc_dbgen_usage();
          exit(0);
      } else if (strcmp(argv[i], "-t") == 0) {
         option = atoi(argv[i+1]);
         i++;
      }  else if (strcmp(argv[i], "-r") == 0) {
         ware_start = atoi(argv[i+1]);
         ware_end = atoi(argv[i+2]);
         i += 2;
      } else if (strcmp(argv[i], "-d") == 0) {
         delim = argv[i+1];
         i++;
      } else if ((strcmp(argv[i], "-f")  == 0) ||
                 (strcmp(argv[i], "-f1") == 0)) {
         outtype1 = IOH_FILE;
         outname1 = argv[i+1];
         i++;
      } else if (strcmp(argv[i], "-f2") == 0) {
         outtype2 = IOH_FILE;
         outname2 = argv[i+1];
         i++;
      } else if ((strcmp(argv[i], "-p")  == 0) ||
                 (strcmp(argv[i], "-p1") == 0)) {
         outtype1 = IOH_PIPE;
         outname1 = argv[i+1];
         i++;
      } else if (strcmp(argv[i], "-p2") == 0) {
         outtype2 = IOH_PIPE;
         outname2 = argv[i+1];
         i++;
      } else if (strcmp(argv[i], "-s") == 0) {             
         ScalingReport();
         exit(0);                                          
      } else if (strcmp(argv[i], "-q") == 0) {             
         quiet_mode = 1;                                   
      } else {
         fprintf(stderr, "gendata: Don't understand argument: %s\n",argv[i]);
         exit(-1);
      }
   }

   // Validate Command Line Arguments                                  

   // Validate Table Argument
   if (option < 3 || option > 11 || option == 10) {
      fprintf(stderr,"gendata: Invalid table selected: %d\n",option);
      exit(-1);
   }

   // Validate Delimiter Argument
   if (delim == NULL) {
     // default delimiter, no processing neccessary
   } else if (strlen(delim) == 1 && !isalnum(delim[0]) &&
              delim[0] != '.' && delim[0] != '%')
   {
      // user-supplied delimiter
      InitFormatStrings(delim[0]);
   } else {
      fprintf(stderr,"gendata: Invalid delimiter specified: %s\n",delim);
      exit(-1);
   }

   // Validate File/Pipe Arguments
   if (option != 9 && outtype1 > 0 && outtype2 > 0) {
      fprintf(stderr,
              "gendata: Specifying two output file/pipes allowed only " \
              "when generating\norders/orderline.\n");
      exit(-1);
   }
   
   if (option == 9 && ((outtype1 == 0) || (outtype2 == 0))) {
      fprintf(stderr,
              "gendata: Must specify two output file/pipes when generating " \
              "orders/orderline.\n");
      exit(-1);
   }

   if (outtype1 == 0 || outname1 == NULL || strcmp(outname1,"") == 0) {
      fprintf(stderr,"gendata: Invalid 1st output file/pipe specified.\n");
      exit(-1);
   }

   if (option == 9 && 
       (outtype2 == 0 || outname2 == NULL || strcmp(outname2,"") == 0)) 
       {
           fprintf(stderr,
                   "gendata: Invalid 2nd output file/pipe specified.\n");
           exit(-1);
       }


   /* Ensure O/OL flat files are opened in append mode.  This is required */
   /* because we generate O/OL concurrently.  See comments in genload.pl  */
   /* for further details on why this is neccessary.                      */
   if (option == 9)                                              
   {
      if (outtype1 == IOH_FILE) outtype1 = IOH_FILE_APPEND;
      if (outtype2 == IOH_FILE) outtype2 = IOH_FILE_APPEND;
   }                                                             

   // Validate Range Arguments
   if (ware_start <= 0 || ware_start > maxLocalWarehouse) {     
     fprintf(stderr,"gendata: Invalid range starting value: %d\n",ware_start);
     exit(-1);
   }

   if (ware_end <= 0 || 
       ware_end > maxLocalWarehouse || 
       ware_end < ware_start) 
       {
           fprintf(stderr,
                   "gendata: Invalid range ending value: %d\n",
                   ware_end);
           exit(-1);
       }

   initialize_random();                                     

   // Generate Data                                                     
   switch (option) {
    case 3: /* WAREHOUSE */
      gen_ware_tbl();      
      break;
    case 4: /* DISTRICT  */
      gen_dist_tbl();      
      break;
    case 5: /* ITEM  */    
      gen_item_tbl();      
      break;
    case 6: /* STOCK     */
      gen_stock_tbl();     
      break;
    case 7: /* CUSTOMER  */
      gen_cust_tbl();      
      break;
    case 8: /* HISTORY   */
      gen_hist_tbl();      
      break;
    case 9: /* ORDERS + ORDER_LINE */
      gen_ordr_tbl();      
      break;
    case 11: /* NEW_ORDER */
      gen_nu_ord_tbl();                                          
      break;
    case 2:
    case 10:
    default:
      fprintf(stderr, "Error: invalid option = %d \n",(option));
      break;
   }
   return 0;
}



// Util methods

// fn: Print kit usage
void print_tpcc_dbgen_usage( void ) {

    fprintf(stderr, "    \n");
    fprintf(stderr, "    Usage\n");
    fprintf(stderr, "    -----\n");
    fprintf(stderr, "    dbgen_tpcc [OPTIONS]\n");
    fprintf(stderr, "    \n");
    fprintf(stderr, "    \n");
    fprintf(stderr, "    Valid Command Line Options\n");
    fprintf(stderr, "    --------------------------\n");
    fprintf(stderr, "    Table Option:             -t <table>         (-t 3 for warehouse)\n");
    fprintf(stderr, "    Output Column Delimiter:  -d <char>          (-d ' ', -d '|', etc)\n");
    fprintf(stderr, "    Output to File:           -f[n] <file>       (-f customer.dat)\n");
    fprintf(stderr, "    Output to Pipe:           -p[n] <pipename>   (-p tpccpipe.000)\n");
    fprintf(stderr, "    Warehouse Range:          -r <start> <end>   (-r 1 100)\n");
    fprintf(stderr, "    Scaling Report:           -s\n");
    fprintf(stderr, "    Quiet Mode:               -q\n");
    fprintf(stderr, "    \n");
    fprintf(stderr, "    The -f[n] and/or -p[n] options are required.\n");
    fprintf(stderr, "    The -t, -d, -r, -s and -q options are optional.\n");
    fprintf(stderr, "    \n");
    fprintf(stderr, "    If -d is omitted, the vertical bar (pipe) symbol ('|') will be used.\n");
    fprintf(stderr, "    If -r is omitted, the range [1..WAREHOUSES] will be used.\n");
    fprintf(stderr, "    \n");
    fprintf(stderr, "    Due to the TPC-C spec requiring that orders and orderline be\n");
    fprintf(stderr, "    generated at the same time, there is an extension to the -f and -p\n");
    fprintf(stderr, "    options to specify one of the two output streams for each argument.\n");
    fprintf(stderr, "    \n");
    fprintf(stderr, "    -f1 orders.dat -f2 orderline.dat will output to two files\n");
    fprintf(stderr, "    -f1 orders.dat -p2 tpccpipe.000 will output to a file and a pipe\n");
    fprintf(stderr, "    \n");
    fprintf(stderr, "    -f1/-p1 specifies the destination for the orders table\n");
    fprintf(stderr, "    -f2/-p2 specifies the destination for the orderline table\n");
    fprintf(stderr, "    \n");
    fprintf(stderr, "    WAREHOUSE - 3\n");
    fprintf(stderr, "    DISTRICT - 4\n");
    fprintf(stderr, "    ITEM - 5\n");
    fprintf(stderr, "    STOCK - 6\n");
    fprintf(stderr, "    CUSTOMER - 7\n");
    fprintf(stderr, "    HISTORY - 8\n");
    fprintf(stderr, "    ORDERS + ORDERLINE - 9\n");
    fprintf(stderr, "    NEW_ORDER - 11\n");
    fprintf(stderr, "    \n");
    
}

// fn: Return successfuk table creation
int report_gen_success( char* tName ) {

     timestamp2 = current_time();
     elapse = timestamp2 - timestamp1;
     
     fprintf(stdout,
             "\n%s table generated in %8.2f seconds.\n\n",
             tName,
             elapse); 
     fflush(stdout);                                            

     return (1);
}


// fn: Return failed table creation
int report_gen_fail( char* tName, int iter ) {

     timestamp2 = current_time();
     elapse = timestamp2 - timestamp1;
     
     fprintf(stdout,
             "\n%s table FAILED at (I %d) after %8.2f seconds.\n\n",
             tName,
             iter,
             elapse); 
     fflush(stdout);                                            

     return (0);
}



// fn: Generate ITEM table
int gen_item_tbl( void ) {

   int item_num = 0;
   int item_im_id;
   char item_name[25];
   int item_price;
   char item_data[51];

   int numBytes;
   ioHandle hnd;
   char Buffer[1024];

   timestamp1 = current_time();

   rc = GenericOpen(&hnd, outtype1, outname1);

   if (rc != 0) {
     return (report_gen_fail("ITEM", 0));
   }


   for(item_num = 1; item_num <= ITEMS; item_num++)
   {
      /* create image id field */
      item_im_id = rand_integer( 1, 10000 );

      /* create name field */
      create_random_a_string( item_name, 14, 24);

      /* create price field */
      item_price = rand_integer( 100, 10000 );

      /* create ORIGINAL field */
      create_a_string_with_original( item_data, 26, 50, 10) ;     

      numBytes = sprintf(Buffer, fmtItem,
                         item_num,
                         item_im_id,
                         item_name,
                         item_price,
                         item_data);

      rc = GenericWrite(&hnd, Buffer, numBytes);

      if (rc != 0) {
        return (report_gen_fail("ITEM", item_num));
      }
   }

   rc = GenericClose(&hnd);

   if (rc == 0) {  
       return (report_gen_success("ITEM"));
   } else {
       return (report_gen_fail("ITEM",0));
   }
}



// fn: Generate STOCK table                                             
int gen_stock_tbl( void ) {

   int ware_num = 0;
   int stock_num = 0;                                     
   int stock_quant;
   int s_ytd;
   int s_order_cnt, s_remote_cnt;
   char stock_dist_01[25];
   char stock_dist_02[25];
   char stock_dist_03[25];
   char stock_dist_04[25];
   char stock_dist_05[25];
   char stock_dist_06[25];
   char stock_dist_07[25];
   char stock_dist_08[25];
   char stock_dist_09[25];
   char stock_dist_10[25];
   char stock_data[51] ;

   int numBytes;                                       
   ioHandle hnd;                                           
   char Buffer[1024];

   timestamp1 = current_time();                            

   rc = GenericOpen(&hnd, outtype1, outname1);             
   if (rc != 0) { 
     report_gen_fail("STOCK", 0);
   }                       


   // ---------------------------------------------------------------
   // Defect 258698:  Change STOCK index order
   // ---------------------------------------------------------------
   // The STOCK index needs to be created in (S_I_ID, S_W_ID) order
   // in order to take advantage of the skew on S_I_ID.  We need to
   // generate the data in that order to get CLUSTERRATIO 100%.
   // ---------------------------------------------------------------

   for (stock_num = 1; stock_num <= STOCK_PER_WAREHOUSE; stock_num++) {

      if (!quiet_mode && (stock_num%500 == 0)) {
         fprintf(stdout, "STOCK for Item #%d\n",stock_num);
         fflush(stdout);                                   
      }

      for (ware_num = ware_start; ware_num <= ware_end; ware_num++) {
         stock_quant = rand_integer( 10, 100 ) ;
         create_random_a_string( stock_dist_01, 24, 24);
         create_random_a_string( stock_dist_02, 24, 24);
         create_random_a_string( stock_dist_03, 24, 24);
         create_random_a_string( stock_dist_04, 24, 24);
         create_random_a_string( stock_dist_05, 24, 24);
         create_random_a_string( stock_dist_06, 24, 24);
         create_random_a_string( stock_dist_07, 24, 24);
         create_random_a_string( stock_dist_08, 24, 24);
         create_random_a_string( stock_dist_09, 24, 24);
         create_random_a_string( stock_dist_10, 24, 24);

         /* create ORIGINAL field */
         create_a_string_with_original( stock_data, 26, 50, 10 ); 
         s_ytd = s_order_cnt = s_remote_cnt = 0;

         numBytes = sprintf(Buffer, fmtStock,                     
                            stock_num,
                            ware_num,
                            s_remote_cnt,
                            stock_quant,
                            s_order_cnt,
                            s_ytd,
                            stock_dist_01,
                            stock_dist_02,
                            stock_dist_03,
                            stock_dist_04,
                            stock_dist_05,
                            stock_dist_06,
                            stock_dist_07,
                            stock_dist_08,
                            stock_dist_09,
                            stock_dist_10,
                            stock_data);

         rc = GenericWrite(&hnd, Buffer, numBytes);              
         
         if (rc != 0) {
             return (report_gen_fail("STOCK", ware_num));
         }
      }
   } 

   rc = GenericClose(&hnd);

   if (rc == 0) {                                               
     return (report_gen_success("STOCK"));
   } else {
     return (report_gen_fail("STOCK",0));
   }
}


// fn: Generate WAREHOUSE table
int gen_ware_tbl( void ) {
   int ware_num = 0;    
   char ware_name[11];
   char ware_street_1[21];
   char ware_street_2[21];
   char ware_city[21];
   char ware_state[3];
   char ware_zip[10];
   double ware_tax;

   int numBytes;
   ioHandle hnd;
   char Buffer[1024];

   timestamp1 = current_time();

   rc = GenericOpen(&hnd, outtype1, outname1);
   if (rc != 0) {
       return (report_gen_fail("WAREHOUSE", 0));
   }           

   for (ware_num = ware_start; ware_num <= ware_end; ware_num++)
   {
      if (!quiet_mode && ((ware_num % 500) == 0)) {             
         fprintf(stdout, "Warehouse #%d\n", ware_num);          
         fflush(stdout);                                        
      }

      create_random_a_string( ware_name,      6,10) ; /* create name */
      create_random_a_string( ware_street_1, 10,20) ; /* create street 1 */
      create_random_a_string( ware_street_2, 10,20) ; /* create street 2 */
      create_random_a_string( ware_city,     10,20) ; /* create city */
      create_random_a_string( ware_state,     2,2) ;  /* create state */
      create_random_n_string( ware_zip,       4,4) ;  /* create zip */
      strcat(ware_zip, "11111");

      ware_tax = ((double) rand_integer(0, 2000)) / (double)10000.0;

      numBytes = sprintf(Buffer, fmtWare,
                         ware_num,
                         ware_name,
                         ware_street_1,
                         ware_street_2,
                         ware_city,
                         ware_state,
                         ware_zip,
                         ware_tax,
                         300000.0);

      rc = GenericWrite(&hnd, Buffer, numBytes); 
      if (rc != 0) { 
          return (report_gen_fail("WAREHOUSE", 0));
      }                           
   }

   rc = GenericClose(&hnd);

   if (rc == 0) {                                               
     return (report_gen_success("WAREHOUSE"));
   } else {
     return (report_gen_fail("WAREHOUSE",0));
   }
}


// Generate DISTRICT table
int gen_dist_tbl( void ) {
   int ware_num = 0;   
   int dist_num = 0;   
   char dist_name[11];
   char dist_street_1[21];
   char dist_street_2[21];
   char dist_city[21];
   char dist_state[3];
   char dist_zip[10];
   double dist_tax;
   int  next_o_id;

   int numBytes;
   ioHandle hnd;
   char Buffer[1024];

   next_o_id = CUSTOMERS_PER_DISTRICT + 1;
   timestamp1 = current_time();                                   

   rc = GenericOpen(&hnd, outtype1, outname1);                    
   if (rc != 0) { 
       return (report_gen_fail("DISTRICT", 0));
   }

   for (ware_num = ware_start; ware_num <= ware_end; ware_num++)  
   {
      if (!quiet_mode) {                                          
         fprintf(stdout, "DISTRICT for Warehouse #%d\n", ware_num);
         fflush(stdout);                                         
      }                                                          

      for (dist_num = 1; dist_num <= DISTRICTS_PER_WAREHOUSE; dist_num++) {
         create_random_a_string( dist_name,      6,10) ; /* create name */
         create_random_a_string( dist_street_1, 10,20) ; /* create street 1 */
         create_random_a_string( dist_street_2, 10,20) ; /* create street 2 */
         create_random_a_string( dist_city,     10,20) ; /* create city */
         create_random_a_string( dist_state,     2,2) ;  /* create state */
         create_random_n_string( dist_zip,       4,4) ;  /* create zip */
         strcat(dist_zip, "11111");
         dist_tax = ((double) rand_integer(0, 2000)) / (double)10000.0;

         numBytes = sprintf(Buffer, fmtDist,                      
                            dist_num,
                            ware_num,
                            dist_name,
                            dist_street_1,
                            dist_street_2,
                            dist_city,
                            dist_state,
                            dist_zip,
                            dist_tax,
                            30000.0,
                            next_o_id);

         rc = GenericWrite(&hnd, Buffer, numBytes);
         if (rc != 0) { 
             return (report_gen_fail("DISTRICT", dist_num));
         }

      } 
   } 

   rc = GenericClose(&hnd);

   if (rc == 0) {                                               
     return (report_gen_success("DISTRICT"));
   } else {
     return (report_gen_fail("DISTRICT",0));
   }
}


// Generate CUSTOMER table
int gen_cust_tbl( void ) {
   int ware_num = 0;    
   int dist_num = 0;    
   int cust_num = 0;    
   char cust_last[17];
   char cust_middle[3];
   char cust_first[17];
   char cust_street_1[21];
   char cust_street_2[21];
   char cust_city[21];
   char cust_state[3];
   char cust_zip[10];
   char cust_phone[17];
   char cust_credit[3];
   char cust_data1[251];
   char cust_data2[251];
   double cust_discount;
   Int64 currtmstmp;

   int numBytes;
   ioHandle hnd;    
   char Buffer[1024];

   timestamp1 = current_time();

   rc = GenericOpen(&hnd, outtype1, outname1);

   if (rc != 0) { 
       return (report_gen_fail("CUSTOMER", 0));
   }

   strcpy(cust_middle, "OE");
   currtmstmp = time(NULL);

   // ---------------------------------------------------------------
   // Defect 264315:  Change CUSTOMER index order
   // ---------------------------------------------------------------
   // The CUSTOMER index needs to be created in (C_ID, C_W_ID, C_D_ID)
   // order in order to take advantage of the skew on C_ID.  We need
   // to generate the data in that order to get CLUSTERRATIO 100%.
   // ---------------------------------------------------------------

   for (cust_num = 1; cust_num <= CUSTOMERS_PER_DISTRICT; cust_num++)
   {
      if (!quiet_mode) {                                         
         fprintf(stdout, "CUSTOMER #%d:\n", cust_num);           
         fflush(stdout);                                         
      }                                                          

      for (ware_num = ware_start; ware_num <= ware_end; ware_num++)
      {
         for (dist_num = 1; dist_num <= DISTRICTS_PER_WAREHOUSE; dist_num++)
         {
            if (cust_num <= 1000)                       /* create last name */
               create_random_last_name( cust_last, cust_num);
            else                                        /* create last name */
               create_random_last_name( cust_last, 0);
            create_random_a_string( cust_first,     8,16) ; /* create first name */
            create_random_a_string( cust_street_1, 10,20) ; /* create street 1 */
            create_random_a_string( cust_street_2, 10,20) ; /* create street 2 */
            create_random_a_string( cust_city,     10,20) ; /* create city */
            create_random_a_string( cust_state,     2,2) ;  /* create state */
            create_random_n_string( cust_zip,       4,4) ;  /* create zip */
            strcat(cust_zip, "11111");

            /* create phone number */
            create_random_n_string( cust_phone, 16,16) ;
            if ( rand_integer( 1, 100 ) <= 10 )                  
               strcpy( cust_credit, "BC" ) ;
            else
               strcpy( cust_credit, "GC" ) ;

            /* create discount rate */
            cust_discount = ((double) rand_integer(0, 5000)) / (double)10000.0;

            /* create customer data */
            create_random_a_string( cust_data1, 250, 250);
            create_random_a_string( cust_data2, 50,  250);

            // ---------------------------------------------------------------
            // Defect 343303: Dump C_C_LAST_LOAD
            // ---------------------------------------------------------------
            // As per Francois' request, we dump the vaue of C_C_LAST_LOAD
            // in the C_FIRST field of the first customer.
            // This makes is simple to validate what value of C_C_LAST_LOAD
            // was used to generate the database.
            // ---------------------------------------------------------------

            if (cust_num == 1 && dist_num == 1 && ware_num == 1) {
              sprintf(cust_first,"C_LAST_LOAD=%d",C_C_LAST_LOAD);
            }                                                   

            numBytes = sprintf(Buffer, fmtCust,                 
                               cust_num,
                               dist_num,
                               ware_num,
                               cust_first,
                               cust_middle,
                               cust_last,
                               cust_street_1,
                               cust_street_2,
                               cust_city,
                               cust_state,
                               cust_zip,
                               cust_phone,
                               currtmstmp,
                               cust_credit,
                               50000.00,
                               cust_discount,
                               0,
                               -10.0,
                               10.0,
                               1,
                               cust_data1,
                               cust_data2);

            rc = GenericWrite(&hnd, Buffer, numBytes);
            if (rc != 0) { 
                return (report_gen_fail("CUSTOMER", cust_num));
            }

         } 
      } 
   } 

   rc = GenericClose(&hnd);

   if (rc == 0) {                                               
     return (report_gen_success("CUSTOMER"));
   } else {
     return (report_gen_fail("CUSTOMER",0));
   }
}


// Generate HISTORY table
int gen_hist_tbl( void ) {
   int ware_num = 0 ;    
   int dist_num = 0 ;    
   int cust_num = 0 ;    
   char hist_data[25] ;
   Int64 currtmstmp;     

   int numBytes;     
   ioHandle hnd;
   char Buffer[1024];

   timestamp1 = current_time();                                  

   rc = GenericOpen(&hnd, outtype1, outname1);                   
   if (rc != 0) { 
       return (report_gen_fail("HISTORY", 0));
   }

   currtmstmp = time(NULL);                                     

   for (ware_num = ware_start; ware_num <= ware_end; ware_num++)
   {
      if (!quiet_mode) {                                         
         fprintf(stdout, "HISTORY for Warehouse #%d:\n", ware_num);
         fflush(stdout);                                          
      }                                                           

      for (dist_num = 1; dist_num <= DISTRICTS_PER_WAREHOUSE; dist_num++)
      {
         for (cust_num = 1; cust_num <= CUSTOMERS_PER_DISTRICT; cust_num++)
         {
            /* create history data */
            create_random_a_string( hist_data, 12,24) ;

            numBytes = sprintf(Buffer, fmtHist,                   
                               cust_num,
                               dist_num,
                               ware_num,
                               dist_num,
                               ware_num,
                               currtmstmp,
                               1000,
                               hist_data);

            rc = GenericWrite(&hnd, Buffer, numBytes);
            if (rc != 0) { 
                return (report_gen_fail("HISTORY", cust_num));
            }

         } 
      } 
   } 

   rc = GenericClose(&hnd);

   if (rc == 0) {                                               
     return (report_gen_success("HISTORY"));
   } else {
     return (report_gen_fail("HISTORY",0));
   }
}


// Generate NEW_ORDER table
int gen_nu_ord_tbl( void ) {
   int ware_num = 0 ;      
   int dist_num = 0 ;      
   int nu_ord_id = 0 ;     
   int nu_ord_hi ;

   int numBytes;       
   ioHandle hnd;           
   char Buffer[1024];

   /* compute maximum and minimum
      order numbers for this
      district */
   nu_ord_hi = CUSTOMERS_PER_DISTRICT - NU_ORDERS_PER_DISTRICT + 1;

   if (nu_ord_hi < 0) {
      nu_ord_hi = CUSTOMERS_PER_DISTRICT - (CUSTOMERS_PER_DISTRICT / 3) + 1;
      fprintf(stderr,
              "\n**** WARNING **** NU_ORDERS_PER_DISTRICT is >" \
              " CUSTOMERS_PER_DISTRICT\n");
      fprintf(stderr,
              "                  " \
              "Check the values in file tpcc_conf.h\n");
      fprintf(stderr,
              "                  " \
              "Loading New-Order with 1/3 of CUSTOMERS_PER_DISTRICT\n");
   }

   timestamp1 = current_time();                                   

   rc = GenericOpen(&hnd, outtype1, outname1);                   
   if (rc != 0) { 
       return (report_gen_fail("NEW_ORDER", 0));
   }

   /* We generate in O/W/D order for non-RCT tables.  With the
    * data clustered on O_ID, this gives us good bufferpool
    * characteristics.  We also create a btree index in W/D/O
    * order, to satisfy MIN(O_ID) queries.
    *
    * For RCT tables *with* RCT Jump Cache, we *should* generate
    * the data in W/D/O order (to match the table definition.)
    * We don't since it would push schema decision into flat file
    * generation (and I don't want to do that.)  It's just as easy
    * to sort the flat files afterwards.
    */

   for (nu_ord_id = nu_ord_hi;
        nu_ord_id <= CUSTOMERS_PER_DISTRICT;
        nu_ord_id++)
   {
      if (!quiet_mode) {                                          
         fprintf(stdout, "NEW_ORDER for Customer #%d:\n", nu_ord_id); 
         fflush(stdout);                                          
      }                                                           
      for (ware_num = ware_start; ware_num <= ware_end; ware_num++)
      {
         for (dist_num = 1; dist_num <= DISTRICTS_PER_WAREHOUSE; dist_num++)
         {
            numBytes = sprintf(Buffer, fmtNewOrd,                 
                               nu_ord_id,
                               dist_num,
                               ware_num); 

            rc = GenericWrite(&hnd, Buffer, numBytes);            
            if (rc != 0) { 
                return (report_gen_fail("NEW_ORDER", dist_num));
            }

         } 
      } 
   } 

   rc = GenericClose(&hnd);

   if (rc == 0) {                                               
     return (report_gen_success("NEW_ORDER"));
   } else {
     return (report_gen_fail("NEW_ORDER",0));
   }
}


// Generate ORDER and ORDER_LINE tables
int gen_ordr_tbl( void ) {
   int ware_num = 0 ;    
   int dist_num = 0 ;    
   int cust_num = 0 ;    
   int ord_num = 0 ;     
   int ordr_carrier_id;
   int ordr_ol_cnt;
   int oline_ol_num;
   int oline_item_num;

   int oline_amount;
   char oline_dist_info[25];
   Int64 nulltmstmp = 0;
   Int64 currtmstmp;    

   int numBytes;    
   ioHandle hnd1, hnd2; 
   char Buffer[1024];

   oline_dist_info[24] = '\0';

   timestamp1 = current_time();                                  

   rc1 = GenericOpen(&hnd1, outtype1, outname1);                 
   if (rc1 != 0) { 
       return (report_gen_fail("ORDER", 0));
   }

   rc2 = GenericOpen(&hnd2, outtype2, outname2);                 
   if (rc2 != 0) { 
       return (report_gen_fail("ORDER_LINE", 0));
   }

   currtmstmp = time(NULL);                                      

   // ---------------------------------------------------------------
   // Defect 264432:  Change ORDERS index order
   // ---------------------------------------------------------------
   // The ORDERS index needs to be created in (O_ID, O_W_ID, O_D_ID)
   // order in order to take advantage of the spread on O_ID.  We
   // should generate the data in that order to get CLUSTERRATIO 100%.
   // ---------------------------------------------------------------
   // However, doing so means that we need to keep state for all of
   // the O_ID permulations for each warehouse, which has too high of
   // a memory cost.  So, we generate in O_W_ID, O_D_ID, O_ID order
   // and then sort the flat files before loading in order to get
   // CLUSTERRATIO 100% after loading.
   // ---------------------------------------------------------------

   for (ware_num = ware_start; ware_num <= ware_end; ware_num++)       
       {
           if (!quiet_mode) {                                        
               fprintf(stdout, "ORDERS & ORDER_LINE for Warehouse #%d\n", 
                       ware_num); 
               fflush(stdout);                                        
           }                                                         
           for (dist_num = 1; dist_num <= DISTRICTS_PER_WAREHOUSE; dist_num++)
               {
                   if (!quiet_mode) {                                    
                       fprintf(stdout, "District #%d\t", dist_num);       
                       fflush(stdout);                                    
                   }                                                     
                   
                   seed_1_3000();
                   
                   for (ord_num = 1; ord_num <= CUSTOMERS_PER_DISTRICT; ord_num++)
                       {
                           if (ord_num < 2101)
                               ordr_carrier_id = rand_integer( 1, 10 ) ;
                           else
                               ordr_carrier_id = 0;
                           
                           cust_num = random_1_3000();                         
                           ordr_ol_cnt = rand_integer(MIN_OL_PER_ORDER,MAX_OL_PER_ORDER);
                           
                           numBytes = sprintf(Buffer, fmtOrdr,                 
                                              ord_num,
                                              cust_num,
                                              dist_num,
                                              ware_num,
                                              currtmstmp,
                                              ordr_carrier_id,
                                              ordr_ol_cnt,
                                              1);
                           
                           rc1 = GenericWrite(&hnd1, Buffer, numBytes);         
                           if (rc1 != 0) { 
                               return (report_gen_fail("ORDERS", ord_num));
                           }
                           
                           for ( oline_ol_num = 1; 
                                 oline_ol_num <= ordr_ol_cnt; oline_ol_num++ )
                               {
                                   oline_item_num = rand_integer(1, ITEMS) ;
                                   create_random_a_string( oline_dist_info, 24, 24) ;
                                   oline_amount = rand_integer(001, 999999);
                                   
                                   if (ord_num < 2101)
                                       {
                                           oline_amount = 0;
                                           numBytes = sprintf(Buffer, fmtOLine,
                                                              ord_num,
                                                              dist_num,
                                                              ware_num,
                                                              oline_ol_num,
                                                              oline_item_num,
                                                              ware_num,
                                                              currtmstmp,
                                                              5,
                                                              oline_amount,
                                                              oline_dist_info);
                                       } else {
                                           numBytes = sprintf(Buffer, fmtOLine,           
                                                              ord_num,
                                                              dist_num,
                                                              ware_num,
                                                              oline_ol_num,
                                                              oline_item_num,
                                                              ware_num,
                                                              nulltmstmp,                  
                                                              5,
                                                              oline_amount,
                                                              oline_dist_info);
                                       } /* if (ord_num < 2101) */
                                   
                                   rc2 = GenericWrite(&hnd2, Buffer, numBytes);      
                                   if (rc2 != 0) { 
                                       return (report_gen_fail("ORDER_LINE", oline_item_num));
                                   }

                               } 
                       } 
               } 
       } 
   
   rc1 = GenericClose(&hnd2);
   rc2 = GenericClose(&hnd1);


   if ((rc1 == 0) && (rc2 == 0)) {  
       return (report_gen_success("ORDER && ORDER_LINE"));
   } else {
       return (report_gen_fail("ORDER OR ORDER_LINE",0));
   }
   
}


// This routine will initalize the printf format strings and replace the
// delimiter with the one provided.  The pipe symbol is the default.
void InitFormatStrings(char delim) {
  char *p;

  // Check if Using Default Delimiter
  if (delim == '|') return;

  // Replace Delimiters
  while (p = strchr(fmtWare,'|'))   { *p = delim; }
  while (p = strchr(fmtDist,'|'))   { *p = delim; }
  while (p = strchr(fmtItem,'|'))   { *p = delim; }
  while (p = strchr(fmtStock,'|'))  { *p = delim; }
  while (p = strchr(fmtCust,'|'))   { *p = delim; }
  while (p = strchr(fmtHist,'|'))   { *p = delim; }
  while (p = strchr(fmtOrdr,'|'))   { *p = delim; }
  while (p = strchr(fmtOLine,'|'))  { *p = delim; }
  while (p = strchr(fmtNewOrd,'|')) { *p = delim; }
}                                  


// fn: Print scaling values
void ScalingReport(void) {
   fprintf(stdout,"Scaling Values in Use\n");
   fprintf(stdout,"-------------------------------\n");
   fprintf(stdout,"Warehouses:            %d\n", WAREHOUSES);
   fprintf(stdout,"Districts/Warehouse:   %d\n", DISTRICTS_PER_WAREHOUSE);
   fprintf(stdout,"Customers/District:    %d\n", CUSTOMERS_PER_DISTRICT);
   fprintf(stdout,"Items:                 %d\n", ITEMS);
   fprintf(stdout,"Stock/Warehouse:       %d\n", STOCK_PER_WAREHOUSE);
   fprintf(stdout,"Min Order Lines/Order: %d\n", MIN_OL_PER_ORDER);
   fprintf(stdout,"Max Order Lines/Order: %d\n", MAX_OL_PER_ORDER);
   fprintf(stdout,"New Orders/District:   %d\n", NU_ORDERS_PER_DISTRICT);
   fprintf(stdout,"-------------------------------\n");
   fprintf(stdout,"Local Warehouses:      %d\n", maxLocalWarehouse);
   fprintf(stdout,"-------------------------------\n");
}                                                      
