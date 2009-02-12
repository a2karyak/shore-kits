/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tm1_input.h
 *
 *  @brief:  Declaration of the (common) inputs for the TM1 trxs
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */


#ifndef __TM1_INPUT_H
#define __TM1_INPUT_H

#include "util.h"
#include "workload/tm1/tm1_const.h"


ENTER_NAMESPACE(tm1);



/** Exported helper functions */

const int   URand(const int low, const int high);
const bool  URandBool();
const short URandShort(const short low, const short high);


const char TM1_CAPS[]  = { "ABCDEFGHIJKLMNOPQRSTUVWXYZ" };
const char TM1_NUMBX[] = { "012345789" }; 

const void URandFillStrCaps(char* dest, const int sz);
const void URandFillStrNumbx(char* dest, const int sz);




/* use this for allocation of NULL-terminated strings */
#define STRSIZE(x)(x+1)


/** Exported data structures */

struct tm1_sub_t
{
    int   S_ID;
    char  SUB_NBR[STRSIZE(15)];
    bool  BIT_XX[10];
    short HEX_XX[10];
    short BYTE2_XX[10];
    int   MSC_LOCATION;
    int   VLR_LOCATION;
};


struct tm1_ai_t
{
    int   S_ID;
    short AI_TYPE;
    short DATA1;
    short DATA2;
    char  DATA3[STRSIZE(3)];
    char  DATA4[STRSIZE(5)];
};


struct tm1_sf_t
{
    int   S_ID;
    short SF_TYPE;
    bool  IS_ACTIVE;
    short ERROR_CNTRL;
    short DATA_A;
    char  DATA_B[STRSIZE(5)];
};


struct tm1_cf_t
{
    int   S_ID;
    short SF_TYPE;
    short START_TIME;
    short END_TIME;
    char  NUMBERX[STRSIZE(15)];
};


/*********************************************************************
 * 
 * get_sub_data_input_t
 *
 * Input for any GET_SUBSCRIBER_DATA transaction
 *
 *********************************************************************/

struct get_sub_data_input_t 
{
    /**
     *  @brief GET_SUBSCRIBER_DATA transaction inputs:
     *  
     *  1) S_ID int [1 .. SF] : subscriber id
     */

    int    _s_id;             /* input: URand(1,SF) */

    get_sub_data_input_t& operator= (const get_sub_data_input_t& rhs);

}; // EOF get_sub_data_input_t



/*********************************************************************
 * 
 * get_new_dest_input_t
 *
 * Input for any GET_NEW_DESTINATION transaction
 *
 *********************************************************************/

struct get_new_dest_input_t 
{
    /**
     *  @brief GET_NEW_DESTINATION transaction inputs:
     *  
     *  1) S_ID    int   [1 .. SF] : subscriber id
     *  2) SF_TYPE short [1 .. 4]  : special facility type
     *  3) S_TIME  short {0,8,16}  : start time
     *  4) E_TIME  short [1 .. 24] : end time
     */

    int    _s_id;            /* input: URand(1,SF) */
    short  _sf_type;         /* input: URand(1,4) */
    short  _s_time;          /* input: URand(0,2)*8 */
    short  _e_time;          /* input: URand(1,2x4) */

    get_new_dest_input_t& operator= (const get_new_dest_input_t& rhs);

}; // EOF get_new_dest_input_t



/*********************************************************************
 * 
 * get_acc_data_input_t
 *
 * Input for any GET_ACCESS_DATA transaction
 *
 *********************************************************************/

struct get_acc_data_input_t 
{
    /**
     *  @brief GET_ACC_DATA transaction inputs:
     *  
     *  1) S_ID    int   [1 .. SF] : subscriber id
     *  2) AI_TYPE short [1 .. 4]  : access info type
     */

    int    _s_id;            /* input: URand(1,SF) */
    short  _ai_type;         /* input: URand(1,4) */

    get_acc_data_input_t& operator= (const get_acc_data_input_t& rhs);

}; // EOF get_acc_data_input_t



/*********************************************************************
 * 
 * upd_sub_data_input_t
 *
 * Input for any UPDATE_SUBSCRIBER_DATA transaction
 *
 *********************************************************************/

struct upd_sub_data_input_t 
{
    /**
     *  @brief GET_ACC_DATA transaction inputs:
     *  
     *  1) S_ID    int   [1 .. SF] : subscriber id
     *  2) SF_TYPE short [1 .. 4]  : special facility type
     *  3) A_BIT   bool  {true,false} : random subscriber.bit_1
     *  4) A_DATA  short [0 .. 255]  : random sf.data_a
     */

