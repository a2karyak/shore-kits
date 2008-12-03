# -*- makefile -*-


# Shore related 
SHORE = $(SHORE_HOME)/src
SHORE_CXXFLAGS = -DSOLARIS2 -DSTHREAD_CORE_PTHREAD -DARCH_LP64 -DNO_FASTNEW   -DW_LARGEFILE
SHORE_INCLUDES = -I$(SHORE)/fc -I$(SHORE)/sm -I$(SHORE)/common -I$(SHORE)/sthread -I$(SHORE)/../config $(SHORE_CXXFLAGS)
SHORE_LDADD = -L$(SHORE)/fc -L$(SHORE)/sm -L$(SHORE)/common -L$(SHORE)/sthread -lsm -lsthread -lfc -lcommon -lmtmalloc


# GLOBAL includes
INCLUDES = -I$(top_srcdir)/include -I$(BOOST_HOME) $(SHORE_INCLUDES) -I/usr/local/include

# List of libraries
lib_LIBRARIES = \
   lib/libtest.a lib/libworkload.a lib/libscheduler.a \
   lib/libstages.a lib/libcore.a lib/libdora.a lib/libsm.a lib/libutil.a \
   lib/libdbgen.a

# GLOBAL ld
LDADD = $(lib_LIBRARIES) $(SHORE_LDADD) -lreadline


TESTS = \
   tests/alloc \
   tests/cpu_binding \
   tests/trace \
   tests/itimer_keypress \
   tests/itimer_threads \
   tests/bitvector \
   tests/hashtable \
   tests/context_set \
   tests/context_makecontext \
   tests/context_producer_consumer \
   tests/hash_map \
   tests/busy_wait \
   tests/busy_wait_all \
   tests/acounter \
   tests/clh_lock \
   tests/clh_rwlock \
   tests/bptree \
   tests/raii_file \
   tests/config_file_example \
   tests/row_cache \
   tests/tpcc_load_shore \
   tests/tpcc_kit_shell \
   tests/shore_trees \
   tests/simple \
   tests/shore_rvp

DBGEN = \
   dbgen_tpcc



#bin_PROGRAMS = stagedtrx $(TESTS) $(DBGEN)
bin_PROGRAMS = $(TESTS) $(DBGEN)

tests: $(TESTS)

dbgen: $(DBGEN)


################################################################################
#
# src/util/
#
################################################################################

SU=$(top_srcdir)/src/util

UTIL = \
	$(SU)/c_str.cpp \
	$(SU)/fnv.cpp \
	$(SU)/hash_functions.cpp \
	$(SU)/static_hash_map.cpp \
	$(SU)/thread.cpp \
	$(SU)/time_util.cpp \
	$(SU)/tmpfile.cpp \
	$(SU)/trace.cpp \
	$(SU)/tcp.cpp \
	$(SU)/rio.cpp \
	$(SU)/chomp.cpp \
        $(SU)/fileops.cpp \
        $(SU)/static_list_node.cpp \
        $(SU)/static_list.cpp \
        $(SU)/resource_pool.cpp \
        $(SU)/itimer.cpp \
        $(SU)/progress.cpp \
	$(SU)/pool_alloc.cpp \
	$(SU)/clh_lock.cpp \
	$(SU)/clh_rwlock.cpp \
	$(SU)/clh_profile.cpp \
        $(SU)/store_string.cpp \
        $(SU)/history.cpp \
        $(SU)/countdown.cpp \
        $(SU)/confparser.cpp \
        $(SU)/envvar.cpp \
        $(SU)/prcinfo.cpp \
        $(SU)/stl_pool.cpp \
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
# src/core/
#
################################################################################

SC=$(top_srcdir)/src/core
SC_BASE = \
	$(SC)/dispatcher.cpp \
	$(SC)/packet.cpp \
	$(SC)/trx_packet.cpp \
	$(SC)/stage_container.cpp \
	$(SC)/tuple.cpp \
	$(SC)/tuple_fifo.cpp \
	$(SC)/tuple_fifo_directory.cpp

