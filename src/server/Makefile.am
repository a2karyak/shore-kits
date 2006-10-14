# -*- makefile -*-
SUBDIRS = extras

INCLUDES = -I$(top_srcdir)/include 

# libtestcommon temporarily added so we can use register_stage()
LDBASE = $(top_srcdir)/src
AM_LDFLAGS = -Wl,-export-dynamic

LDADD = \
	$(LDBASE)/workload/libworkload.a \
	$(LDBASE)/scheduler/libscheduler.a \
	$(LDBASE)/stages/libstages.a \
	$(LDBASE)/core/libcore.a \
	$(LDBASE)/util/libutil.a \
	$(BDB_HOME)/lib/libdb_cxx.a \
	$(LDBASE)/bdb-ext/libbdbext.a \
	-lrt \
	-laio \
	-lreadline \
	-lltdl \
	-lcurses

# define programs here
bin_PROGRAMS = qpipe


COMMANDS = \
   command/printer.cpp \
   command/load_handler.cpp \
   command/tpch_handler.cpp

qpipe_SOURCES = \
   server.cpp \
   command_set.cpp \
   interactive_mode.cpp \
   history.cpp \
   process_next_command_using_fgets.cpp \
   network_mode.cpp \
   $(COMMANDS)
