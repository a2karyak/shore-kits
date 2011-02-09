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

/** @file skewer.cpp
 *
 *  @brief Implementation for the class skewer_t
 *
 *  @author: Pinar Tozun, Feb 2011
 */

#include "util/skewer.h"
#include "util/random_input.h"

/*********************************************************************
 * 
 * @class skewer
 *
 * @brief Class that keeps the skew information for creating dynamic skew
 *        With zipf we can just create a static skew on some area
 *
 *********************************************************************/

void skewer_t::set(int hot_area, int lower, int upper, int load_imbalance)
{
    _hot_area = hot_area;
    _lower = lower;
    _upper = upper;
    _load_imbalance = load_imbalance;
    _set_intervals();
}

void skewer_t::clear() {
    _interval_l.clear();
    _interval_u.clear();
}

void skewer_t::reset(skew_type_t type) {
    clear();
    if(type == SKEW_CHAOTIC) {
	_load_imbalance = URand(0,100);
	_hot_area = URand(1,100);
    }
    _set_intervals();
}

bool skewer_t::is_set() {
    return (_interval_u.size() > 0);
}

void skewer_t::_set_intervals() {
    int interval_lower = 0;
    int interval_upper = 0;
    int middle = URand(_lower,_upper);
    int interval = ceil(((double) (_upper * _hot_area))/100);
    
    if(URand(1,100) < 70) { // a continuous hot spot
	interval_lower = middle - interval / 2;
	interval_upper = middle + interval / 2;
	_add_interval(interval_lower, interval_upper);
    } else { // for non continious several hot spots
	int middle_2 = URand(_lower,_upper);
	interval_lower = middle - interval / 4;
	interval_upper = middle + interval / 4;
	int interval_lower_2 = middle_2 - interval / 4;
	int interval_upper_2 = middle_2 + interval / 4;
	_add_interval(interval_lower, interval_upper);
	_add_interval(interval_lower_2, interval_upper_2);
    }
}

void skewer_t::_add_interval(int interval_lower, int interval_upper) {
    if(interval_lower < _lower) {
	_interval_l.push_back(_lower);
	_interval_u.push_back(interval_upper);
	_interval_l.push_back(_upper + interval_lower);
	_interval_u.push_back(_upper);
    } else if(interval_upper > _upper) {
	_interval_l.push_back(_lower);
	_interval_u.push_back(interval_upper - _upper);
	_interval_l.push_back(interval_lower);
	_interval_u.push_back(_upper);
    } else {
	_interval_l.push_back(interval_lower);
	_interval_u.push_back(interval_upper);
    }
}

bool skewer_t::get_input(int& input) {
    int rand = URand(1,100);
    int load = _load_imbalance / _interval_u.size();
    for(int i=0; i<_interval_u.size(); i++) {
	if(rand < load * (i+1)) {
	    input = UZRand(_interval_l[i],_interval_u[i]);
	    return true;
	}
    }
}
