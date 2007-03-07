// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _REGISTER_STAGE_H
#define _REGISTER_STAGE_H



#include "core.h"


using namespace qpipe;

template <class Stage>
void register_stage(int worker_threads=10, bool osp=true) {
    stage_container_t* sc;
    c_str name("%s_CONTAINER", Stage::DEFAULT_STAGE_NAME.data());
    sc = new stage_container_t(name, new stage_factory<Stage>, worker_threads);
    dispatcher_t::register_stage_container(Stage::stage_packet_t::PACKET_TYPE.data(), sc, osp);
}



#endif