    int    _s_id;            /* input: URand(1,SF) */
    short  _sf_type;         /* input: URand(1,4) */
    bool   _a_bit;           /* input: URand(0,1)*true */
    short  _a_data;          /* input: URand(0,255) */

    upd_sub_data_input_t& operator= (const upd_sub_data_input_t& rhs);

}; // EOF upd_sub_data_input_t



/*********************************************************************
 * 
 * upd_loc_input_t
 *
 * Input for any UPDATE_LOCATION transaction
 *
 *********************************************************************/

struct upd_loc_input_t 
{
    /**
     *  @brief UPDATE_LOCATION transaction inputs:
     *  
     *  1) S_ID    int   [1 .. SF] : subscriber id
     *  2) SUB_NBR string[15] of [1 .. SF] : subscriber number
     *  3) VLR_LOC int [1 .. MAXINT]  : vlr location
     */

    int    _s_id;            /* input: URand(1,SF) */
    char   _sub_nbr[15];     /* input: string(URand(1,SF)) */
    short  _vlr_loc;         /* input: URand(0,MAXINT) */

    upd_loc_input_t& operator= (const upd_loc_input_t& rhs);

}; // EOF upd_loc_input_t



/*********************************************************************
 * 
 * ins_call_fwd_input_t
 *
 * Input for any INSERT_CALL_FORWARDING transaction
 *
 *********************************************************************/

struct ins_call_fwd_input_t 
{
    /**
     *  @brief INSERT_CALL_FORWARDING transaction inputs:
     *  
     *  1) S_ID    int   [1 .. SF] : subscriber id
     *  2) SUB_NBR string[15] of [1 .. SF] : subscriber number
     *  3) SF_TYPE short [1 .. 4]  : special facility type
     *  4) S_TIME  short {0,8,16}  : start time
     *  5) E_TIME  short [1 .. 24] : end time
     *  6) NUMBERX string[15] of [1 .. SF] : number-X
     */

    int    _s_id;            /* input: URand(1,SF) */
    char   _sub_nbr[15];     /* input: string(_s_id) */
    short  _sf_type;         /* input: URand(1,4) */
    short  _s_time;          /* input: URand(0,2)*8 */
    short  _e_time;          /* input: URand(1,2x4) */
    char   _numberx[15];     /* input: string(URand(1,SF)) */

    ins_call_fwd_input_t& operator= (const ins_call_fwd_input_t& rhs);

}; // EOF ins_call_fwd_input_t



/*********************************************************************
 * 
 * del_call_fwd_input_t
 *
 * Input for any DELETE_CALL_FORWARDING transaction
 *
 *********************************************************************/

struct del_call_fwd_input_t 
{
    /**
     *  @brief DELETE_CALL_FORWARDING transaction inputs:
     *  
     *  1) S_ID    int   [1 .. SF] : subscriber id
     *  2) SUB_NBR string[15] of [1 .. SF] : subscriber number
     *  3) SF_TYPE short [1 .. 4]  : special facility type
     *  4) S_TIME  short {0,8,16}  : start time
     */

    int    _s_id;            /* input: URand(1,SF) */
    char   _sub_nbr[15];     /* input: string(_s_id) */
    short  _sf_type;         /* input: URand(1,4) */
    short  _s_time;          /* input: URand(0,2)*8 */

    del_call_fwd_input_t& operator= (const del_call_fwd_input_t& rhs);

}; // EOF del_call_fwd_input_t



/////////////////////////////////////////////////////////////
//
// @brief: Declaration of functions that generate the inputs 
//         for the TM1 TRXs
//
/////////////////////////////////////////////////////////////


get_sub_data_input_t create_get_sub_data_input(int SF, 
                                               int specificSub = 0);


get_new_dest_input_t create_get_new_dest_input(int SF, 
                                               int specificSub = 0);


get_acc_data_input_t create_get_acc_data_input(int SF, 
                                               int specificSub = 0);


upd_sub_data_input_t create_upd_sub_data_input(int SF, 
                                               int specificSub = 0);


upd_loc_input_t create_upd_loc_input(int SF, 
                                     int specificSub = 0);


ins_call_fwd_input_t create_ins_call_fwd_input(int SF, 
                                               int specificSub = 0);


del_call_fwd_input_t create_del_call_fwd_input(int SF, 
                                               int specificSub = 0);



/* --- translates or picks a random xct type given the benchmark specification --- */


const int random_tm1_xct_type(const int selected);





EXIT_NAMESPACE(tm1);


#endif

