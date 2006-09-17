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

 public:
  
  tuple_source_once_t(packet_t* packet)
    : _packet(packet)
    {
      assert( packet != NULL );
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
