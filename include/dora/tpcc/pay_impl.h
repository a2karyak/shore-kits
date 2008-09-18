/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file pay_impl.h
 *
 *  @brief tpcc-payment-related actions
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __DORA_PAY_IMPL_H
#define __DORA_PAY_IMPL_H

#include <cstdio>

#include "util.h"

#include "dora/action.h"

#include "stages/tpcc/common/tpcc_input.h"
#include "stages/tpcc/common/tpcc_trx_input.h"

#include "stages/tpcc/shore/shore_tpcc_env.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;


/******************************************************************** 
 *
 * @class: pay_upd_wh_impl
 *
 * @brief: payment update warehouse
 *
 ********************************************************************/

class pay_upd_wh_impl : public action_t<int>
{
private:

    ShoreTPCCEnv*   _ptpcc_env;
    payment_input_t _pin;

public:

    pay_upd_wh_impl(ShoreTPCCEnv* tpccenv, countdown_t* prvp, xct_t* pxct, 
                    payment_input_t* pin)
        : action_t<int>(tpccenv, 1, prvp, pxct), 
          _ptpcc_env(tpccenv), _pin(*pin)
    {
        assert (_ptpcc_env);
    }

    ~pay_upd_wh_impl() { 
    }

    
    /** trx-related operations */
    w_rc_t trx_exec();


private:

    // copying not allowed
    pay_upd_wh_impl(pay_upd_wh_impl const &);
    void operator=(pay_upd_wh_impl const &);
    
}; // EOF: pay_upd_wh_impl




EXIT_NAMESPACE(dora);

#endif /** __DORA_ACTION_H */

