# -*- makefile -*-


INCLUDES = -I$(top_srcdir)/include \
           -I$(top_srcdir)/include/workload

WL_BASE = \
   workload.cpp \
   workload_client.cpp \
   register_stage_containers.cpp \
   dispatcher_globals.cpp

WL_COMMON = \
   common/q6_packet.cpp \
   common/q6_client.cpp 

WL_TPCH = \
   tpch/tpch_db.cpp \
   tpch/tpch_db_load.cpp \
   tpch/tpch_compare.cpp \
   tpch/tpch_env.cpp \
   tpch/tpch_type_convert.cpp \
   tpch/drivers/tpch_q1.cpp  \
   tpch/drivers/tpch_q4.cpp  \
   tpch/drivers/tpch_q6.cpp  \
   tpch/drivers/tpch_q12.cpp \
   tpch/drivers/tpch_q13.cpp \
   tpch/drivers/tpch_q16.cpp \
   tpch/drivers/tpch_m_1_4_6_12.cpp \
   tpch/drivers/tpch_upto13.cpp \
   tpch/drivers/tpch_m146.cpp \
   tpch/drivers/tpch_m14612.cpp \
   tpch/drivers/tpch_sim_mix.cpp


# encapsulates all basic classes needed in qpipe
lib_LIBRARIES = libworkload.a

libworkload_a_SOURCES = \
   $(WL_BASE) \
   $(WL_COMMON) \
   $(WL_TPCH)
