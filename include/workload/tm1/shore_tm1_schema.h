/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_tm1_schema.h
 *
 *  @brief:  Declaration of the Telecom One (TM1) benchmark tables
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */

#ifndef __SHORE_TM1_SCHEMA_H
#define __SHORE_TM1_SCHEMA_H

#include <math.h>

#include "sm_vas.h"
#include "util.h"

#include "workload/tm1/tm1_const.h"

#include "sm/shore/shore_table_man.h"


using namespace shore;


ENTER_NAMESPACE(tm1);



/*********************************************************************
 *
 * TM1 SCHEMA
 * 
 * This file contains the classes for tables in the TM1 benchmark.
 *
 *********************************************************************/


/*
 * indices created on the tables are:
 *
 * 1. unique index on subscriber(s_id)
 * 2. index on subscriber(s_sub_nbr)
 * 3. unique index on access_info(s_id,ai_type)
 * 4. unique index on special_facility(s_id,sf_type)
 * 5. unique index on call_forwarding(s_id,sf_type,start_time)
 */



/* ------------------------------------------------ */
/* --- All the tables used in the TM1 benchmark --- */
/* ------------------------------------------------ */


class subscriber_t : public table_desc_t 
{
public:
    subscriber_t(string sysname) : 
        table_desc_t("SUBSCRIBER", TM1_SUBSCRIBERS_FCOUNT) 
    {
        /* table schema */
        _desc[0].setup(SQL_INT,         "S_ID");         // UNIQUE [1..SF]
        _desc[1].setup(SQL_CHAR,        "SUB_NBR", 15);  
        _desc[2].setup(SQL_BIT,         "BIT_1");        // BIT (0,1)
        _desc[3].setup(SQL_BIT,         "BIT_2");
        _desc[4].setup(SQL_BIT,         "BIT_3");
        _desc[5].setup(SQL_BIT,         "BIT_4");
        _desc[6].setup(SQL_BIT,         "BIT_5");
        _desc[7].setup(SQL_BIT,         "BIT_6");
        _desc[8].setup(SQL_BIT,         "BIT_7");
        _desc[9].setup(SQL_BIT,         "BIT_8");
        _desc[10].setup(SQL_BIT,        "BIT_9");
        _desc[11].setup(SQL_BIT,        "BIT_10");
        _desc[12].setup(SQL_SMALLINT,   "HEX_1");        // SMALLINT (0,15)
        _desc[13].setup(SQL_SMALLINT,   "HEX_2");
        _desc[14].setup(SQL_SMALLINT,   "HEX_3");
        _desc[15].setup(SQL_SMALLINT,   "HEX_4");
        _desc[16].setup(SQL_SMALLINT,   "HEX_5");
        _desc[17].setup(SQL_SMALLINT,   "HEX_6");
        _desc[18].setup(SQL_SMALLINT,   "HEX_7");
        _desc[19].setup(SQL_SMALLINT,   "HEX_8");
        _desc[20].setup(SQL_SMALLINT,   "HEX_9");
        _desc[21].setup(SQL_SMALLINT,   "HEX_10");
        _desc[22].setup(SQL_SMALLINT,   "BYTE2_1");      // SMALLINT (0,255)
        _desc[23].setup(SQL_SMALLINT,   "BYTE2_2");
        _desc[24].setup(SQL_SMALLINT,   "BYTE2_3");
        _desc[25].setup(SQL_SMALLINT,   "BYTE2_4");
        _desc[26].setup(SQL_SMALLINT,   "BYTE2_5");
        _desc[27].setup(SQL_SMALLINT,   "BYTE2_6");
        _desc[28].setup(SQL_SMALLINT,   "BYTE2_7");
        _desc[29].setup(SQL_SMALLINT,   "BYTE2_8");
        _desc[30].setup(SQL_SMALLINT,   "BYTE2_9");
        _desc[31].setup(SQL_SMALLINT,   "BYTE2_10");
        _desc[32].setup(SQL_INT,        "MSC_LOCATION"); // INT (0,2^32-1)
        _desc[33].setup(SQL_INT,        "VLR_LOCATION");

        int keys1[1] = { 0 }; // IDX { S_ID }

        int keys2[2] = { 1 }; // IDX { SUB_NBR }


        // depending on the system name, create the corresponding indexes 
        int idxs_created = 0;

        // baseline - regular indexes
        //        if (sysname.compare("baseline")==0) {

        assert (idxs_created==0);
        idxs_created=1;
        TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);
        
        // create unique index s_index on (s_id)
        create_primary_idx("S_INDEX", 0, keys1, 1);


        // create index sub_nbr_index on (sub_nbr)
        create_index("SUB_NBR_INDEX", 0, keys2, 2, false);
    }

}; // EOF: subscriber_t



