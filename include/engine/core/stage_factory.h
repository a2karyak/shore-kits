
#ifndef _STAGE_FACTORY_H
#define _STAGE_FACTORY_H



#include "engine/core/packet.h"
#include "engine/core/stage.h"



// include me last!!!
#include "engine/namespace.h"



/* exported datatypes */

class stage_container_t;


struct stage_factory_t {
  virtual stage_t* create_stage()=0;
  virtual ~stage_factory_t() { }

};



/**
 *  @brief If a stage provides DEFAULT_STAGE_NAME and does not need
 *  any additional constructor parameters, we actually write a lot of
 *  uncessary code creating its factory. We create this stage_factory
 *  (NOTE NO "_t"!) to provide a quick and dirty template
 *  workaround. For example, we could type:
 *
 *  dispatcher_t::register_stage(new stage_factory<sort_stage_t>);
 *
 *  The template specifies which stage to construct.
 */
template<class STAGE>
struct stage_factory : public stage_factory_t {

  virtual stage_t* create_stage() {
    return new STAGE();
  }

  virtual ~stage_factory() { }
};



#include "engine/namespace.h"



#endif
