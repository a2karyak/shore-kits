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

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_iter.h
 *
 *  @brief:  Base class for iterators over certain set of tuples
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_ITER_H
#define __SHORE_ITER_H

#include "sm_vas.h"
#include "util.h"

#include "sm/shore/shore_error.h"


ENTER_NAMESPACE(shore);


/****************************************************************** 
 *  @class: tuple_iter
 *
 *  @brief: Abstract class which is the base for table and index 
 *          scan iterators (tscan - iscan).
 *
 ******************************************************************/

template <class file_desc, class file_scanner, class rowpointer>
class tuple_iter_t 
{
protected:
    ss_m*         _db;
    bool          _opened;  /* whether the init is successful */
    file_desc*    _file;
    file_scanner* _scan;

    lock_mode_t   _lm;
    bool          _shoulddelete;    
    
public:
    // Constructor
    tuple_iter_t(ss_m* db, file_desc* file, 
                 lock_mode_t alm, bool shoulddelete)
        //                 lock_mode_t alm = SH, bool shoulddelete = false)
        : _db(db), _opened(false), _file(file), _scan(NULL), 
          _lm(alm), _shoulddelete(shoulddelete)
    { 
        assert (_db);
    }

    virtual ~tuple_iter_t() { close_scan(); }

    // Access methods
    bool opened() const { return (_opened); }


    /* ------------------------- */
    /* --- iteration methods --- */
    /* ------------------------- */

    // virtual w_rc_t open_scan()=0; 

    virtual w_rc_t next(ss_m* db, bool& eof, rowpointer& tuple)=0;

    w_rc_t close_scan() {
        if ((_opened) && (_shoulddelete)) { 
            // be careful: if <file_scanner> is scalar, it should NOT be deleted!
            assert (_scan);
            delete (_scan);
        }
        _opened = false;
        return (RCOK);
    }    

}; // EOF: tuple_iter_t



EXIT_NAMESPACE(shore);

#endif /* __SHORE_ITER_H */
