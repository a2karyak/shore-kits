/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-MT -- Multi-threaded port of the SHORE storage manager
   
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

/* -*- mode:c++; c-basic-offset:4 -*- */

#ifndef _MEASUREMENTS_H
#define _MEASUREMENTS_H

#include <time.h>
#include <cassert>


ENTER_NAMESPACE(workload);



/**
 *  @class execution_time_t
 *
 *  @brief The total running time for something in our system.
 */
class execution_time_t {

private:

    bool   _is_active;
    bool   _is_stopped;
    time_t _time_start;
    time_t _time_finish;

public:

    execution_time_t() {
        reset(this);
    }


    static void reset(execution_time_t* t) {
        t->_is_active  = false;
        t->_is_stopped = false;
    }


    void start() {

        // error checking
        assert( !_is_active );

        time_t t = ::time(NULL);
        assert( t != (time_t)-1 );

        _time_start = t;
        _is_active = true;
    }


    void stop() {
        
        // error checking
        assert( _is_active && !_is_stopped );
        
        time_t t = ::time(NULL);
        assert( t != (time_t)-1 );
        
        _time_finish = t;
        _is_stopped = true;
    }


    double time() {

        // error checking
        assert( _is_active && _is_stopped );

        return difftime(_time_finish, _time_start);
    }
    
};


EXIT_NAMESPACE(workload);


#endif	// _MEASUREMENTS_H
