/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   action.cpp
 *
 *  @brief:  Implementation of Actions for DORA
 *
 *  @author: Ippokratis Pandis, Nov 2008
 */


#include "dora/action.h"


ENTER_NAMESPACE(dora);


// copying allowed
base_action_t& base_action_t::operator=(const base_action_t& rhs)
{
  if (this != &rhs) {
    _prvp = rhs._prvp;
    _xct = rhs._xct;
    _tid = rhs._tid;
    _keys_needed = rhs._keys_needed;
  }
  return (*this);
}



EXIT_NAMESPACE(dora);
