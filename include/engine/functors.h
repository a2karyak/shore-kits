/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __FUNCTORS_H
#define __FUNCTORS_H

#include "engine/core/tuple.h"



#include "engine/namespace.h"

// use this to force proper alignment of key arrays
#define ALIGNED __attribute__ ((aligned))


/** 
 * @brief Base selection/projection functor. Should be extended to
 * implement other selection/projection functors.
 */

class tuple_filter_t {
private:
    size_t _tuple_size;
    
public:

    size_t input_tuple_size() { return _tuple_size; }
    
    /**
     *  @brief Determine which attributes (if any) of a given source
     *  tuple pass an internal set of predicates. Should be applied to
     *  the output of a stage to implement either selection or
     *  projection.
     *
     *  This default implementation allows every attribute of every
     *  source tuple to pass.
     *
     *  @param src The tuple we are checking.
     *
     *  @return True if the specified tuple passes the
     *  selection. False otherwise.
     */

    virtual bool select(const tuple_t &) {
        return true;
    }


    /**
     *  @brief Project some threads from the src to the dest tuple.
     *
     *  This default implementation copies the entire tuple, so dest
     *  must point to a location with at least src.size bytes
     *  available.
     *
     *  @param src Our source tuple.
     *
     *  @param dest We copy the necessary attributes here. This tuple
     *  should already point into a tuple_buffer_t where we can store
     *  data.
     *
     *  @return void
     */

    virtual void project(tuple_t &dest, const tuple_t &src) {
        dest.assign(src);
    }

    // should simply return new <child-class>(*this);
    virtual tuple_filter_t* clone()=0;
    
    tuple_filter_t(size_t input_tuple_size)
        : _tuple_size(input_tuple_size)
    {
    }
    
    virtual ~tuple_filter_t() { }

};

struct trivial_filter_t : public tuple_filter_t {
    
    virtual tuple_filter_t* clone() {
        return new trivial_filter_t(*this);
    };
    
    trivial_filter_t(size_t input_tuple_size)
        : tuple_filter_t(input_tuple_size)
    {
    }
    
};

/**
 * @brief A key-tuple pair used for operations such as hashing and
 * sorting. The first 32 bits of the key (or hash) are stored directly
 * and should suffice for most comparisons; the full key can always be
 * accessed by following the pointer to the tuple's data.
 */

struct hint_tuple_pair_t {
    int hint;
    char *data;

    hint_tuple_pair_t()
        : hint(0), data(NULL)
    {
    }
    hint_tuple_pair_t(int h, char *d)
        : hint(h), data(d)
    {
    }
};



/**
 *  @brief Base comparison functor. Extend it to implement specific
 *  comparisons on tuple field(s).
 */

struct key_compare_t {
    
    /**
     * @brief Determines the lexicographical relationship between key1
     * and key2. Ascending sorts based on this comparison will produce
     * outputs ordered such that compare(key[i], key[i+1]) < 0 for
     * every i.
     *
     * @return negative if a < b, positive if a > b, and zero if a == b
     */
    
    virtual int operator()(const void* key1, const void* key2)=0;
    virtual key_compare_t* clone()=0;

    virtual ~key_compare_t() { }
};

class key_extractor_t {
    size_t _key_size;
    
public:
    key_extractor_t(size_t key_size=sizeof(int))
        : _key_size(key_size)
    {
    }
    size_t key_size() { return _key_size; }

    /**
     * @brief extracts the full key of a tuple.
     */
    void extract_key(void* key, const tuple_t &tuple) {
        extract_key(key, tuple.data);
    }

    virtual void extract_key(void* key, const void* tuple_data)=0;

    /**
     * @brief extracts an abbreviated key that represents the most
     * significant 4 bytes of the full key. This allows quicker
     * comparisons but equal shortcut keys require a full key compare
     * to resolve the ambiguity.
     */
    int extract_hint(const tuple_t &tuple) {
        return extract_hint(tuple.data);
    }
    
    virtual int extract_hint(const void* tuple_data) {
        // this guarantees that we're not doing something dangerous
        assert(key_size() <= sizeof(int));
        
        // make sure the key is always big enough to hold an int
        char key[key_size() + sizeof(int)] ALIGNED;

        // clear out any padding that might result from smaller keys
        *(int*) key = 0;
        extract_key(key, tuple_data);
        return *(int*) key;
    }

