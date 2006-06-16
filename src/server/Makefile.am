# -*- makefile -*-

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_srcdir)/include/server \
           -I$(top_srcdir)/include/workload

LDADD = ../lib/libqpipe.a ../trace/libtrace.a

# define programs here
bin_PROGRAMS = \
   qpipe


qpipe_SOURCES = qpipe.cpp ../workload/workload.cpp ../workload/parser.cpp