lib_libcore_a_SOURCES = \
	$(SC_BASE)



################################################################################
#
# src/scheduler/
#
################################################################################

SS=$(top_srcdir)/src/scheduler
SS_BASE = \
	$(SS)/cpu.cpp \
	$(SS)/cpu_set.cpp


lib_libscheduler_a_SOURCES = \
	$(SS_BASE)



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
# src/stages/
#
################################################################################

SG=$(top_srcdir)/src/stages

SG_COMMON = \
   $(SG)/common/process_query.cpp \
   $(SG)/common/predicates.cpp


lib_libstages_a_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)

lib_libstages_a_SOURCES = \
   $(SG_COMMON)




################################################################################
#
# src/dora/
#
################################################################################

SD=$(top_srcdir)/src/dora
DO_BASE = \
   $(SD)/lockman.cpp \
   $(SD)/action.cpp \
   $(SD)/partition.cpp \
   $(SD)/rvp.cpp



lib_libdora_a_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)

lib_libdora_a_SOURCES = \
   $(DO_BASE)


################################################################################
#
# src/workload/
#
################################################################################



SW=$(top_srcdir)/src/workload
WL_BASE = \
   $(SW)/workload.cpp \
   $(SW)/workload_client.cpp \
   $(SW)/register_stage_containers.cpp \
   $(SW)/dispatcher_globals.cpp

WL_TPCC_COMMON = \
   $(SG)/tpcc/common/tpcc_input.cpp \
   $(SG)/tpcc/common/tpcc_trx_input.cpp \
   $(SG)/tpcc/common/tpcc_tbl_parsers.cpp \
   $(SG)/tpcc/common/tpcc_scaling_factor.cpp \
   $(SG)/tpcc/common/tpcc_random.cpp

WL_TPCC_SHORE = \
   $(SW)/tpcc/shore_tpcc_random.cpp \
   $(SW)/tpcc/shore_tpcc_schema.cpp \
   $(SW)/tpcc/shore_tpcc_schema_man.cpp \
   $(SW)/tpcc/shore_tpcc_schema_gen.cpp \
   $(SW)/tpcc/shore_tpcc_env.cpp \
   $(SW)/tpcc/shore_tpcc_xct.cpp \
   $(SW)/tpcc/shore_tpcc_worker.cpp \
   $(SW)/tpcc/shore_tpcc_load.cpp


WL_TPCC_DORA = \
   $(SD)/tpcc/dora_payment.cpp \
   $(SD)/tpcc/dora_new_order.cpp \
   $(SD)/tpcc/dora_mbench.cpp \
   $(SD)/tpcc/dora_tpcc.cpp \
   $(SD)/tpcc/dora_tpcc_xct.cpp


############## DEPRECATED ##################################
WL_TPCC_SHORE_STAGES_DEPR = \
   $(SG)/tpcc/shore/shore_payment_baseline.cpp \
   $(SG)/tpcc/shore/shore_new_order_baseline.cpp \
   $(SG)/tpcc/shore/shore_order_status_baseline.cpp \
   $(SG)/tpcc/shore/shore_delivery_baseline.cpp \
   $(SG)/tpcc/shore/shore_stock_level_baseline.cpp

WL_TPCC_SHORE_STAGED_DEPR = \
   $(SG)/tpcc/shore/staged/shore_payment_functions.cpp \
   $(SG)/tpcc/shore/staged/shore_payment_begin.cpp \
   $(SG)/tpcc/shore/staged/shore_payment_finalize.cpp \
   $(SG)/tpcc/shore/staged/shore_payment_ins_hist.cpp \
   $(SG)/tpcc/shore/staged/shore_payment_upd_cust.cpp \
   $(SG)/tpcc/shore/staged/shore_payment_upd_distr.cpp \
   $(SG)/tpcc/shore/staged/shore_payment_upd_wh.cpp \
   $(SG)/tpcc/shore/staged/shore_new_order_functions.cpp \
   $(SG)/tpcc/shore/staged/shore_new_order_begin.cpp \
   $(SG)/tpcc/shore/staged/shore_new_order_finalize.cpp \
   $(SG)/tpcc/shore/staged/shore_new_order_outside_loop.cpp \
   $(SG)/tpcc/shore/staged/shore_new_order_one_ol.cpp

