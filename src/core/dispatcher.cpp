
#include "util.h"
#include "core/dispatcher.h"
#include "stages.h"

#include <cstdio>
#include <cstring>
#include <map>

using std::map;



ENTER_NAMESPACE(qpipe);


dispatcher_t* dispatcher_t::_instance = NULL;

pthread_mutex_t dispatcher_t::_instance_lock = thread_mutex_create();



dispatcher_t::dispatcher_t() { }



dispatcher_t::~dispatcher_t() {
  TRACE(TRACE_ALWAYS, "Need to destroy nodes and keys\n");
}



/**
 *  @brief THIS FUNCTION IS NOT THREAD-SAFE. It should not have to be
 *  since stages should register themselves in their constructors and
 *  their constructors should execute in the context of the root
 *  thread.
 */
void dispatcher_t::_register_stage_container(const c_str &packet_type,
                                             stage_container_t* sc)
{
  // We may eventually want multiple stages willing to perform
  // SORT's. But then we need policy/cost model to determine which
  // SORT stage to use when. For now, restrict to one stage per type.
  if ( _scdir[packet_type] != NULL )
    THROW2(DispatcherException,
           "Trying to register duplicate stage for type %s\n",
           packet_type.data());

  _scdir[packet_type] = sc;
}



/**
 *  @brief THIS FUNCTION IS NOT THREAD-SAFE. It should not have to be
 *  since stages should register themselves in their constructors and
 *  their constructors should execute in the context of the root
 *  thread.
 */
void dispatcher_t::_dispatch_packet(packet_t* packet) {

  stage_container_t* sc = _scdir[packet->_packet_type];
  if (sc == NULL)
    THROW2(DispatcherException, 
           "Packet type %s unregistered\n", packet->_packet_type.data());
  sc->enqueue(packet);
}



/**
 *  @brief Acquire the required number of worker threads.
 *
 *  THIS FUNCTION IS NOT THREAD-SAFE. It should not have to be since
 *  stages should register themselves in their constructors and their
 *  constructors should execute in the context of the root
 *  thread. Reserving workers does not modify the dispatcher's data
 *  structures.
 */
void dispatcher_t::_reserve_workers(const c_str& type, int n) {
  
  if (type == c_str("TSCAN")) {
    TRACE(TRACE_ALWAYS, "Reserving %d TSCAN\n", n);
  }
    
  stage_container_t* sc = _scdir[type];
  if (sc == NULL)
    THROW2(DispatcherException,
           "Type %s unregistered\n", type.data());
  sc->reserve(n);
}



/**
 *  @brief Release the specified number of worker threads.
 *
 *  THIS FUNCTION IS NOT THREAD-SAFE. It should not have to be since
 *  stages should register themselves in their constructors and their
 *  constructors should execute in the context of the root
 *  thread. Reserving workers does not modify the dispatcher's data
 *  structures.
 */
void dispatcher_t::_unreserve_workers(const c_str& type, int n) {
  
  stage_container_t* sc = _scdir[type];
  if (sc == NULL)
    THROW2(DispatcherException,
           "Type %s unregistered\n", type.data());
  sc->unreserve(n);
}



void dispatcher_t::register_stage_container(const c_str &packet_type,
                                            stage_container_t* sc) {
  instance()->_register_stage_container(packet_type, sc);
}



void dispatcher_t::dispatch_packet(packet_t* packet) {
  instance()->_dispatch_packet(packet);
}



dispatcher_t::worker_reserver_t* dispatcher_t::reserver_acquire() {
  return new worker_reserver_t(instance());
}



void dispatcher_t::reserver_release(worker_reserver_t* wr) {
  delete wr;
}



dispatcher_t::worker_releaser_t* dispatcher_t::releaser_acquire() {
  return new worker_releaser_t(instance());
}



void dispatcher_t::releaser_release(worker_releaser_t* wr) {
  delete wr;
}



EXIT_NAMESPACE(qpipe);

