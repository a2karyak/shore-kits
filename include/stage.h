/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _STAGE_H
#define _STAGE_H

#include "tuple.h"
#include "packet.h"



// include me last!!!
#include "lib/namespace.h"



/* exported datatypes */


/**
 *  @brief A QPIPE stage is a queue of packets (work that must be
 *  completed) and a process_next_packet() function that worker
 *  threads can call to process the packets.
 */
class stage_t {


public:

    typedef enum {
	RESULT_CONTINUE = 0,
	RESULT_STOP,
	RESULT_ERROR
    } result_t;


    /**
     *  @brief The purpose of a stage::adaptor_t is to provide a
     *  stage's process packet method with exactly the functionality
     *  it needs to read from the primary packet and send results out
     *  to every packet in the stage.
     */
    struct adaptor_t {

    private:

        page_guard_t _page;

    public:
        
	virtual const char* get_container_name()=0;
        virtual packet_t* get_packet()=0;
        virtual result_t output(tuple_page_t *page)=0;
	virtual void stop_accepting_packets()=0;	
        virtual bool check_for_cancellation()=0;
        
        /**
         *  @brief Write a tuple to each waiting output buffer in a
         *  chain of packets.
         *
         *  @return OUTPUT_RETURN_CONTINUE to indicate that we should continue
         *  processing the query. OUTPUT_RETURN_STOP if all packets have been
         *  serviced, sent EOFs, and deleted. OUTPUT_RETURN_ERROR on
         *  unrecoverable error. process() should probably propagate this
         *  error up.
         */
        result_t output(const tuple_t &tuple) {
            assert(!_page->full());
            _page->append_init(tuple);
            if(_page->full()) {
                result_t result = output(_page);
                _page->clear();
                return result;
            }
    
            return stage_t::RESULT_CONTINUE;
        }
        
        adaptor_t(tuple_page_t *page)
            : _page(page)
        {
            assert(_page);
        }
        
        virtual ~adaptor_t() { }
        
    protected:
        /**
         * @brief outputs the last partial page, if any. The adaptor's
         * implementation should call this function after normal
         * processing has completed (stage implementations need not
         * concern themselves with this).
         */
        result_t flush() {
            if(_page->empty())
                return RESULT_STOP;

            return output(_page);
        }
        
    };


protected:

    adaptor_t* _adaptor;

    virtual result_t process_packet()=0;

public:

    stage_t()
	: _adaptor(NULL)
    {
    }

    virtual ~stage_t() { }
    

    void init(adaptor_t* adaptor) {
	_adaptor = adaptor;
    }    


    /**
     *  @brief Process this packet. The stage container must invoke
     *  init_stage() with an adaptor that we can use.
     *
     *  @return 0 on success. Non-zero value on unrecoverable
     *  error. If this function returns error, the stage will
     *  terminate all queries that it is currently involved in
     *  computing.
     */
    result_t process() {
	assert(_adaptor != NULL);
	return process_packet();
    }
};



#include "lib/namespace.h"
#endif
