/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef _HASHTABLE_H
#define _HASHTABLE_H



/**
 * @brief A generic RAII guard class.
 *
 * This class ensures that the object it encloses will be properly
 * disposed of when it goes out of scope, or with an explicit call to
 * done(), whichever comes first.
 *
 * This class is much like the auto_ptr class, other than allowing
 * actions besides delete upon destruct. In particular it is *NOT
 * SAFE* to use it in STL containers because it does not fulfill the
 * Assignable concept.
 *
 * TODO: make generic and configurable (ie, use templates to determine
 * what "null" and "action" are)
 *
 */
template <class Data, class Extract, class Hash, class Equal>
class hashtable {

 private:

  struct entry {
    bool e_exists;
    Data e_data;
  };
  
  size_t _capacity;
  size_t _size;
  struct entry* _entries;
  

  /**
   * @brief Find slot to insert 'd'. This implementation uses linear
   * probing.
   */
  size_t find_empty_slot(Data d) {
    
    /* Check for space. */
    assert(_size < _capacity);

    /* Compute position to insert data */
    /* TODO Apply hasher here */
    size_t hash_code = 0xdeadbeef;
    size_t hash_pos  = hash_code % _capacity;
    
    /* Loop 1: Probe until we find empty slot or hit the end of the
       array. */
    for ( ; _entries[hash_pos].exists && (hash_pos < _capacity); hash_pos++);

    if (hash_pos == _capacity) {
      /* Reached end of table with no slot. */
      /* Loop 2: Continue probing from beginning of array. */
      for ( hash_pos = 0; _entries[hash_pos].exists && (hash_pos < _capacity); hash_pos++);
    }

    /* At this point, we had better have stopped because we found an
       empty slot. We already verified at the start of the function
       that there is space available. */
    assert(! _entries[hash_pos].exists );
    return hash_pos;
  }


 public:
    
  hashtable(size_t capacity)
    :
    _capacity(capacity),
    _size(0),
    _entries(new struct entry[capacity])
    {
        /* TODO check whether this is the correct way to "initialize" the
           memory region */
        memset(_entries, 0, sizeof(_entries));
    }


  

};



#endif
