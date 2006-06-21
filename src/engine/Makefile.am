# -*- makefile -*-


INCLUDES = -I$(top_srcdir)/include

CORE = \
   core/page.cpp \
   core/packet.cpp \
   core/tuple.cpp \
   core/stage_container.cpp

STAGES = \
   stages/fscan.cpp \
   stages/fdump.cpp \
   stages/tscan.cpp \
   stages/aggregate.cpp \
   stages/partial_aggregate.cpp \
   stages/hash_join.cpp \
   stages/func_call.cpp \
   stages/merge.cpp \
   stages/sort.cpp \
   stages/bnl_join.cpp \
   stages/bnl_in.cpp

DISPATCHER = \
   dispatcher/dispatcher.cpp

UTIL = \
   util/static_hash_map.cpp \
   util/hash_functions.cpp \
   util/shuffle.cpp \
   util/tmpfile.cpp \
   util/fnv.cpp \
   util/time_util.cpp



# encapsulates all basic classes needed in qpipe
lib_LIBRARIES = libqpipe.a

libqpipe_a_SOURCES = \
   thread.cpp \
   $(CORE) \
   $(DISPATCHER) \
   $(UTIL)  \
   $(STAGES) 
