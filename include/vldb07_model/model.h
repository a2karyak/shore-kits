/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _VLDB07_MODEL_H
#define _VLDB07_MODEL_H


namespace VLDB07Model {


/**
 *  @brief Shared/Unshared execution predictive model presented in VLDB07.
 */

struct VLDB07_model_t {
  static bool is_shared_faster_scan(   const int   m
                                     , const float p
                                     , const float w
                                     , const float s
                                     , const float k
                                     , const float fs
                                     , const float fa
                                   );

  static bool is_shared_faster_join(   const int   m
                                     , const float ps1
                                     , const float ps2
                                     , const float w
                                     , const float s
                                     , const float pa
                                     , const float k
                                     , const float fs
                                     , const float fa
                                   );

  static void set_M(const int an_m);
  static int  get_M(void);

  static void enable_model(void);
  static void disable_model(void);
  static bool is_model_enabled(void);

  static void set_all_packet_flags(const bool val);

};

} // namespace VLDB07Model


#endif // _VLDB07_MODEL_H
