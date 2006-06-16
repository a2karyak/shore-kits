# -*- makefile -*-


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/include/lib


STAGES = \
   stages/fscan.cpp \
   stages/fdump.cpp \
   stages/tscan.cpp \
   stages/aggregate.cpp \
   stages/hash_join.cpp \
   stages/func_call.cpp \
   stages/merge.cpp \
   stages/sort.cpp

DISPATCHER = \
   dispatcher.cpp

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
   page.cpp \
   tuple.cpp \
   packet.cpp \
   stage_container.cpp \
   $(DISPATCHER) \
   $(UTIL)  \
   $(STAGES) 
