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
        $(BDB_HOME)/lib/libdb_cxx.a \
        -lreadline \
        -lcurses

# define programs here
bin_PROGRAMS = qpipe


COMMANDS = \
   command/printer.cpp \
   command/tpch_handler.cpp

qpipe_SOURCES = \
   server.cpp \
   command_set.cpp \
   interactive_mode.cpp \
   $(COMMANDS)
