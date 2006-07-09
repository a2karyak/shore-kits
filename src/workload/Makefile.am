# -*- makefile -*-


INCLUDES = -I$(top_srcdir)/include \
           -I$(top_srcdir)/include/workload

WL_COMMON = \
   workload.cpp \
   client.cpp \
   parser.cpp \
   job.cpp \
   job_directory.cpp \
   register_job_drivers.cpp

WL_TPCH = \
   tpch/tpch_db.cpp \
   tpch/tpch_db_load.cpp \
   tpch/tpch_compare.cpp \
   tpch/tpch_env.cpp \
   tpch/tpch_type_convert.cpp

# encapsulates all basic classes needed in qpipe
lib_LIBRARIES = libworkload.a

libworkload_a_SOURCES = \
   $(WL_COMMON) \
   $(WL_TPCH)
