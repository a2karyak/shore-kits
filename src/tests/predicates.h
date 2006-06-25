// -*- mode:c++; c-basic-offset:4 -*-
#ifndef __PREDICATES_H
#define __PREDICATES_H

#include <vector>
#include <algorithm>
#include <functional>
#include <string>

using std::string;

using std::less;
using std::greater;
using std::less_equal;
using std::greater_equal;
using std::equal_to;
using std::not_equal_to;

/**
 * @brief Composable predicate base class
 */

struct predicate_t {
    virtual bool select(const tuple_t &tuple)=0;

    virtual predicate_t* clone()=0;
    
    virtual ~predicate_t() { }
};



/**
 * @brief scalar predicate. Given a field type and offset in the
 * tuple, it extracts the field and tests it against the given
 * value. select returns the test result.
 *
 * V must have proper relational operators defined
 */
  
template <typename V, template<class> class T=equal_to>
class scalar_predicate_t : public predicate_t {
    V _value;
    size_t _offset;
public:
    scalar_predicate_t(V value, size_t offset)
        : _value(value), _offset(offset)
    {
    }
    virtual bool select(const tuple_t &tuple) {
        V* field = reinterpret_cast<V*>(tuple.data + _offset);
        return T<V>()(*field, _value);
    }
    virtual scalar_predicate_t* clone() {
        return new scalar_predicate_t(*this);
    }
};


/**
 * @brief string predicate. Given a field type and offset in the
 * tuple, it extracts the field and tests it against the given
 * value. select returns the test result.
 *
 * V must have proper relational operators defined
 */
  
template <template<class> class T=equal_to>
class string_predicate_t : public predicate_t {
    string _value;
    size_t _offset;
public:
    string_predicate_t(const string &value, size_t offset)
        : _value(value), _offset(offset)
    {
    }
    virtual bool select(const tuple_t &tuple) {
        const char* field = tuple.data + _offset;
        return T<int>()(strcmp(field, _value.c_str()), 0);
    }
    virtual string_predicate_t* clone() {
        return new string_predicate_t(*this);
    }
};

template <typename V, template<class> class T=equal_to>
class field_predicate_t : public predicate_t {
    size_t _offset1;
    size_t _offset2;
public:
    field_predicate_t(size_t offset1, size_t offset2)
        : _offset1(offset1), _offset2(offset2)
    {
    }
    virtual bool select(const tuple_t &tuple) {
        V* field1 = reinterpret_cast<V*>(tuple.data + _offset1);
        V* field2 = reinterpret_cast<V*>(tuple.data + _offset2);
        return T<V>()(*field1, *field2);
    }
    virtual field_predicate_t* clone() {
        return new field_predicate_t(*this);
    }
};

/**
 * @brief Conjunctive predicate. Selects a tuple only if all of its
 * internal predicates pass.
 */
template<bool DISJUNCTION>
class compound_predicate_t : public predicate_t {
public:
    typedef std::vector<predicate_t*> predicate_list_t;
private:
    predicate_list_t _list;
    
    // if DISJUNCTION, returns true when the argument selects its
    // tuple; else returns true when the argument does not select its
    // tuple
    struct test_t {
        const tuple_t &_t;
        test_t(const tuple_t &t) : _t(t) { }
        bool operator()(predicate_t* p) {
            return p->select(_t) == DISJUNCTION;
        }
    };

    // clones its argument
    struct clone_t {
        predicate_t* operator()(predicate_t* p) {
            return p->clone();
        }
    };

    // deletes its argument
    struct delete_t {
        void operator()(predicate_t* p) {
            delete p;
        }
    };
    void clone_list() {
        // clone the predicates in the list
        std::transform(_list.begin(), _list.end(), _list.begin(), clone_t());
    }
public:
    void add(predicate_t* p) {
        _list.push_back(p);
    }
    virtual bool select(const tuple_t  &t) {
        // "search" for the answer
        predicate_list_t::iterator result;
        result = std::find_if(_list.begin(), _list.end(), test_t(t));
        
        // if disjunction, success means we didn't reach the end of
        // the list; else success means we did
        return DISJUNCTION? result != _list.end() : result == _list.end();
    }
    virtual compound_predicate_t* clone() {
        return new compound_predicate_t(*this);
    }
    compound_predicate_t(const compound_predicate_t &other)
        : predicate_t(other), _list(other._list)
    {
        clone_list();
    }
    compound_predicate_t() { }
    compound_predicate_t &operator=(const compound_predicate_t &other) {
        _list = other._list;
        clone_list();
    }
    virtual ~compound_predicate_t() {
        std::for_each(_list.begin(), _list.end(), delete_t());
    }
};

typedef compound_predicate_t<false> and_predicate_t;
typedef compound_predicate_t<true> or_predicate_t;

#endif
