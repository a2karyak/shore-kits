/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <cstdlib>
#include "util/guard.h"

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
template <class Data, class Key, class HashFcn, class ExtractKey, class EqualKey>
class hashtable {
    
private:

    typedef struct {
        int  prev;
        int  next;
    } link_t;

    /* the table */
    int          _capacity;
    int          _size;
    array_guard_t<Data>        _data;
    array_guard_t<link_t>      _links;
    bool*        _exists;
    
    /* key-value functors */
    ExtractKey _extract;
    EqualKey   _equal;
    HashFcn    _hash;

    /* free list */
    int  _free_head;

    
    void remove_from_free_list(int index) {

        /* splice ends of list together */
        link_t* link = &_links[index];
        int prev_index = link->prev;
        int next_index = link->next;

        /* fix _free_head */
        if (_free_head == index) {
            if (index == prev_index)
                _free_head = -1;
            else
                _free_head = next_index;
        }

        link_t* prev = &_links[prev_index];
        link_t* next = &_links[next_index];
        prev->next = next_index;
        next->prev = prev_index;

        /* make floating node */
        link->next = link->prev = index;
    }
    
    
    int remove_free_list_head(void) {
        if (_free_head == -1)
            return -1;
        int head = _free_head;
        remove_from_free_list(head);
        return head;
    }


    void splice_before(int index, int insert_before_this) {

        link_t* next   = &_links[insert_before_this];
        int prev_index = next->prev;
        link_t* prev   = &_links[prev_index];
        link_t* link   = &_links[index];
        
        prev->next = index;
        link->prev = prev_index;
        link->next = insert_before_this;
        next->prev = index;
    }

    
    void splice_after(int index, int insert_after_this) {
        
        link_t* prev   = &_links[insert_after_this];
        int prevs_next = prev->next;
        splice_before(index, prevs_next);
    }

    
    bool bucket_contains(Data& d, int index) {
        
        if ( !_exists[index] )
            return false;
        
        if (_equal(_extract(_data[index]), _extract(d)))
            return true;
        
        int curr;
        for (curr = _links[index].next; curr != index; curr = _links[curr].next) {
            if (_equal(_extract(_data[curr]), _extract(d)))
                return true;
        }

        return false;
    }

    void insert_into_bucket(Data& d, int hash_pos) {
        
        /* check for collision */
        int insert_pos;
        if ( !_exists[hash_pos] ) {
            /* remove from free list */
            remove_from_free_list(hash_pos);
            /* insert here */
            insert_pos = hash_pos;
        }
        else {
            /* remove a node from the free list */
            insert_pos = remove_free_list_head();
            assert(insert_pos != -1);
            assert(!_exists[insert_pos]);
            /* connect node to existsing chain */
            splice_before(insert_pos, hash_pos);
        }
                
        _data[insert_pos]   = d;
        _exists[insert_pos] = true;
    }
    
public:
    
    
    hashtable(int capacity, HashFcn hash, EqualKey equal, ExtractKey extract)
        : _capacity(capacity)
        , _size(0)
        , _data(new Data[capacity])
        , _links(new link_t[capacity])
        , _exists(new bool[capacity])
        , _extract(extract)
        , _equal(equal)
        , _hash(hash)
    {
        
        /* create free list */
	_links[0].prev = capacity-1;
	_links[capacity-1].next = 0;
        for (int i = 1; i < capacity; i++) {
	    _links[i-1].next = i;
	    _links[i].prev = i-1;
        }
        
        _free_head = 0;

	// operator new() calls Data() for each element in the data array
    }

    
    /**
     * @brief 
     */
    void insert_equal_noresize(Data d) {
        
        /* Check for space. */
        assert(_size < _capacity);
        size_t hash_code = _hash(_extract(d));
        int hash_pos = (int)(hash_code % (size_t)_capacity);
     
        insert_into_bucket(d, hash_pos);
    }
        

    /**
     * @brief 
     */
    bool insert_unique_noresize(Data d) {
        
        size_t hash_code = _hash(_extract(d));
        int hash_pos = (int)(hash_code % (size_t)_capacity);
     
        if (bucket_contains(d, hash_pos))
            /* data already present */
            return false;

        /* Check for space. */
        assert(_size < _capacity);
        
        insert_into_bucket(d, hash_pos);
        return true;
    }


    /**
     * @brief Check for the specified data.
     */
    bool contains(Data d) {
        size_t hash_code = _hash(_extract(d));
        int hash_pos = (int)(hash_code % (size_t)_capacity);
        return bucket_contains(d, hash_pos);
    }

    void clear() {
	// TODO: I'm not sure why this was being called, but it's not
	// terribly hard to implement...
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




#if 0

/* Old linear probing code */

    /**
     * @brief Find slot to insert 'd'. This implementation uses linear
     * probing.
     */
    int find_slot(Data d) {
    
        /* Check for space. */
        assert(_size < _capacity);

        /* Compute position to insert data */
        /* TODO Apply hasher here */
        int hash_code = _hashfcn(d);
        int hash_pos  = hash_code % _capacity;
    
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

#endif


#endif
