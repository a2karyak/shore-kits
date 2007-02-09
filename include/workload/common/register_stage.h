// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _REGISTER_STAGE_H
#define _REGISTER_STAGE_H



#include "core.h"


using namespace qpipe;

/**
 * A wrapper to allow us to run multiple threads off of one stage
 * container. It might be possible to make stage_container_t inherit
 * from thread_t and instantiate it directly multiple times, but
 * thread-local state like the RNG would be too risky to use.
 */
struct stage_thread : thread_t {
    stage_container_t* _sc;
    stage_thread(const c_str &name, stage_container_t* sc)
        : thread_t(name), _sc(sc)
    {
    }
    virtual void* run() {
        _sc->run();
        return NULL;
    }
};

template <class Stage>
void register_stage(int worker_threads=10) {
    stage_container_t* sc;
    c_str name("%s_CONTAINER", Stage::DEFAULT_STAGE_NAME.data());
    sc = new stage_container_t(name, new stage_factory<Stage>);
    dispatcher_t::register_stage_container(Stage::stage_packet_t::PACKET_TYPE.data(), sc);

    /* This is qpipe-plain. Don't create any worker threads. */
    worker_threads = 0;

    // fire up the worker threads
    for(int i=0; i < worker_threads; i++) {
        thread_t* thread = new stage_thread(c_str("%s_THREAD_%d", name.data(), i), sc);
        thread_create(thread);
    }
}



#endif
