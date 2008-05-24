/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __DRIVER_H
#define __DRIVER_H

#include "util.h"



ENTER_NAMESPACE(workload);


/**
 *  @class driver_t
 *
 *  @brief A driver is basically a wrapper around a submit() method
 *  that clients can invoke to submit requests into the system. This
 *  lets us isolate all workload specific properties in a single
 *  class. One important point is that driver_t instances will be
 *  shared by concurrent workload client threads. Because of this, a
 *  driver SHOULD NOT store any object state. The submit() function
 *  should allocate all required data structures within the context of
 *  the calling thread.
 */

class driver_t {

private:
    
    c_str _description;
    
public:

    driver_t(const c_str& description)
        : _description(description)
    {
    }

    virtual ~driver_t() { }

    virtual void submit(void* arg)=0;

    c_str description() { return _description; }
};



#define DECLARE_DRIVER(dname) \
   class dname##_driver : public driver_t { \
   \
   public: \
   \
     dname ## _driver(const c_str& description) \
         : driver_t(description) \
     { \
     } \
   \
     virtual void submit(void* arg); \
   \
  };




EXIT_NAMESPACE(workload);


#endif // _DRIVER_H
