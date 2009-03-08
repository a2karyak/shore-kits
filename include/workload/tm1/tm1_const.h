/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tm1_const.h
 *
 *  @brief:  Constants needed by the TM1 kit
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */

#ifndef __TM1_CONST_H
#define __TM1_CONST_H


#include "util/namespace.h"


ENTER_NAMESPACE(tm1);


// the scaling factor unit (arbitrarily chosen)
// SF = 1   --> 10K Subscribers
// SF = 100 --> 1M  Subscribers
const int TM1_SUBS_PER_SF = 10000;

// DORA_SF = 1 --> 10K Subscribes (SF == DORA_SF)
const int TM1_SUBS_PER_DORA_PART = 10000;


const int TM1_DEF_SF = 10;
const int TM1_DEF_QF = 10;


// commit every 1000 new Subscribers
const int TM1_LOADING_COMMIT_INTERVAL = 1000;
const int TM1_LOADING_TRACE_INTERVAL  = 10000;


// loading-related defaults
//const int TM1_LOADERS_TO_USE     = 40;
const int TM1_LOADERS_TO_USE     = 1;
const int TM1_MAX_NUM_OF_LOADERS = 128;
const int TM1_SUBS_TO_PRELOAD = 2000; 


/* --- standard scale -- */

const int TM1_MIN_AI_PER_SUBSCR = 1;
const int TM1_MAX_AI_PER_SUBSCR = 4;

const int TM1_MIN_SF_PER_SUBSCR = 1;
const int TM1_MAX_SF_PER_SUBSCR = 4;

const int TM1_MIN_CF_PER_SF = 0;
const int TM1_MAX_CF_PER_SF = 3;

const int TM1_PROB_ACTIVE_SF_YES = 85;
const int TM1_PROB_ACTIVE_SF_NO  = 15;


/* --- number of fields per table --- */

// @note It is the actual number of fields plus one for padding

const int TM1_SUB_FCOUNT  = 35; //34;
const int TM1_AI_FCOUNT   = 7;  //6;
const int TM1_SF_FCOUNT   = 7;  //6;
const int TM1_CF_FCOUNT   = 6;  //5;


// some field sizes
const int TM1_SUB_NBR_SZ    = 15;
const int TM1_AI_DATA3_SZ   = 3;
const int TM1_AI_DATA4_SZ   = 5;
const int TM1_SF_DATA_B_SZ  = 5;
const int TM1_CF_NUMBERX_SZ = 15;




/* --------------------------- */
/* --- TM1 TRANSACTION MIX --- */
/* --------------------------- */

const int XCT_TM1_MIX           = 20;
const int XCT_TM1_GET_SUB_DATA  = 21;
const int XCT_TM1_GET_NEW_DEST  = 22;
const int XCT_TM1_GET_ACC_DATA  = 23;
const int XCT_TM1_UPD_SUB_DATA  = 24;
const int XCT_TM1_UPD_LOCATION  = 25;
const int XCT_TM1_CALL_FWD_MIX  = 26;

const int XCT_TM1_INS_CALL_FWD  = 27;
const int XCT_TM1_DEL_CALL_FWD  = 28;



// @note: The DORA_XXX should be (DORA_MIX + REGULAR_TRX_ID)
const int XCT_TM1_DORA_MIX           = 200;
const int XCT_TM1_DORA_GET_SUB_DATA  = 221;
const int XCT_TM1_DORA_GET_NEW_DEST  = 222;
const int XCT_TM1_DORA_GET_ACC_DATA  = 223;
const int XCT_TM1_DORA_UPD_SUB_DATA  = 224;
const int XCT_TM1_DORA_UPD_LOCATION  = 225;
const int XCT_TM1_DORA_CALL_FWD_MIX  = 226;

const int XCT_TM1_DORA_INS_CALL_FWD  = 227;
const int XCT_TM1_DORA_DEL_CALL_FWD  = 228;




/* --- probabilities for the TM1 MIX --- */

// READ-ONLY (80%)
const int PROB_TM1_GET_SUB_DATA  = 35;
const int PROB_TM1_GET_NEW_DEST  = 10;
const int PROB_TM1_GET_ACC_DATA  = 35;

// UPDATE (20%)
const int PROB_TM1_UPD_SUB_DATA  = 2;
const int PROB_TM1_UPD_LOCATION  = 14;
const int PROB_TM1_INS_CALL_FWD  = 2;
const int PROB_TM1_DEL_CALL_FWD  = 2;


EXIT_NAMESPACE(tm1);

#endif /* __TM1_CONST_H */
