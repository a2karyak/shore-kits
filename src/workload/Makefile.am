# -*- makefile -*-


INCLUDES = -I$(top_srcdir)/include \
           -I$(top_srcdir)/include/workload

WL_BASE = \
   workload.cpp \
   client.cpp \
   register_stage_containers.cpp \
   dispatcher_globals.cpp

WL_COMMON = \
   common/q6_packet.cpp \
   common/q6_client.cpp \
   common/copy_string.cpp

WL_TPCH = \
   tpch/tpch_db.cpp \
   tpch/tpch_db_load.cpp \
   tpch/tpch_compare.cpp \
   tpch/tpch_env.cpp \
   tpch/tpch_type_convert.cpp \
   tpch/drivers/tpch_q1.cpp

# encapsulates all basic classes needed in qpipe
lib_LIBRARIES = libworkload.a

libworkload_a_SOURCES = \
   $(WL_BASE) \
   $(WL_COMMON) \
   $(WL_TPCH)
