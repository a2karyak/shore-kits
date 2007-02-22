
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
void dispatcher_t::_register_stage_container(const c_str &packet_type,
                                             stage_container_t* sc)
{
  // We eventually want multiple stages willing to perform SORT's. But
  // then we need policy/cost model to determine which SORT stage to
  // use when. For now, restrict to one stage per type.
  if ( !static_hash_map_find( &stage_directory, packet_type, NULL, NULL ) )
      THROW2(DispatcherException,
                      "Trying to register duplicate stage for type %s\n",
                      packet_type.data());

  // allocate hash node and copy of key string
  static_hash_node_t node = (static_hash_node_t)malloc(sizeof(*node));
  if ( node == NULL )
      THROW(BadAlloc);

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
      THROW2(DispatcherException, 
                      "Packet type %s unregistered\n",
                      packet->_packet_type.data());
 
  stage_container_t* stage_container = (stage_container_t*)sc;
  stage_container->enqueue(packet);
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
  
  void* sc;
  if ( static_hash_map_find( &stage_directory, type.data(), &sc, NULL ) )
    THROW2(DispatcherException, 
           "Type %s unregistered\n",
           type.data());
  
  stage_container_t* stage_container = (stage_container_t*)sc;
  stage_container->reserve(n);
}



void dispatcher_t::register_stage_container(const c_str &packet_type, stage_container_t* sc) {
  instance()->_register_stage_container(packet_type, sc);
}



void dispatcher_t::dispatch_packet(packet_t* packet) {
  instance()->_dispatch_packet(packet);
}



void dispatcher_t::reserve_workers(const c_str& type, int n) {
  instance()->_reserve_workers(type, n);
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



class worker_reserver_t : public resource_reserver_t
{

private:

  map<c_str, int> worker_needs;

  void reserve(const c_str& type) {
    int n = worker_needs[type];
    if (n > 0)
      dispatcher_t::reserve_workers(type, n);
  }

public:

  virtual void declare_resource_need(const c_str& name, int count) {
    int curr_needs = worker_needs[name];
    worker_needs[name] = curr_needs + count;
  }

  virtual void acquire_resources() {
    
    static const char* order[] = {
      "AGGREGATE"
      ,"BNL_IN"
      ,"BNL_JOIN"
      ,"FDUMP"
      ,"FSCAN"
      ,"FUNC_CALL"
      ,"HASH_JOIN"
      ,"MERGE"
      ,"PARTIAL_AGGREGATE"
      ,"SORT"
      ,"SORTED_IN"
      ,"TSCAN"
    };
    
    int num_types = sizeof(order)/sizeof(order[0]);
    for (int i = 0; i < num_types; i++)
      reserve(order[i]);
  }

};



void reserve_query_workers(packet_t* root)
{
  worker_reserver_t wr ;
  root->declare_worker_needs(&wr);
  wr.acquire_resources();
}



EXIT_NAMESPACE(qpipe);

