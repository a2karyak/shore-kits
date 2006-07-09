# -*- makefile -*-

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_srcdir)/include/server \
           -I$(top_srcdir)/include/workload

LDADD = ../engine/libqpipe.a \
        ../trace/libtrace.a  \
        ../workload/libworkload.a \
        ../engine/libqpipe.a


# define programs here
bin_PROGRAMS = \
   qpipe

qpipe_SOURCES = \
   server.cpp
