/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _FUNC_CALL_H
#define _FUNC_CALL_H

#include <cstdio>

#include "core.h"



using namespace qpipe;



/* exported datatypes */


class func_call_packet_t : public packet_t {
  
public:

    static const c_str PACKET_TYPE;
    
    
    void (*_func) (void*, void*);
    
    
    void* _func_arg;
    
    
    void (*_destructor) (void*);

    
    /**
     *  @brief func_call_packet_t constructor.
     *
     *  @param packet_id The ID of this packet. This should point to a
     *  block of bytes allocated with malloc(). This packet will take
     *  ownership of this block and invoke free() when it is
     *  destroyed.
     *
     *  @param output_buffer This packet's output buffer. This buffer
     *  is not used directly. The container will send_eof() and close
     *  this buffer when process_packet() returns. However, this will
     *  not be done until func is called with func_arg. So a test
     *  program may pass in a tuple_fifo for both output_buffer
     *  and func_arg. A packet DOES NOT own its output buffer (we will
     *  not invoke delete or free() on this field in our packet
     *  destructor).
     *
     *  @param output_filter This filter is not used, but it cannot be
     *  NULL. Pass in a tuple_filter_t instance. The packet OWNS this
     *  filter. It will be deleted in the packet destructor.
     *
     *  @param func The function that should be called with func_arg.
     *
     *  @param func_arg func will be called with this value. This
     *  packet does not own this value.
     */
    func_call_packet_t(const c_str    &packet_id,
                       tuple_fifo* output_buffer,
                       tuple_filter_t* output_filter,
                       void (*func) (void*, void*),
                       void* func_arg,
                       void (*destructor) (void*) = NULL,
                       bool _merge=false)
        : packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                   _merge ? create_plan(output_filter, func) : NULL,
                   _merge),
          _func(func),
          _func_arg(func_arg),
          _destructor(destructor)
    {
        // error checking
        assert(func != NULL);
    }

    
    virtual ~func_call_packet_t() {
        // if a destructor was specified, apply it to _func_arg field
        if ( _destructor != NULL )
            _destructor(_func_arg);
    }

    static query_plan* create_plan(tuple_filter_t* filter, void (*func) (void*, void*)) {
        c_str action("%p", func);
        return new query_plan(action, filter->to_string(), NULL, 0);
    }
    
    virtual bool is_compatible(packet_t* other) {
        // enforce the OSP_SCAN policy (attempt to merge compatible
        // packets unless OSP_NONE prevents this from being called in
        // the first place)
        return packet_t::is_compatible(plan(), other->plan());
    }

};



/**
 *  @brief FUNC_CALL stage. Simply invokes a packet's specified
 *  function with the packet's tuple buffer. Useful for unit testing.
 */
class func_call_stage_t : public stage_t {

public:
  
    static const c_str DEFAULT_STAGE_NAME;
    typedef func_call_packet_t stage_packet_t;
    
    func_call_stage_t() { }
    
    virtual ~func_call_stage_t() { }
    
protected:

    virtual void process_packet();
};



#endif
