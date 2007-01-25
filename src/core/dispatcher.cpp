
#include "util.h"
#include "core/dispatcher.h"

#include <cstdio>
#include <cstring>



ENTER_NAMESPACE(qpipe);


dispatcher_t* dispatcher_t::_instance = NULL;

pthread_mutex_t dispatcher_t::_instance_lock = thread_mutex_create();



/* internal helper functions */

int string_comparator (const void* key1, const void* key2);
size_t packet_type_hash(const void* key);



dispatcher_t::dispatcher_t() {
  static_hash_map_init( &stage_directory,
			&stage_directory_buckets[0],
			DISPATCHER_NUM_STATIC_HASH_MAP_BUCKETS,
			packet_type_hash,
			string_comparator);
}



dispatcher_t::~dispatcher_t() {
  TRACE(TRACE_ALWAYS, "Need to destroy nodes and keys\n");
}



/**
 *  @brief THIS FUNCTION IS NOT THREAD-SAFE. It should not have to be
 *  since stages should register themselves in their constructors and
 *  their constructors should execute in the context of the root
 *  thread.
 */
void dispatcher_t::_register_stage_container(const c_str &packet_type, stage_container_t* sc)
{

  
  // We eventually want multiple stages willing to perform SORT's. But
  // then we need policy/cost model to determine which SORT stage to
  // use when. For now, restrict to one stage per type.
  if ( !static_hash_map_find( &stage_directory, packet_type, NULL, NULL ) )
      throw EXCEPTION(DispatcherException,
                      "Trying to register duplicate stage for type %s\n",
                      packet_type.data());

  // allocate hash node and copy of key string
  static_hash_node_t node = (static_hash_node_t)malloc(sizeof(*node));
  if ( node == NULL )
      throw EXCEPTION(BadAlloc);

  char* ptcopy = new char[strlen(packet_type.data())+1];
  strcpy(ptcopy, packet_type.data());
  
  // add to hash map
  static_hash_map_insert( &stage_directory, ptcopy, sc, node );
}



/**
 *  @brief THIS FUNCTION IS NOT THREAD-SAFE. It should not have to be
 *  since stages should register themselves in their constructors and
 *  their constructors should execute in the context of the root
 *  thread.
 */
void dispatcher_t::_dispatch_packet(packet_t* packet) {

  
  void* sc;
  if ( static_hash_map_find( &stage_directory, packet->_packet_type.data(), &sc, NULL ) )
      throw EXCEPTION(DispatcherException, 
                      "Packet type %s unregistered\n",
                      packet->_packet_type.data());
 
  stage_container_t* stage_container = (stage_container_t*)sc;
  stage_container->enqueue(packet);
}



void dispatcher_t::register_stage_container(const c_str &packet_type, stage_container_t* sc) {
  instance()->_register_stage_container(packet_type, sc);
}



void dispatcher_t::dispatch_packet(packet_t* packet) {
  instance()->_dispatch_packet(packet);
}



/* definitions of internal helper functions */

int string_comparator(const void* key1, const void* key2) {
  const char* str1 = (const char*)key1;
  const char* str2 = (const char*)key2;
  return strcmp(str1, str2);
}

size_t packet_type_hash(const void* key) {
  const char* str = (const char*)key;
  return (size_t)RSHash(str, strlen(str));
}

EXIT_NAMESPACE(qpipe);

