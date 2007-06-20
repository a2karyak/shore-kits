

#include <cstdio>
#include <cstring>

#include <math.h>
#include <sys/pset.h>

#include "core/packet.h"
#include "vldb07_model/model.h"

#include <stdio.h>

using namespace qpipe;

namespace VLDB07Model {

#define MIN(a, b)          ((a < b) ? a : b)
#define MAX(a, b)          ((a > b) ? a : b)
#define MAX3(a, b, c)      ((a > b) ? MAX(a, c) : MAX(b, c))
#define MAX4(a, b, c, d)   ((a > b) ? MAX3(a, c, d) : MAX3(b, c, d))

static bool use_model = true;
static int  M;

/**
 *  @brief Shared/Unshared execution predictive model presented in VLDB07.
 */
bool VLDB07_model_t::is_shared_faster_scan(   const int   m
                                            , const float p
                                            , const float w
                                            , const float s
                                            , const float k
                                            , const float fs
                                            , const float fa
                                          )
{
  uint_t n;

  pset_info(PS_MYID,  NULL,  &n, NULL);

  float dshared = MAX(p, w+m*s);
  float ushared = w+m*(s+p);
  float shared = k*m*MIN(1/dshared, pow(n, fs)/ushared);

  float dalone = MAX(p, w+s);
  float ualone = m*(w+s+p);
  float alone = k*m*MIN(1/dalone, pow(n, fa)/ualone);

  float speedup = shared/alone;

  return (speedup > 1.0);
}



bool VLDB07_model_t::is_shared_faster_join(   const int   m
                                            , const float ps1
                                            , const float ps2
                                            , const float w
                                            , const float s
                                            , const float pa
                                            , const float k
                                            , const float fs
                                            , const float fa
                                          )
{
  uint_t n;

  pset_info(PS_MYID,  NULL,  &n, NULL);

  float dshared = MAX4(ps1, ps2, w+m*s, pa);
  float ushared = ps1+ps2+w+m*(s+pa);
  float shared = k*m*MIN(1/dshared, pow(n, fs)/ushared);

  float dalone = MAX4(ps1, ps2, w+s, pa);
  float ualone = m*(ps1+ps2+w+s+pa);
  float alone = k*m*MIN(1/dalone, pow(n, fa)/ualone);

  float speedup = shared/alone;

  return (speedup > 1.0);
}



void VLDB07_model_t::set_M(const int an_m) {
  M = an_m;
}

int VLDB07_model_t::get_M(void) {
  return M;
}



void VLDB07_model_t::enable_model(void) {
  use_model = true;
}

void VLDB07_model_t::disable_model(void) {
  use_model = false;
}

bool VLDB07_model_t::is_model_enabled(void) {
  return use_model;
}

void VLDB07_model_t::set_all_packet_flags(const bool val) {
  qpipe::set_osp_for_type(c_str("TSCAN"), val);
  qpipe::set_osp_for_type(c_str("AGGREGATE"), val);
  qpipe::set_osp_for_type(c_str("PARTIAL_AGGREGATE"), val);
  qpipe::set_osp_for_type(c_str("HASH_AGGREGATE"), val);
  qpipe::set_osp_for_type(c_str("HASH_JOIN"), val);
  qpipe::set_osp_for_type(c_str("PIPE_HASH_JOIN"), val);
  qpipe::set_osp_for_type(c_str("FUNC_CALL"), val);
  qpipe::set_osp_for_type(c_str("SORT"), val);
  qpipe::set_osp_for_type(c_str("SORTED_IN"), val);
  qpipe::set_osp_for_type(c_str("ECHO"), val);
  qpipe::set_osp_for_type(c_str("SIEVE"), val);
}


} // namespace VLDB07Model



