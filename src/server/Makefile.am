# -*- makefile -*-

INCLUDES = -I$(top_srcdir)/include 

# libtestcommon temporarily added so we can use register_stage()
LDBASE = $(top_srcdir)/src

LDADD = \
	$(LDBASE)/workload/libworkload.a \
	$(LDBASE)/scheduler/libscheduler.a \
	$(LDBASE)/stages/libstages.a \
	$(LDBASE)/core/libcore.a \
	$(LDBASE)/util/libutil.a \
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
   history.cpp \
   $(COMMANDS)
