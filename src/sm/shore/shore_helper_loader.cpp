/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_helper_loader.cpp
 *
 *  @brief Declaration of helper loader thread classes
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "sm/shore/shore_helper_loader.h"
#include "sm/shore/shore_env.h"


using namespace shore;


/** Exported functions */



/****************************************************************** 
 *
 * class db_init_smt_t
 *
 ******************************************************************/

void db_init_smt_t::work()
{
    if (!_env->is_initialized()) {
        if (_env->init()) {
            // Couldn't initialize the Shore environment
            // cannot proceed
            TRACE( TRACE_ALWAYS, "Couldn't initialize Shore...\n");
            assert (false);
            return;
        }
    }

    // if reached this point everything went ok
    TRACE( TRACE_DEBUG, "Shore initialized...\n");
    _rv = 0;
}


/****************************************************************** 
 *
 * class table_loading_smt_t
 *
 ******************************************************************/

void table_loading_smt_t::work()
{
    char fname[MAX_FILENAME_LEN];
    strcpy(fname, _datadir);
    strcat(fname, "/");
    strcat(fname, _ptable->name());
    strcat(fname, ".dat");
    TRACE( TRACE_DEBUG, "Loading (%s) from (%s)\n", 
           _ptable->name(), fname);
    time_t ttablestart = time(NULL);
    w_rc_t e = _ptable->load_from_file(_pssm, fname);
    time_t ttablestop = time(NULL);
    if (e != RCOK) {
        TRACE( TRACE_ALWAYS, "Error while loading (%s) *****\n",
               _ptable->name());
        _rv = 1;
        return;
    }

    TRACE( TRACE_ALWAYS, "Table (%s) loaded in (%d) secs...\n",
           _ptable->name(), (ttablestop - ttablestart));
    _rv = 0;
}


/****************************************************************** 
 *
 * class index_loading_smt_t
 *
 ******************************************************************/

void index_loading_smt_t::work()
{
    w_rc_t e = do_help();
    if (e) {
        TRACE( TRACE_ALWAYS, "Index (%s) loading aborted [0x%x]\n", 
               _pindex->name(), e.err_num());
       
        int iretries = 0;
        w_rc_t abrt_rc = _pssm->abort_xct();
        
        while (!abrt_rc) {
            iretries++;
            abrt_rc = _pssm->abort_xct();
            if (iretries > SHORE_NUM_OF_RETRIES)
                break;
        }

        _rv = 1;
        return;
    }

    // the do_help() function should have exited when _finish was set to true
    assert (_finish); 
 

    // if reached this point everything was ok
    _rv = 0;
}


w_rc_t index_loading_smt_t::do_help()
{
    char* pdest  = NULL;
    int   bufsz  = 0;
    int   key_sz = 0;
    int   mark   = COMMIT_ACTION_COUNT;
    bool  cons_happened = false;
    int   ispin  = 0;

    CRITICAL_SECTION(hcs, &_cs_mutex);
    hcs.pause();

    while(!_start) {
        ispin++;
    }

    W_DO(_pssm->begin_xct());    
    W_DO(_pindex->find_fid(_pssm));
    
    while (true) {

        //*** START: CS ***//
        hcs.resume();

        if (_has_to_consume) {
            // if new row waiting

            // if signalled to finish
            if (_finish)
                break;

            //*** CONSUME ***//

            //            _prow->print_tuple();
        
            key_sz = _ptable->format_key(_pindex, _prow, *_prow->_rep);
            assert (pdest); // (ip) if NULL invalid key
            
            W_DO(_pssm->create_assoc(_pindex->fid(),
                                     vec_t(pdest, key_sz),
                                     vec_t(&(_prow->_rid), sizeof(rid_t))));
            
            _has_to_consume = false;
            cons_happened = true; // a consumption just happened
        }

        hcs.pause();
        //*** EOF: CS ***//

        if (cons_happened) {
            // It just consumed a row, increase the counters
            _t_count++;
            
            if (_t_count >= mark) { 
                W_DO(_pssm->commit_xct());

                if ((_t_count % 100000) == 0) { // every 100K
                    TRACE( TRACE_ALWAYS, "index(%s): %d\n", _pindex->name(), _t_count);
                }
                else {
                    TRACE( TRACE_TRX_FLOW, "index(%s): %d\n", _pindex->name(), _t_count);
                }

                W_DO(_pssm->begin_xct());
                mark += COMMIT_ACTION_COUNT;
            }
            cons_happened = false;
        }
    }
    // final commit
    W_DO(_pssm->commit_xct());

//     TRACE( TRACE_ALWAYS, "Checkpointing\n");
//     W_DO(_pssm->checkpoint());

    // if we reached this point everything went ok
    return (RCOK);
}


/****************************************************************** 
 *
 * class table_checking_smt_t
 *
 ******************************************************************/

void table_checking_smt_t::work()
{
    TRACE( TRACE_DEBUG, "Checking (%s)\n", _ptable->name());
    if (!_ptable->check_all_indexes(_pssm)) {
        TRACE( TRACE_DEBUG, "Inconsistency in (%s)\n", _ptable->name());
    }
    else {
        TRACE( TRACE_DEBUG, "(%s) OK...\n", _ptable->name());
    }
}

