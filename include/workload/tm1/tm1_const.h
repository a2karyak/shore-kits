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


/* --------------------------- */
/* --- TM1 SCALING FACTORS --- */
/* --------------------------- */



/* --- standard scale -- */

const int SUBSCR_WITH_1_AI = 25;
const int SUBSCR_WITH_2_AI = 25;
const int SUBSCR_WITH_3_AI = 25;
const int SUBSCR_WITH_4_AI = 25;

const int SUBSCR_WITH_1_SF = 25;
const int SUBSCR_WITH_2_SF = 25;
const int SUBSCR_WITH_3_SF = 25;
const int SUBSCR_WITH_4_SF = 25;

const int PROB_ACTIVE_SF_YES = 85;
const int PROB_ACTIVE_SF_NO  = 15;

const int SF_WITH_0_CALL_FWD = 25;
const int SF_WITH_1_CALL_FWD = 25;
const int SF_WITH_2_CALL_FWD = 25;
const int SF_WITH_3_CALL_FWD = 25;


/* --- number of fields per table --- */

const int TM1_SUBSCRIBERS_FCOUNT       = 34;
const int TM1_ACCESS_INFO_FCOUNT       = 6;
const int TM1_SPECIAL_FACILITY_FCOUNT  = 6;
const int TM1_CALL_FORWARDING_FCOUNT   = 5;



/* --------------------------- */
/* --- TM1 TRANSACTION MIX --- */
/* --------------------------- */

const int XCT_TM1_MIX              = 20;
const int XCT_TM1_GET_SUBSCR_DATA  = 21;
const int XCT_TM1_GET_NEW_DEST     = 22;
const int XCT_TM1_GET_ACCESS_DATA  = 23;
const int XCT_TM1_UPD_SUBSCR_DATA  = 24;
const int XCT_TM1_UPD_LOCATION     = 25;
const int XCT_TM1_INS_CALL_FWD     = 26;
const int XCT_TM1_DEL_CALL_FWD     = 27;



const int XCT_TM1_DORA_MIX              = 120;
const int XCT_TM1_DORA_GET_SUBSCR_DATA  = 121;
const int XCT_TM1_DORA_GET_NEW_DEST     = 122;
const int XCT_TM1_DORA_GET_ACCESS_DATA  = 123;
const int XCT_TM1_DORA_UPD_SUBSCR_DATA  = 124;
const int XCT_TM1_DORA_UPD_LOCATION     = 125;
const int XCT_TM1_DORA_INS_CALL_FWD     = 126;
const int XCT_TM1_DORA_DEL_CALL_FWD     = 127;



/* --- probabilities for the TM1 MIX --- */

// READ-ONLY (80%)
const int PROB_TM1_GET_SUBSCR_DATA  = 35;
const int PROB_TM1_NEW_DEST         = 10;
const int PROB_TM1_ACCESS_DATA      = 35;

// UPDATE (20%)
const int PROB_TM1_UPD_SUBSCR_DATA  = 2;
const int PROB_TM1_UPD_LOCATION     = 14;
const int PROB_TM1_INS_CALL_FWD     = 2;
const int PROB_TM1_DEL_CALL_FWD     = 2;


/* --- Helper functions --- */


/* --- translates or picks a random xct type given the benchmark specification --- */

int random_xct_tm1_type(int selected);


EXIT_NAMESPACE(tm1);

#endif /* __TM1_CONST_H */
