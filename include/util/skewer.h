/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/** @file skewer.h
 *
 *  @brief Definition for the class skewer
 *
 *  @author: Pinar Tozun, Feb 2011
 */

#ifndef __UTIL_SKEWER_H
#define __UTIL_SKEWER_H

#include <iostream>
#include <vector>

/* ---------------------------------------------------------------
 *
 * @enum:  skew_type_t
 *
 * @brief: There are different options for handling dynamic skew
 *         SKEW_NONE      - 
 *         SKEW_NORMAL    - TODO: pin: explain these
 *         SKEW_DYNAMIC   -
 *         SKEW_CHAOTIC   -
 *
 * --------------------------------------------------------------- */

enum skew_type_t {
    SKEW_NONE      = 0x0,
    SKEW_NORMAL    = 0x1,
    SKEW_DYNAMIC   = 0x2,
    SKEW_CHAOTIC   = 0x4
};

using namespace std;

/*********************************************************************
 * 
 * @class skewer_t
 *
 * @brief Class that keeps the skew information for creating dynamic skew
 *        With zipf we can just create a static skew on some area
 *
 *********************************************************************/

// TODO: pin: add comments here and .cpp 
class skewer_t {
  
private:
  
    int _hot_area;
    int _lower;
    int _upper;
    int _load_imbalance;
    
    vector<int> _interval_l;
    vector<int> _interval_u;
    
public:

    skewer_t() { }
    
    void set(int hot_area, int lower, int upper, int load_imbalance);

    void clear();
    
    void reset(skew_type_t type);
    
    bool get_input(int& input);

    bool is_set();
    
private:

    void _set_intervals();
    
    void _add_interval(int interval_lower, int interval_upper);
    
};

#endif // __UTIL_SKEW_INFO_H
