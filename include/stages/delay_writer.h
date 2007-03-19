/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef _DELAY_WRITER_H
#define _DELAY_WRITER_H

#include "core.h"



using namespace qpipe;



/**
 *  @brief Packet definition for the DELAY_WRITER stage.
 */

struct delay_writer_packet_t : public packet_t {

public:

    static const c_str PACKET_TYPE;


    const size_t _output_tuple_size;
    const int _delay_ms;
    const int _num_tuples;

   
    /**
     *  @brief delay_writer_packet_t constructor.
     */
    delay_writer_packet_t(const c_str&    packet_id,
                          tuple_fifo*     output_buffer,
                          tuple_filter_t* output_filter,
                          int             delay_ms,
                          int             num_tuples)
	: packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                   NULL,
                   false, /* merging not allowed */
                   true   /* unreserve worker on completion */
                   ),
	  _output_tuple_size(output_buffer->tuple_size()),
          _delay_ms(delay_ms),
          _num_tuples(num_tuples)
    {
    }

    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        /* no inputs */
    }
};



/**
 *  @brief Table scan stage that reads tuples from the storage manager.
 */

class delay_writer_stage_t : public stage_t {

public:

    typedef delay_writer_packet_t stage_packet_t;
    static const c_str DEFAULT_STAGE_NAME;

    static const size_t DELAY_WRITER_BULK_READ_BUFFER_SIZE;

    delay_writer_stage_t() { }

    virtual ~delay_writer_stage_t() { }

protected:

    virtual void process_packet();
};



#endif
