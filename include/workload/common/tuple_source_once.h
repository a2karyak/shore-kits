// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _TUPLE_SOURCE_ONCE_H
#define _TUPLE_SOURCE_ONCE_H

#include "stages.h"

using namespace qpipe;



ENTER_NAMESPACE(workload);



/**
 *  @brief Acts like a normal tuple source, but reset() may only be
 *  called once.
 */
class tuple_source_once_t : public tuple_source_t {
  
private:

    packet_t* _packet;
    size_t    _tuple_size;
    

public:
  
    tuple_source_once_t(packet_t* packet, size_t tuple_size)
        : _packet(packet),
          _tuple_size(tuple_size)
    {
        assert( packet != NULL );
    }

    virtual size_t tuple_size() {
        return _tuple_size;
    }

    virtual packet_t* reset() {
        assert ( _packet != NULL );
        packet_t* packet = _packet;
        _packet = NULL;
        return packet;
    }

};



EXIT_NAMESPACE(workload);



#endif
