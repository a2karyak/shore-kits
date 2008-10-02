/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   lockman.cpp
 *
 *  @brief:  Implementation of a Lock manager for DORA partitions
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */

#include "dora/lockman.h"

using namespace dora;


// friend function
std::ostream& operator<<(std::ostream& os, const ll_entry& rhs)
{
    os << "lm (" << rhs._ll << ") - c (" << rhs._counter << ") ";
    return (os);
}
