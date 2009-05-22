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

/** @file:   shore_tpcc_random.h
 *
 *  @brief:  Declaration of random functions needed by the TPC-C kit
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_TPCC_RANDOM_H
#define __SHORE_TPCC_RANDOM_H

#include "util.h"

#include "workload/tpcc/common/tpcc_const.h"


ENTER_NAMESPACE(tpcc);


/****************************************************************** 
 *  @class: tpcc_random_gen_t
 *
 *  @brief: Class which contains the random generators for the TPC-C
 *          kit.
 *
 *  @note:  Main purpose of this class is to be used as a static
 *          member for each table
 *
 ******************************************************************/

class tpcc_random_gen_t {
private:
    uint _seed;
    char _tbl_3000[3000];
    pthread_mutex_t _tbl_mutex;

public:

    /* -------------------- */
    /* --- construction --- */
    /* -------------------- */

    tpcc_random_gen_t(uint x = NULL) 
    { 
        pthread_mutex_init(&_tbl_mutex, NULL);
        reset();
    }
    
    ~tpcc_random_gen_t() 
    { 
        pthread_mutex_destroy(&_tbl_mutex);
    }


    /* ------------------------------- */
    /* --- helper random functions --- */
    /* ------------------------------- */

    void reset(uint x = NULL) {
        if (x)
            _seed = x;
        else 
            _seed = (getpid() + time(NULL)) * time(NULL);
        
        for (int i=0;i<3000;i++)
            _tbl_3000[i] = 0;
    }

    void init_random(int x);
    int  random_integer(int val_lo, int val_hi);
    
    void seed_1_3000(void);
    int random_1_3000(void);

    int NUrand_val(int A, int x, int y, int C);


    /* ----------------------------- */
    /* --- tpcc random functions --- */
    /* ----------------------------- */


    int  random_a_string(char * string,
                         int min_size,
                         int max_size);
    int  random_n_string(char * string,
                         int min_size,
                         int max_size);

    int  random_last_name(char * string,
                          int id);
    char* random_last_name(int end);

    float random_float_val_return(float min,
                                  float max);

    int   random_c_id();

    int string_with_original(char * string,
                             int min_size,
                             int max_size,
                             float percentage_to_set,
                             int * hit);

    void random_ol_cnt(short ol_cnt_array[]);

    int random_i_id();

}; // EOF: tpcc_random_gen_t



EXIT_NAMESPACE(tpcc);


#endif /* __SHORE_TPCC_RANDOM_H */
