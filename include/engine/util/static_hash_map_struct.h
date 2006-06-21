
/** @file static_hash_map_struct.h
 *
 *  @brief Exports the internal representation of a
 *  static_hash_map_t datatype. This file should be included in a
 *  module for the sole purpose of statically allocating tables
 *  (i.e. as globals).
 *
 *  Modules should only include this file IF AND ONLY IF they need to
 *  statically and/or initialize static_hash_maps_t's. Once that is
 *  done, this module should continue to use the functions provided in
 *  static_hash_map.h to manipulate the data structure.
 *
 *  @author Naju Mancheril
 *
 *  @bug None known.
 */

#ifndef _STATIC_HASH_MAP_STRUCT_H
#define _STATIC_HASH_MAP_STRUCT_H

#include <sys/types.h>




/* exported structures */


/** @struct static_hash_map_s
 *
 *  @brief This structure represents our hash table. We expose it here
 *  so that tables may be statically allocated. Once a structure is
 *  declared, cast its address to a static_hash_map_t. Use the
 *  static_hash_map_t "datatype" for all manipulations.
 */

struct static_hash_map_s {
    
  struct static_hash_node_s* table_entries;
  size_t table_size;
  
  size_t (*hf) (const void* key);
  int    (*comparator) (const void* key1, const void* key2);
};




/** @struct static_hash_node_s
 *
 *  @brief The static_hash_map_t uses separate chaining to deal with
 *  collisions. This structure represents a node in a bucket chain. We
 *  expose it here so that nodes may be statically allocated. Once a
 *  structure is declared, cast its address to a
 *  static_hash_node_t. Use the static_hash_node_t "datatype" for all
 *  manipulations.
 */

struct static_hash_node_s {
    
  /** The previous node in the list. */
  struct static_hash_node_s* prev;

  /** The next node in the list. */
  struct static_hash_node_s* next;

  /** This entry's key. */
  const void* key;

  /** This entry's data. */
  const void* value;
};



/* It would be nice to provide static initializers here, but every
   hash node in a hash table must be initialized. Since there are a
   variable number of these, I'm not sure how it can be done. */



#endif