    // should simply return new <child-class>(*this);
    virtual key_extractor_t* clone()=0;
    virtual ~key_extractor_t() { }
};

struct tuple_comparator_t {
    key_extractor_t* _extract;
    key_compare_t* _compare;
    tuple_comparator_t(key_extractor_t* extract, key_compare_t* compare)
        : _extract(extract), _compare(compare)
    {
    }
    
    int operator()(const hint_tuple_pair_t &a, const hint_tuple_pair_t &b) {
        int diff = a.hint - b.hint;
        if(_extract->key_size() <= sizeof(int) || diff)
            return diff;

        char akey[_extract->key_size()];
        char bkey[_extract->key_size()];
        _extract->extract_key(akey, a.data);
        _extract->extract_key(bkey, b.data);
        return (*_compare)(akey, bkey);
    }
};

/** 
 * @brief Base join functor. Should be used by join stages to
 * determine if two source tuples pass the join predicate. Should be
 * extended to implement other join functors.
 */

class tuple_join_t {

    size_t _left_tuple_size;
    pointer_guard_t<key_extractor_t> _left;
    
    size_t _right_tuple_size;
    pointer_guard_t<key_extractor_t> _right;

    pointer_guard_t<key_compare_t> _compare;
    
    size_t _out_tuple_size;
public:

    size_t left_tuple_size() { return _left_tuple_size; }
    size_t right_tuple_size() { return _right_tuple_size; }
    size_t out_tuple_size() { return _out_tuple_size; }
    size_t key_size() { return _left->key_size(); }

    tuple_join_t(size_t left_tuple_size, key_extractor_t *left,
                 size_t right_tuple_size, key_extractor_t *right,
                 key_compare_t* compare, size_t out_tuple_size)
        : _left_tuple_size(left_tuple_size), _left(left),
          _right_tuple_size(right_tuple_size), _right(right),
          _compare(compare), _out_tuple_size(out_tuple_size)
    {
        assert(left->key_size() == right->key_size());
    }
    void get_left_key(void* key, const tuple_t &tuple) {
        return _left->extract_key(key, tuple);
    }
    void get_right_key(void* key, const tuple_t &tuple) {
        return _right->extract_key(key, tuple);
    }

    int compare(const void* left_key, const void* right_key) {
        return (*_compare)(left_key, right_key);
    }
    
    /**
     *  @brief Determine whether two tuples pass an internal set of join
     *  predicates. Should be applied within a join stage.
     *
     *  @param dest If the two tuples pass the specified join
     *  predicates, a composite of the two is assigned here.
     *
     *  @param A tuple from the outer relation.
     *
     *  @param A tuple from the inner relation.
     *
     *  @return True if the specified tuples pass the join
     *  predicates. False otherwise.
     */

    virtual void join(tuple_t &dest,
                      const tuple_t &left,
                      const tuple_t &right)=0;

    /**
     * @brief Performs a left outer join (right outer joins are not
     * directly supported but can be achieved by reversing the meaning
     * of "left" and "right")
     */
    virtual void outer_join(tuple_t &, const tuple_t &) {
        assert(false);
    }

    // TODO: implement a join function that tests the predicates first
    virtual ~tuple_join_t() { }
};



/** 
 * @brief Base aggregation functor. Should be extended to implement
 * other aggregation functors.
 */
class tuple_aggregate_t {

public:

    
    /**
     *  @brief Update the internal aggregate state and determine if a
     *  new output tuple can be produced (if a group break occurred).
     *
     *  @param dest If an output tuple is produced, it is assigned to
     *  this tuple.
     *
     *  @return True if an output tuple is produced. False otherwise.
     */

    virtual bool aggregate(tuple_t &dest, const tuple_t &src)=0;


    /**
     *  @brief Do a final update of the internal aggregate state and
     *  (possibly) produce a final output tuple. This method should be
     *  invoked when there are no more source tuples to be aggregated.
     *
     *  @param dest If an output tuple is produced, it is assigned to
     *  this tuple.
     *
     *  @return True if an output tuple is produced. False otherwise.
     */

    virtual bool eof(tuple_t &dest)=0;
    
  
    virtual ~tuple_aggregate_t() { }

};



#include "engine/namespace.h"



#endif	// __FUNCTORS_H