class access_info_t : public table_desc_t 
{
public:
    access_info_t(string sysname) : 
        table_desc_t("ACCESS_INFO", TM1_ACCESS_INFO_FCOUNT) 
    {
        /* table schema */
        _desc[0].setup(SQL_INT,        "S_ID");       // REF S.S_ID
        _desc[1].setup(SQL_SMALLINT,   "AI_TYPE");    // SMALLINT (1,4)   - (AI.S_ID,AI.AI_TYPE) is PRIMARY KEY
        _desc[2].setup(SQL_SMALLINT,   "DATA1");      // SMALLINT (0,255)
        _desc[3].setup(SQL_SMALLINT,   "DATA2");     
        _desc[4].setup(SQL_CHAR,       "DATA3", 3);   // CHAR (5). [A-Z]
        _desc[5].setup(SQL_CHAR,       "DATA4", 5);   // CHAR (3). [A-Z]

        // There are between 1 and 4 Acess_Info records per Subscriber.
        // 25% Subscribers with one record
        // 25% Subscribers with two records
        // etc...


        int keys[2] = { 0, 1 }; // IDX { S_ID, AI_TYPE }

        // depending on the system name, create the corresponding indexes 
        int idxs_created = 0;

        // baseline - regular indexes
        //        if (sysname.compare("baseline")==0) {
        assert (idxs_created==0);
        idxs_created=1;
        TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);

        // create unique index ai_index on (s_id, ai_type)
        create_primary_idx("AI_INDEX", 0, keys, 2);

        assert (idxs_created==1); // make sure that idxs were created
    }

}; // EOF: access_info_t




class special_facility_t : public table_desc_t 
{
public:
    special_facility_t(string sysname) : 
        table_desc_t("SPECIAL_FACILITY", TM1_SPECIAL_FACILITY_FCOUNT) 
    {
        /* table schema */
        _desc[0].setup(SQL_INT,        "S_ID");         // REF S.S_ID
        _desc[1].setup(SQL_SMALLINT,   "SF_TYPE");      // SMALLINT (1,4). (SF.S_ID,SF.SF_TYPE) PRIMARY KEY
        _desc[2].setup(SQL_BIT,        "IS_ACTIVE");    // BIT (0,1). 85% is 1 - 15% is 0
        _desc[3].setup(SQL_SMALLINT,   "ERROR_CNTRL");  // SMALLINT (0,255)
        _desc[4].setup(SQL_SMALLINT,   "DATA_A");       
        _desc[5].setup(SQL_CHAR,       "DATA_B", 5);    // CHAR (5) [A-Z] 

        // There are between 1 and 4 Special_Facility records per Subscriber.
        // 25% Subscribers with one sf
        // 25% Subscribers with two sf
        // etc...

        int keys[2] = { 0, 1 }; // IDX { S_ID, SF_TYPE }

        // depending on the system name, create the corresponding indexes 
        int idxs_created = 0;

        // baseline - regular indexes
        //        if (sysname.compare("baseline")==0) {
        assert (idxs_created==0);
        idxs_created=1;
        TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);

        // create unique index d_index on (s_id, sf_type)
        create_primary_idx("SF_INDEX", 0, keys, 2);

        assert (idxs_created==1); // make sure that idxs were created
    }

}; // EOF: special_facility_t



class call_forwarding_t : public table_desc_t 
{
public:
    call_forwarding_t(string sysname) : 
        table_desc_t("CALL_FORWARDING", TM1_CALL_FORWARDING_FCOUNT) 
    {
        /* table schema */
        _desc[0].setup(SQL_INT,        "S_ID");        // REF SF.S_ID
        _desc[1].setup(SQL_SMALLINT,   "SF_TYPE");     // REF SF.SF_TYPE
        _desc[2].setup(SQL_SMALLINT,   "START_TIME");  // SMALLINT {0,8,16}
        _desc[3].setup(SQL_SMALLINT,   "END_TIME");    // SMALLINT START_TIME + URAND(1,8)
        _desc[4].setup(SQL_CHAR,       "NUMBERX", 15); // CHAR (15) [0-9]

        int keys[3] = { 0, 1, 2 }; // IDX { S_ID, SF_TYPE, START_TIME }

        // depending on the system name, create the corresponding indexes 
        int idxs_created = 0;

        // baseline - regular indexes
        //        if (sysname.compare("baseline")==0) {
        assert (idxs_created==0);
        idxs_created=1;
        TRACE( TRACE_DEBUG, "Regular idxs for (%s)\n", _name);

        // create unique index d_index on (s_id, sf_type, start_time)
        create_primary_idx("CF_INDEX", 0, keys, 3);

        assert (idxs_created==1); // make sure that idxs were created
    }

}; // EOF: call_forwarding_t



// loaders
typedef table_loading_smt_impl<subscriber_t>       s_loader_t;
typedef table_loading_smt_impl<access_info_t>      ai_loader_t;
typedef table_loading_smt_impl<special_facility_t> sf_loader_t;
typedef table_loading_smt_impl<call_forwarding_t>  cf_loader_t;

// checkers
typedef table_checking_smt_impl<subscriber_t>       s_checker_t;
typedef table_checking_smt_impl<access_info_t>      ai_checker_t;
typedef table_checking_smt_impl<special_facility_t> sf_checker_t;
typedef table_checking_smt_impl<call_forwarding_t>  cf_checker_t;


EXIT_NAMESPACE(tm1);


#endif /* __SHORE_TM1_SCHEMA_H */
