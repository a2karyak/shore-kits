/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_rnd.h
 *
 *  @brief Declaration of random generation functions for TPC-C
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_RND_H
#define __TPCC_RND_H

void initialize_random(void);                                
int rand_integer( int val_lo, int val_hi ) ;
int NUrand_val( int A, int val_lo, int val_hi, int C ) ;

void seed_1_3000( void );

int random_1_3000( void );

int create_random_a_string( char *out_buffer,
                            int length_lo,
                            int length_hi );

int create_random_n_string( char *out_buffer,
                            int length_lo,
                            int length_hi );

int create_a_string_with_original( char *out_buffer,
                                   int length_lo,
                                   int length_hi,
                                   int percent_to_set ) ;

int create_random_last_name(char *out_buffer, int cust_num);

#endif