WL_TPCC_SHORE_DRIVERS_DEPR = \
   $(SW)/tpcc/drivers/shore/shore_tpcc_new_order_baseline.cpp \
   $(SW)/tpcc/drivers/shore/shore_tpcc_new_order_staged.cpp \
   $(SW)/tpcc/drivers/shore/shore_tpcc_payment_baseline.cpp \
   $(SW)/tpcc/drivers/shore/shore_tpcc_payment_staged.cpp \
   $(SW)/tpcc/drivers/shore/shore_tpcc_order_status_baseline.cpp \
   $(SW)/tpcc/drivers/shore/shore_tpcc_delivery_baseline.cpp \
   $(SW)/tpcc/drivers/shore/shore_tpcc_stock_level_baseline.cpp \
   $(SW)/tpcc/drivers/shore/shore_tpcc_mix_baseline.cpp
#############################################################

lib_libworkload_a_SOURCES = \
   $(WL_BASE) \
   $(WL_TPCC_COMMON) \
   $(WL_SG_TPCC_SHORE) \
   $(WL_TPCC_SHORE) \
   $(WL_TPCC_DORA) \
   $(WL_TPCC_SHORE_STAGES_DEPR) \
   $(WL_TPCC_SHORE_STAGED_DEPR) \
   $(WL_TPCC_SHORE_DRIVERS_DEPR)



lib_libworkload_a_INCLUDES = $(INCLUDES) -I$(top_srcdir)/include/workload $(SHORE_INCLUDES)




################################################################################
#
# src/workload/ (dbgen)
#
################################################################################

SDBGEN=$(top_srcdir)/src/workload

WL_TPCC_DBGEN = \
   $(SDBGEN)/tpcc/dbgen/gendata.cpp \
   $(SDBGEN)/tpcc/dbgen/tpcc_misc.cpp \
   $(SDBGEN)/tpcc/dbgen/tpcc_rnd.cpp

lib_libdbgen_a_SOURCES = \
   $(WL_TPCC_DBGEN)

lib_libdbgen_a_INCLUDES = $(INCLUDES) -I$(top_srcdir)/include/workload

dbgen_tpcc_SOURCES = $(SDBGEN)/tpcc/dbgen/gendata.cpp

dbgen_tpcc_LDADD = lib/libdbgen.a $(LDADD)


################################################################################
#
# src/tests/common
#
################################################################################

STC = $(top_srcdir)/src/tests/common

lib_libtest_a_CXXFLAGS = $(AM_CXXFLAGS)

lib_libtest_a_SOURCES = \
   $(STC)/drive_stage.cpp \
   $(STC)/tester_query.cpp \
   $(STC)/cpu_bind.cpp \
   $(STC)/busy_wait.cpp \
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

