# -*- makefile -*-


# Shore related 
SHORE = $(SHORE_HOME)/src
SHORE_CXXFLAGS = -DSOLARIS2 -DSTHREAD_CORE_PTHREAD -DARCH_LP64 -DNO_FASTNEW   -DW_LARGEFILE
SHORE_INCLUDES = -I$(SHORE)/fc -I$(SHORE)/sm -I$(SHORE)/common -I$(SHORE)/sthread -I$(SHORE)/../config $(SHORE_CXXFLAGS)
SHORE_LDADD = -L$(SHORE)/fc -L$(SHORE)/sm -L$(SHORE)/common -L$(SHORE)/sthread -lsm -lsthread -lfc -lcommon -lmtmalloc



# GLOBAL includes
INCLUDES = -I$(top_srcdir)/include $(SHORE_INCLUDES) -I/usr/local/include

# List of libraries
lib_LIBRARIES = \
   lib/libtest.a lib/libworkload.a lib/libsm.a lib/libutil.a

# GLOBAL ld
LDADD = $(lib_LIBRARIES) $(SHORE_LDADD) -lreadline


TESTS = \
   shore_kits



bin_PROGRAMS = $(TESTS)

################################################################################
#
# src/util/
#
################################################################################

SU=$(top_srcdir)/src/util

UTIL = \
	$(SU)/c_str.cpp \
	$(SU)/thread.cpp \
	$(SU)/time_util.cpp \
	$(SU)/trace.cpp \
	$(SU)/chomp.cpp \
        $(SU)/progress.cpp \
	$(SU)/pool_alloc.cpp \
        $(SU)/store_string.cpp \
        $(SU)/history.cpp \
        $(SU)/countdown.cpp \
        $(SU)/confparser.cpp \
        $(SU)/envvar.cpp \
        $(SU)/prcinfo.cpp \
        $(SU)/stl_pool.cpp \
        $(SU)/random_input.cpp \
        $(SU)/shell.cpp

UTIL_CMD = \
        $(SU)/command/tracer.cpp \
        $(SU)/command/printer.cpp


lib_libutil_a_CXXFLAGS = $(AM_CXXFLAGS) 

lib_libutil_a_SOURCES = \
	$(UTIL) \
        $(UTIL_CMD)



################################################################################
#
# src/sm/
#
################################################################################

SM=$(top_srcdir)/src/sm
SM_SHORE = \
   $(SM)/shore/shore_file_desc.cpp \
   $(SM)/shore/shore_field.cpp \
   $(SM)/shore/shore_table.cpp \
   $(SM)/shore/shore_row.cpp \
   $(SM)/shore/shore_sort_buf.cpp \
   $(SM)/shore/shore_env.cpp \
   $(SM)/shore/shore_helper_loader.cpp \
   $(SM)/shore/shore_client.cpp \
   $(SM)/shore/shore_worker.cpp

lib_libsm_a_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)

lib_libsm_a_SOURCES = \
	$(SM_SHORE)





################################################################################
#
# src/workload/
#
################################################################################



SW=$(top_srcdir)/src/workload

WL_TPCC_SHORE = \
   $(SW)/tpcc/tpcc_input.cpp \
   $(SW)/tpcc/shore_tpcc_schema_man.cpp \
   $(SW)/tpcc/shore_tpcc_env.cpp \
   $(SW)/tpcc/shore_tpcc_xct.cpp \
   $(SW)/tpcc/tpcc_random.cpp


WL_TM1_SHORE = \
   $(SW)/tm1/tm1_input.cpp \
   $(SW)/tm1/shore_tm1_schema_man.cpp \
   $(SW)/tm1/shore_tm1_env.cpp \
   $(SW)/tm1/shore_tm1_xct.cpp \
   $(SW)/tm1/shore_tm1_client.cpp

WL_TPCB_SHORE = \
   $(SW)/tpcb/tpcb_input.cpp \
   $(SW)/tpcb/shore_tpcb_xct.cpp \
   $(SW)/tpcb/shore_tpcb_schema_man.cpp \
   $(SW)/tpcb/shore_tpcb_env.cpp \
   $(SW)/tpcb/shore_tpcb_client.cpp


lib_libworkload_a_SOURCES = \
   $(WL_SG_TPCC_SHORE) \
   $(WL_TPCC_SHORE) \
   $(WL_TM1_SHORE) \
   $(WL_TPCB_SHORE)

lib_libworkload_a_INCLUDES = $(INCLUDES) -I$(top_srcdir)/include/workload $(SHORE_INCLUDES)



################################################################################
#
# src/tests/common
#
################################################################################

STC = $(top_srcdir)/src/tests/common

lib_libtest_a_CXXFLAGS = $(AM_CXXFLAGS)

lib_libtest_a_SOURCES = \
   $(STC)/tester_shore_client.cpp \
   $(STC)/tester_shore_shell.cpp



################################################################################
#
# src/tests/
#
################################################################################

ST = $(top_srcdir)/src/tests

TEST_LIBS = lib/libtest.a $(LDADD)

#########
#
# SOURCES
#
#########

shore_kits_SOURCES = $(ST)/shore_kits.cpp


#######
#
# CXXFLAGS/LDADD
#
#######

shore_kits_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)
shore_kits_LDADD = $(TEST_LIBS) -lreadline
