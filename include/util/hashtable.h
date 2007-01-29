/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <cstdlib>
#include <utility>
#include "util/guard.h"

using std::pair;



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
template <class Data, class Key, class HashFcn, class ExtractKey, class EqualKey, class EqualData>
class hashtable {
    
private:

    /* the table */
    int _capacity;
    int _size;
    array_guard_t<Data>  _data;
    array_guard_t<bool>  _exists;
    
    /* key-value functors */
    HashFcn    _hash;
    ExtractKey _extract;
    EqualKey   _equalkey;
    EqualData  _equaldata;

    /* free list */
    int  _free_head;


public:
    
   
    hashtable(int capacity, HashFcn hash, ExtractKey extract,
              EqualKey equalkey, EqualData equaldata)
        : _capacity(capacity)
        , _size(0)
        , _data(new Data[capacity])
        , _exists(new bool[capacity])
        , _hash(hash)
        , _extract(extract)
        , _equalkey(equalkey)
        , _equaldata(equaldata)
    {
        /* initialize _exists */
        for (int i = 0; i < capacity; i++) {
	    _exists[i] = false;
        }
    }

    
    /**
     * @brief 
     */
    void insert_noresize(Data d) {
        
        /* Check for space. */
        assert(_size < _capacity);
        size_t hash_code = _hash(_extract(d));
        int hash_pos = (int)(hash_code % (size_t)_capacity);

        /* Linear probing */
        int pos;
        /* Loop 1: Probe until we find empty slot or hit the end of
           the array. */
        for (pos = hash_pos;
             (pos < _capacity) && _exists[pos]; pos++);
        if (pos == _capacity) {
            /* Reached end of table with no slot. */
            /* Loop 2: Continue probing from beginning of array. */
            for (pos = 0;
                 (pos < _capacity) && _exists[pos]; pos++);
        }
        /* At this point, we had better have stopped because we found
           an empty slot. We already verified at the start of the
           function that there is space available. */
        assert(! _exists[pos] );

        _data[pos] = d;
        _exists[pos] = true;
    }
        

    /**
     * @brief 
     */
    bool insert_unique_noresize(Data d) {
        
        /* Don't check for free space! */
        size_t hash_code = _hash(_extract(d));
        int hash_pos = (int)(hash_code % (size_t)_capacity);

        int pos;
        /* Linear probing */
        /* Loop 1: Probe until we find empty slot, find a copy, or hit
           the end of the array. */
        for (pos = hash_pos;
             (pos < _capacity)
                 && !equaldata(_data[pos],d)
                 && _exists[pos]; pos++);
        if (pos == _capacity) {
            /* Reached end of table with no slot. */
            /* Loop 2: Continue probing from beginning of array. */
            for (pos = hash_pos;
                 (pos < _capacity)
                     && !equaldata(_data[pos],d)
                     && _exists[pos]; pos++);
        }

        /* At this point, we may have reached the end of the array
           again (since we did not check for free space at the
           beginning). We have dealt with this kind of this with an
           assert() before... */
        assert(pos != _capacity);
        
        if (equaldata(_data[pos],d))
            /* Found a copy! Don't insert! */
            return false;
        
        /* If we are here, we have found an empty slot. */
        assert(! _exists[pos] );
        
        _data[pos] = d;
        _exists[pos] = true;
        return true;
    }


    /**
     * @brief Check for the specified data.
     */
    bool contains(Data d) {

        /* Don't check for free space! */
        size_t hash_code = _hash(_extract(d));
        int hash_pos = (int)(hash_code % (size_t)_capacity);

        int pos;
        /* Linear probing */
        /* Loop 1: Probe until we find empty slot, find a copy, or hit
           the end of the array. */
        for (pos = hash_pos;
             (pos < _capacity)
                 && !equaldata(_data[pos],d)
                 && _exists[pos]; pos++);
        if (pos == _capacity) {
            /* Reached end of table with no slot. */
            /* Loop 2: Continue probing from beginning of array. */
            for (pos = hash_pos;
                 (pos < _capacity)
                     && !equaldata(_data[pos],d)
                     && _exists[pos]; pos++);
        }

        if (pos == _capacity) {
            /* table is full and still does not contain 'd' */
            return false;
        }
        
        if (equaldata(_data[pos],d))
            /* Found a copy! */
            return true;
        
        /* If we are here, we have found an empty slot. */
        assert(! _exists[pos] );
        
        return false;
    }


    void clear() {
        for (int i = 0; i < _capacity; i++)
            _exists[i] = false;
        _size = 0;
    }

#if 1
    /**
     *  @brief Iterator over the tuples in this page. Each dereference
     *  returns a tuple_t.
     */

    class iterator {

    private:
        
        /* iterator position management */
        int  _head_index;
        int  _curr_index;
        bool _returned_head;
        int  _num_returned;

        /* the enclosing hash table */
        hashtable* _parent;
        
        /* the data we use for equal_range() */
        Key       _key;

        
    public:

        iterator(int head_index, hashtable* parent,
                 Key const& key)
            : _head_index(head_index)
            , _curr_index(head_index)
            , _returned_head(false)
            , _num_returned(0)
            , _parent(parent)
            , _key(key)
        {
        }
	iterator()
	    : _curr_index(-1), _parent(NULL)
	{
	}
        bool operator ==(const iterator &other) const {
	    // technically there's more, but this is enough to handle
	    // its intended use in a for loop.
            return _parent == other._parent
                && _curr_index == other._curr_index;
        }

        bool operator !=(const iterator &other) const {
            return !(*this == other);
        }

	Data& operator*() {
	    return *get();
	}
        Data* operator ->() {
	    return get();
	}
	Data* get() {
            if ((_curr_index == _head_index) && _returned_head)
                /* back at the beginning! */
                return NULL;

            return &_parent->_data[_curr_index];
        }

        iterator &operator ++() {

            int curr_index = _curr_index;
            int next_index = _parent->_links[curr_index].next;

	    Data* d = get();
	    if(!d) {
                /* no more elements back at the beginning! */
		_curr_index = -1;
		_parent = NULL;
                return *this;
	    }

	    // if this isn't a match, try again. Naju, an iterative
	    // version would be way better, but I was less likely to
	    // mess up this way...
	    if(!_parent->_equal(_key, _parent->_extract(*d)))
		return ++*this;
	    
            _returned_head = true;
            _curr_index = next_index;
            _num_returned++;
            return *this;
        }

        iterator operator ++(int) {
            iterator old = *this;
            ++*this;
            return old;
        }
    };
    
            
    std::pair<iterator, iterator> equal_range(Key const & k) {
        size_t hash_code = _hash(k);
        int hash_pos = (int)(hash_code % (size_t)_capacity);
        return std::make_pair(iterator(hash_pos, this, k), iterator());
    }

#endif


};


#endif
