/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tpcc_random.cpp
 *
 *  @brief:  Implementation of random functions needed by the TPC-C kit
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#include "workload/tpcc/shore_tpcc_random.h"


using namespace tpcc;


/* ----------------- */
/* --- constants --- */
/* ----------------- */

#define A_C_ID          1023
#define C_C_ID          127
#define C_C_LAST        173
#define C_OL_I_ID       1723
#define A_C_LAST        255
#define A_OL_I_ID       8191

static char alnum[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static char *last_name_parts[] =
    {
        "BAR",
        "OUGHT",
        "ABLE",
        "PRI",
        "PRES",
        "ESE",
        "ANTI",
        "CALLY",
        "ATION",
        "EING"
    };


/* ---------------------------------------- */
/* --- @class tpcc_random_gen_t methods --- */
/* ---------------------------------------- */


/** --- helper random functions --- */

int tpcc_random_gen_t::random_integer(int val_lo, int val_hi)
{
    return((rand_r(&_seed)%(val_hi-val_lo+1))+val_lo);
}


int tpcc_random_gen_t::NUrand_val(int A, int x, int y, int C)
{
    return((((random_integer(0,A)|random_integer(x,y))+C)%(y-x+1))+x);
}


void tpcc_random_gen_t::init_random(int x)
{
    srand(x);
}


int tpcc_random_gen_t::random_a_string(char* out_buffer, 
                                       int length_lo, 
                                       int length_hi )
{
    int i, actual_length ;

    actual_length = random_integer(length_lo, length_hi);

    for (i=0;i < actual_length; i++ ) {
        out_buffer[i] = alnum[random_integer(0, 61)];
    }

    out_buffer[actual_length] = '\0' ;
    i = strlen(out_buffer) ;
    return(i);
}


int tpcc_random_gen_t::random_n_string(char* out_buffer, 
                                       int length_lo, 
                                       int length_hi)
{
    int i, actual_length;

    actual_length = random_integer(length_lo, length_hi);

    for (i=0;i < actual_length; i++ ) {
        out_buffer[i] = (char)random_integer('0', '9' ) ;
    }

    out_buffer[actual_length] = '\0' ;
    i = strlen(out_buffer) ;
    return(i);
}



/* --- tpcc specific random functions --- */


inline int  tpcc_random_gen_t::random_c_id() 
{
    return (NUrand_val(A_C_ID, 1, CUSTOMERS_PER_DISTRICT, C_C_ID));
}


int tpcc_random_gen_t::random_last_name(char *out_buffer, 
                                        int cust_num)
{
    int random_num;

    if (cust_num == 0)
        random_num = NUrand_val( A_C_LAST, 0, 999, C_C_LAST );
    else
        random_num = (cust_num % 1000);

    strcpy(out_buffer, last_name_parts[random_num / 100]);
    random_num %= 100;
    strcat(out_buffer, last_name_parts[random_num / 10]);
    random_num %= 10;
    strcat(out_buffer, last_name_parts[random_num]);

    return(strlen(out_buffer));
}


char* tpcc_random_gen_t::random_last_name(const int end)
{
    return last_name_parts[NUrand_val(A_C_LAST, 0, end, C_C_LAST)];
}


float tpcc_random_gen_t::random_float_val_return(float min, 
                                                 float max)
{
    return (((float)rand_r(&_seed))/(((float)32767)*(max-min)+min));
}


void tpcc_random_gen_t::seed_1_3000()
{
    //*** critical section ***
    critical_section_t cs(_tbl_mutex);
    //    CRITICAL_SECTION(_tbl_mutex);

    for (int i = 0; i < CUSTOMERS_PER_DISTRICT; i++)
        _tbl_3000[i] = 0;
}


int tpcc_random_gen_t::random_1_3000()
{
    //*** critical section ***
    critical_section_t cs(_tbl_mutex);
    //CRITICAL_SECTION(_tbl_mutex);
 
    int x = random_integer(0, CUSTOMERS_PER_DISTRICT - 1);

    for (int i=0; i<CUSTOMERS_PER_DISTRICT; i++) {
        if (_tbl_3000[x] == 0) {
            _tbl_3000[x] = 1;      
            return(x+1);
        }
        else                                        
            x++;

        if (x == CUSTOMERS_PER_DISTRICT)            
            x=0;
    }

    // We should not reach this line
    cerr << "Error in producing random_1_3000... " << endl;
}


int tpcc_random_gen_t::string_with_original( char *out_buffer, 
                                             int length_lo,
                                             int length_hi, 
                                             float percent_to_set,
                                             int *hit )
{
    float f ;
    int actual_length, start_pos ;

    actual_length = random_a_string( out_buffer, length_lo,
                                     length_hi ) ;
    f = random_float_val_return( 0, 100.0 ) ;
    *hit = 0 ;
    if ( f < percent_to_set ) {
        start_pos = random_integer( 0, actual_length-8 ) ;
        strncpy(out_buffer+start_pos,"ORIGINAL",8) ;
        *hit = 1 ;
    }

    return(actual_length);
}


void tpcc_random_gen_t::random_ol_cnt(short* ol_cnt_array)
{
    int average;
    long i;
    long top;
    long ol_cnt_rqd = 0;
    long ol_cnt_total = 0;
    long ol_cnt_diff = 0;

    /* calculate the number of array entries required */
    top = DISTRICTS_PER_WAREHOUSE * CUSTOMERS_PER_DISTRICT;
    /* initiallize the array and keep running total */
    for (i=0; i < top; i++) {
        ol_cnt_array[i] = random_integer(MIN_OL_PER_ORDER, MAX_OL_PER_ORDER);
        ol_cnt_total += ol_cnt_array[i];
    }

    /* compute the average ol_cnt * 10 to keep the precision since divide by 2 */
    average = (MIN_OL_PER_ORDER + MAX_OL_PER_ORDER) * 10 / 2;
    /* if the average is not 100 (10 * average 10), then user is building a strange */
    /* scale so give warning message. */
    if (average != 100) {
        fprintf(stderr,"\nWARNING: The average order lines per order is %.1f.\n",
		(double)average/10.0);
        fprintf(stderr,"         This should be 10 in a properly scaled database!\n");
    }
    /* compute the required total or goal (this is per warehouse) */
    ol_cnt_rqd = top * average / 10;
    /* find out how far off from the goal we are */
    ol_cnt_diff = ol_cnt_total - ol_cnt_rqd;
    fprintf(stderr,"ol_cnt_array loaded %d/%d (diff = %+d",
	    ol_cnt_total, ol_cnt_rqd, ol_cnt_diff);

    /* until we reach our goal, randomly modify an array entry */
    while (ol_cnt_diff != 0) {
        /* pick a random array entry */
        i = random_integer(0, top-1);
        /* do we have too many or too few */
        if (ol_cnt_diff < 0) {
            /* oops too few so increment this random entry by 1 */
            if (ol_cnt_array[i] < MAX_OL_PER_ORDER) {
                ol_cnt_array[i]++;
                ol_cnt_diff++;
            }
        } else {
            /* oops too many so decrement this random entry by 1 */
            if (ol_cnt_array[i] > MIN_OL_PER_ORDER){
                ol_cnt_array[i]--;
                ol_cnt_diff--;
            }
        }
    }
    /* ahh, life is good */
    fprintf(stderr," has been adjusted)\n");
}


int tpcc_random_gen_t::random_i_id()
{
    return NUrand_val(A_OL_I_ID, 1, ITEMS, C_OL_I_ID);
}