tests_alloc_SOURCES = $(ST)/test_alloc.cpp
tests_cpu_binding_SOURCES = $(ST)/test_cpu_binding.cpp
tests_trace_SOURCES = $(ST)/test_trace.cpp
tests_itimer_keypress_SOURCES = $(ST)/test_itimer_keypress.cpp
tests_itimer_threads_SOURCES = $(ST)/test_itimer_threads.cpp
tests_bitvector_SOURCES = $(ST)/test_bitvector.cpp
tests_hashtable_SOURCES = $(ST)/test_hashtable.cpp
tests_context_set_SOURCES = = $(ST)/test_context_set.cpp
tests_context_makecontext_SOURCES = $(ST)/test_context_makecontext.cpp
tests_context_producer_consumer_SOURCES = $(ST)/test_context_producer_consumer.cpp
tests_hash_map_SOURCES = $(ST)/test_hash_map.cpp
tests_busy_wait_SOURCES = $(ST)/test_busy_wait.cpp
tests_busy_wait_all_SOURCES = $(ST)/test_busy_wait_all.cpp
tests_acounter_SOURCES = $(ST)/test_acounter.cpp
tests_clh_lock_SOURCES = $(ST)/test_clh_lock.cpp
tests_clh_rwlock_SOURCES = $(ST)/test_clh_rwlock.cpp
tests_bptree_SOURCES = $(ST)/test_bptree.cpp
tests_raii_file_SOURCES = $(ST)/test_file.cpp
tests_config_file_example_SOURCES = $(ST)/config_file_example.cpp

tests_tpcc_load_shore_SOURCES = $(ST)/shore_tpcc_load.cpp
tests_tpcc_kit_shell_SOURCES = $(ST)/tpcc_kit_shell.cpp
tests_shore_rvp_SOURCES = $(ST)/shore_test_rvp.cpp
tests_row_cache_SOURCES = $(ST)/test_row_cache.cpp
tests_shore_trees_SOURCES = $(ST)/shore_tpcc_trees.cpp

tests_simple_SOURCES = $(ST)/simple_test.cpp


# exec files that need Shore
tests_tpcc_load_shore_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)
tests_tpcc_load_shore_LDADD = $(TEST_LIBS)
tests_tpcc_kit_shell_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)
tests_tpcc_kit_shell_LDADD = $(TEST_LIBS) -lreadline

tests_shore_rvp_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)
tests_shore_rvp_LDADD = $(TEST_LIBS) -lreadline
tests_row_cache_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)
tests_row_cache_LDADD = $(TEST_LIBS)
tests_shore_trees_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES) -I/usr/local/include
tests_shore_trees_LDADD = $(TEST_LIBS) -lreadline

tests_simple_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)
tests_simple_LDADD = $(TEST_LIBS)


#######
#
# LDADD
#
#######

tests_cpu_binding_LDADD = $(TEST_LIBS)
tests_trace_LDADD = $(TEST_LIBS)
tests_bitvector_LDADD = $(TEST_LIBS)
tests_hash_map_LDADD = $(TEST_LIBS)
tests_busy_wait_LDADD = $(TEST_LIBS)
tests_busy_wait_all_LDADD = $(TEST_LIBS)
tests_bptree_LDADD = $(TEST_LIBS)
tests_clh_lock_LDADD = $(TEST_LIBS)
tests_clh_rwlock_LDADD = $(TEST_LIBS)
tests_raii_file_LDADD = $(TEST_LIBS)
tests_config_file_example_LDADD = $(TEST_LIBS)


################################################################################
#
# src/server/
#
################################################################################

SE = $(top_srcdir)/src/server

# FIXME: FRJ: removed because CC doesn't like it. Need to track down
# its version and select between the two properly.
#stagedtrx_LDFLAGS = -Wl,-export-dynamic

# FIXME: FRJ: hack to make CC find <ltdl.h> on lomond. Shouldn't hurt anybody else...
# FIXME: IP: addition to make gcc @ ia64 find <ltdl.h>. Shouldn't hurt anybody else...
stagedtrx_CXXFLAGS = $(AM_CXX_FLAGS) -I/usr/local/include -I/usr/share/libtool/libltdl
stagedtrx_LDADD = $(LDADD) \
		-lltdl

COMMANDS = \
   $(SE)/command/load_handler.cpp \
   $(SE)/command/shore_tpcc_handler.cpp



stagedtrx_SOURCES = \
   $(SE)/server.cpp \
   $(SE)/command_set.cpp \
   $(SE)/interactive_mode.cpp \
   $(SE)/process_next_command_using_fgets.cpp \
   $(SE)/network_mode.cpp \
   $(COMMANDS)
