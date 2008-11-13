/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_client_env.cpp
 *
 *  @brief:  Implementation base class for Shore clients
 *
 *  @author: Ippokratis Pandis, July 2008
 */

#include "sm/shore/shore_client.h"

ENTER_NAMESPACE(shore);


/********************************************************************* 
 *
 *  @fn:    abort/resume_test
 *
 *  @brief: Used for SIGINT functionality
 *
 *********************************************************************/

static bool _abort_test = false;
void base_client_t::abort_test() {
    _abort_test = true;
}
void base_client_t::resume_test() {
    _abort_test = false;
}



/********************************************************************* 
 *
 *  @fn:    submit_batch
 *
 *  @brief: Submits a batch of trxs and always uses the last trx to
 *          wait on its cond var
 *
 *********************************************************************/

void base_client_t::submit_batch(int xct_type, int& trx_cnt, const int batch_sz) 
{       
    assert (batch_sz);
    assert (_cp);
    for(int j=1; j <= batch_sz; j++) {
        if (j == batch_sz) {
            _cp->take_one = true;
            _cp->index = 1-_cp->index;
        }
        run_one_xct(xct_type, trx_cnt++);
        _cp->take_one = false;
    }
}


/********************************************************************* 
 *
 *  @fn:    run_xcts
 *
 *  @brief: Entry point for running the trxs 
 *
 *********************************************************************/

w_rc_t base_client_t::run_xcts(int xct_type, int num_xct)
{
    int i=0;
    int batchsz=1;

    switch (_measure_type) {

        // case of number-of-trxs-based measurement
    case (MT_NUM_OF_TRXS):

        // submit a batch of num_xct trxs
        submit_batch(xct_type, i, num_xct);
        // wait for the batch to complete
        // note: in the test case there is no double buffering
        _cp->wait[_cp->index].wait();
        break;

        // case of duration-based measurement
    case (MT_TIME_DUR):

        // retrieve the default batch size
        batchsz = envVar::instance()->getVarInt("db-cl-batchsz",BATCH_SIZE);
	
	// submit the first two batches...
	submit_batch(xct_type, i, batchsz);
	submit_batch(xct_type, i, batchsz);

	// main loop
        while (true) {
	    // wait for the first to complete
	    _cp->wait[1-_cp->index].wait();

	    // check for exit...
	    if(_abort_test || _env->get_measure() == MST_DONE)
		break;

	    // submit a replacement batch...
	    submit_batch(xct_type, i, batchsz);
        }
	
	// wait for the last batch to complete...
	_cp->wait[_cp->index].wait();	
        break;

    default:
        assert (0); // UNSUPPORTED MEASUREMENT TYPE
        break;
    }
    return (RCOK);
}


EXIT_NAMESPACE(shore);





