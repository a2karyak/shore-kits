// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _REGISTER_STAGE_H
#define _REGISTER_STAGE_H



#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/dispatcher.h"
#include "qpipe_panic.h"
#include "trace.h"

#include <string>


using namespace qpipe;
using std::string;



template <class Stage>
void register_stage(int worker_threads=10) {
    stage_container_t* sc;
    string name = string(Stage::DEFAULT_STAGE_NAME) + "_CONTAINER";
    sc = new stage_container_t(name.c_str(), new stage_factory<Stage>);
    dispatcher_t::register_stage_container(Stage::stage_packet_t::PACKET_TYPE, sc);

    name = string(Stage::DEFAULT_STAGE_NAME) + "_THREAD_";
    for(int i=0; i < worker_threads; i++) {
        char num[5];
        sprintf(num, "%d", i);
        string thread_name = name + num;
	tester_thread_t* thread;
        thread = new tester_thread_t(drive_stage, sc, thread_name.c_str());
	if ( thread_create(NULL, thread) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }
}



#endif
