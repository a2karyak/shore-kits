# -*- makefile -*-


INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/include/engine

ENGINE = \
   engine/packet.cpp \
   engine/page.cpp \
   engine/stage_container.cpp \
   engine/tuple.cpp

STAGES = \
   stages/fscan.cpp \
   stages/fdump.cpp \
   stages/tscan.cpp \
   stages/aggregate.cpp \
   stages/func_call.cpp \
   stages/merge.cpp \
   stages/sort.cpp

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
   $(ENGINE) \
   $(DISPATCHER) \
   $(UTIL)  \
   $(STAGES) 
