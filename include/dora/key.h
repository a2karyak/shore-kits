/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   key.h
 *
 *  @brief:  Implementation of a template-based Key class
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */

#ifndef __DORA_KEY_H
#define __DORA_KEY_H


#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif

#include <iostream>
#include <sstream>
#include <vector>

#include "dora.h"

#include "sm/shore/shore_env.h"


ENTER_NAMESPACE(dora);

using std::vector;

const char KS_DELIMETER[] = "|";


template<typename DataType> struct key_wrapper_t;
template<typename DataType> std::ostream& operator<< (std::ostream& os,
                                                      const key_wrapper_t<DataType>& rhs);



/******************************************************************** 
 *
 * @struct: key_wrapper_t
 *
 * @brief:  Template-based class used for Keys
 *
 * @note:   - Wraps a vector of key entries. Needed for STL  
 *          - All the entries of the key of the same type
 * 
 *
 ********************************************************************/

template<typename DataType>
struct key_wrapper_t
{
    //typedef typename PooledVec<DataType>::Type        DataVec;
    typedef vector<DataType>        DataVec;
    typedef typename DataVec::iterator       DataVecIt;
    typedef typename DataVec::const_iterator DataVecCit;

    // the vector with the entries - of the same type
    DataVec _key_v;

    // empty constructor
    key_wrapper_t() { }

    // copying needs to be allowed (stl...)
    key_wrapper_t(const key_wrapper_t<DataType>& rhs)
    {
        // if already set do not reallocate        
        //        _key_v = new DataVec( rhs._key_v->get_allocator() );
        copy_vector(rhs._key_v);
    }
    
    // copy constructor
    key_wrapper_t<DataType>& operator=(const key_wrapper_t<DataType>& rhs) 
    {        
        //_key_v = new DataVec( rhs._key_v->get_allocator() );
        copy_vector(rhs._key_v);
        return (*this);
    }
    

    // destructor
    ~key_wrapper_t() { }

    // push one item
    inline void push_back(DataType& anitem) {
        _key_v.push_back(anitem);
    }

    // reserve vector space
    inline void reserve(const int keysz) {
        assert (keysz);
        _key_v.reserve(keysz);
    }


    // drops the key
    //inline void drop() { if (_key_v) delete (_key_v); }

    inline void copy(const key_wrapper_t<DataType>& rhs) 
    {
        copy_vector(rhs._key_v);
    }
    

    // helper functions
    inline void copy_vector(const DataVec& aVec) 
    {
        assert (_key_v.empty());
        _key_v.reserve(aVec.size());
        _key_v.assign(aVec.begin(),aVec.end()); // copy vector content
    }

    // comparison operators
    bool operator<(const key_wrapper_t<DataType>& rhs) const;
    bool operator==(const key_wrapper_t<DataType>& rhs) const;
    bool operator<=(const key_wrapper_t<DataType>& rhs) const;


    // CACHEABLE INTERFACE


    void setup(Pool** stl_pool_alloc_list) 
    {
        //assert (stl_pool_alloc_list);

        // it must have 1 pool lists: 
        // stl_pool_list[0]: DataType pool
//         assert (stl_pool_alloc_list[0]); 
//         _key_v = new DataVec( stl_pool_alloc_list[0] );
    }

    void reset() 
    {
        // clear contents
        _key_v.erase(_key_v.begin(),_key_v.end());
    }



    string toString() {
        std::ostringstream out = string("");
        for (DataVecIt it=_key_v.begin(); it!=_key_v.end(); ++it)
            out << out << (*it) << "|";
        return (out.str());
    }

    // friend function
    template<class DataType> friend std::ostream& operator<< (std::ostream& os, 
                                                              const key_wrapper_t<DataType>& rhs);



}; // EOF: struct key_wrapper_t



template<typename DataType> 
std::ostream& operator<< (std::ostream& os,
                          const key_wrapper_t<DataType>& rhs)
{
    typedef key_wrapper_t<DataType>::DataVecCit KeyDataIt;
    for (KeyDataIt it = rhs._key_v.begin(); it != rhs._key_v.end(); ++it) {
        os << (*it) << "|";
    }
    return (os);
}


//// COMPARISON OPERATORS ////


// less
template<class DataType>
inline bool key_wrapper_t<DataType>::operator<(const key_wrapper_t<DataType>& rhs) const 
{
    assert (_key_v.size()<=rhs._key_v.size()); // not necesserily of the same length
    for (int i = 0; i <_key_v.size(); ++i) {
        // goes over the key fields until one inequality is found
        if (_key_v.at(i)==rhs._key_v.at(i))
            continue;
        return (_key_v.at(i)<rhs._key_v.at(i));        
    }
    return (false); // irreflexivity - f(x,x) must be false
}

// equal
template<class DataType>
inline bool key_wrapper_t<DataType>::operator==(const key_wrapper_t<DataType>& rhs) const 
{
    assert (_key_v.size()<=rhs._key_v.size()); // not necesserily of the same length
    for (int i=0; i<_key_v.size(); i++) {
        // goes over the key fields until one inequality is found
        if (_key_v.at(i)==rhs._key_v.at(i))
            continue;
        return (false);        
    }
    return (true);
}

// less or equal
template<class DataType>
inline bool key_wrapper_t<DataType>::operator<=(const key_wrapper_t<DataType>& rhs) const 
{
    assert (_key_v.size()<=rhs._key_v.size()); // not necesserily of the same length
    for (int i=0; i<_key_v.size(); i++) {
        // goes over the key fields
        if (_key_v.at(i)==rhs._key_v.at(i))
            continue;
        return (_key_v.at(i)<rhs._key_v.at(i));        
    }
    // if reached this point all fields are equal so the two keys are equal
    return (true); 
}



EXIT_NAMESPACE(dora);

#endif /* __DORA_KEY_H */
