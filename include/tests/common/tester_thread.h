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

/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TESTER_THREAD_H
#define __TESTER_THREAD_H

#include "util.h"



class tester_thread_t : public thread_t {

private:

    void (*start) (void*);
    void* start_arg;

public:

    void work() {
        start(start_arg);        
    }


    tester_thread_t(void (*start_routine)(void *), void* arg, const c_str &name)
        : thread_t(name),
          start(start_routine), start_arg(arg)
    {
    }

};



#endif /* __TESTER_THREAD_H */
