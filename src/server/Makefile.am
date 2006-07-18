# -*- makefile -*-

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_srcdir)/include/server \
           -I$(top_srcdir)/include/workload

# libtestcommon temporarily added so we can use register_stage()

LDADD = ../engine/libqpipe.a \
        ../trace/libtrace.a  \
        ../workload/libworkload.a \
        ../engine/libqpipe.a \
        ../tests/common/libtestcommon.a \
        $(BDB_HOME)/lib/libdb_cxx.a

# define programs here
bin_PROGRAMS = qpipe

qpipe_SOURCES = \
   server.cpp
