# -*- makefile -*-
INCLUDES = -I$(top_srcdir)/include -I$(BDB_HOME)/include

lib_LIBRARIES = \
   lib/libtest.a lib/libworkload.a lib/libscheduler.a \
   lib/libstages.a lib/libcore.a lib/libutil.a \
   lib/libdbgen.a

LDADD = $(lib_LIBRARIES) $(BDB_HOME)/lib/libdb_cxx.a

#\
   tests/max_timers \
   tests/thread_cputime

TESTS = \
   tests/cpu_binding \
   tests/rand_array_access \
   tests/trace \
   tests/itimer_keypress \
   tests/itimer_threads \
   tests/constants \
   tests/bitvector \
   tests/hashtable \
   tests/send_tuples \
   tests/tuple_buffer \
   tests/context_set \
   tests/context_makecontext \
   tests/context_producer_consumer \
   tests/tpch_struct_sizes \
   tests/fscan_stage \
   tests/fdump_stage \
   tests/tscan_stage \
   tests/customer_load \
   tests/func_call_stage \
   tests/aggregate_stage \
   tests/merge_stage \
   tests/sort_stage \
   tests/hash_join_stage \
   tests/bnl_join_stage \
   tests/bnl_in_stage \
   tests/q4 \
   tests/q6 \
   tests/q6_disp \
   tests/q13 \
   tests/q14 \
   tests/q12 \
   tests/q16 \
   tests/q19 \
   tests/q1 \
   tests/q4 \
   tests/hash_map \
   tests/grouper \
   tests/busy_wait \
   tests/busy_wait_all \
   tests/payment \
   tests/payment_baseline \
   tests/acounter \
   tests/tpch_load \
   tests/tpcc_load \
   tests/tpcc_load_confirm 



DBGEN = \
   dbgen_tpcc




bin_PROGRAMS = qpipe $(TESTS) $(DBGEN)

tests: $(TESTS)

dbgen: $(DBGEN)

################################################################################
#
# src/util/
#
################################################################################

SU=$(top_srcdir)/src/util
lib_libutil_a_CXXFLAGS = $(AM_CXXFLAGS)
lib_libutil_a_SOURCES = \
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
        $(SU)/progress.cpp



################################################################################
#
# src/core/
#
################################################################################

SC=$(top_srcdir)/src/core
lib_libcore_a_SOURCES = \
	$(SC)/dispatcher.cpp \
	$(SC)/packet.cpp \
	$(SC)/stage_container.cpp \
	$(SC)/tuple.cpp \
	$(SC)/tuple_fifo.cpp \
	$(SC)/tuple_fifo_directory.cpp




################################################################################
#
# src/scheduler/
#
################################################################################

SS=$(top_srcdir)/src/scheduler
lib_libscheduler_a_SOURCES = \
	$(SS)/cpu.cpp \
	$(SS)/cpu_set.cpp


################################################################################
#
# src/stages/
#
################################################################################

SG=$(top_srcdir)/src/stages
SG_MAIN = \
   $(SG)/fscan.cpp \
   $(SG)/fdump.cpp \
   $(SG)/tscan.cpp \
   $(SG)/aggregate.cpp \
   $(SG)/partial_aggregate.cpp \
   $(SG)/hash_aggregate.cpp \
   $(SG)/sorted_in.cpp \
   $(SG)/hash_join.cpp \
   $(SG)/pipe_hash_join.cpp \
   $(SG)/func_call.cpp \
   $(SG)/merge.cpp \
   $(SG)/sort.cpp \
   $(SG)/bnl_join.cpp \
   $(SG)/bnl_in.cpp \
   $(SG)/echo.cpp \
   $(SG)/sieve.cpp \

SG_TRX = \
   $(SG)/tpcc/trx_packet.cpp \
   $(SG)/tpcc/payment_baseline.cpp \
   $(SG)/tpcc/payment_begin.cpp \
   $(SG)/tpcc/payment_upd_wh.cpp \
   $(SG)/tpcc/payment_upd_distr.cpp \
   $(SG)/tpcc/payment_upd_cust.cpp \
   $(SG)/tpcc/payment_ins_hist.cpp \
   $(SG)/tpcc/payment_finalize.cpp

SG_COMMON = \
   $(SG)/common/process_query.cpp \
   $(SG)/common/predicates.cpp 

lib_libstages_a_SOURCES = \
   $(SG_MAIN) \
   $(SG_TRX) \
   $(SG_COMMON)



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

WL_COMMON = \
   $(SW)/common/table_load.cpp \
   $(SW)/common/table_compare.cpp \
   $(SW)/common/bdb_env.cpp \
   $(SW)/common/bdb_txn.cpp

WL_TPCH = \
   $(SW)/tpch/tpch_db.cpp \
   $(SW)/tpch/tpch_env.cpp \
   $(SW)/tpch/tpch_tbl_parsers.cpp \
   $(SW)/tpch/tpch_tbl_readers.cpp \
   $(SW)/tpch/tpch_compare.cpp \
   $(SW)/tpch/tpch_type_convert.cpp \
   $(SW)/tpch/drivers/merge_test.cpp  \
   $(SW)/tpch/q6_packet.cpp \
   $(SW)/tpch/drivers/tpch_q1.cpp  \
   $(SW)/tpch/drivers/tpch_q4.cpp  \
   $(SW)/tpch/drivers/tpch_q6.cpp  \
   $(SW)/tpch/drivers/tpch_q12.cpp \
   $(SW)/tpch/drivers/tpch_q13.cpp \
   $(SW)/tpch/drivers/tpch_q14.cpp \
   $(SW)/tpch/drivers/tpch_q16.cpp \
   $(SW)/tpch/drivers/tpch_q19.cpp \
   $(SW)/tpch/drivers/tpch_m_1_4_6_12.cpp \
   $(SW)/tpch/drivers/tpch_upto13.cpp \
   $(SW)/tpch/drivers/tpch_m146.cpp \
   $(SW)/tpch/drivers/tpch_m14612.cpp \
   $(SW)/tpch/drivers/tpch_sim_mix.cpp


WL_TPCC = \
   $(SW)/tpcc/tpcc_db.cpp \
   $(SW)/tpcc/tpcc_env.cpp \
   $(SW)/tpcc/tpcc_tbl_parsers.cpp \
   $(SW)/tpcc/tpcc_tbl_readers.cpp \
   $(SW)/tpcc/tpcc_compare.cpp \
   $(SW)/tpcc/drivers/common.cpp \
   $(SW)/tpcc/drivers/tpcc_payment.cpp \
   $(SW)/tpcc/drivers/tpcc_payment_baseline.cpp


lib_libworkload_a_SOURCES = \
   $(WL_BASE) \
   $(WL_COMMON) \
   $(WL_TPCH) \
   $(WL_TPCC)


lib_libworkload_a_INCLUDES = $(INCLUDES) -I$(top_srcdir)/include/workload



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
   $(STC)/int_tuple_writer.cpp \
   $(STC)/tester_query.cpp \
   $(STC)/cpu_bind.cpp \
   $(STC)/busy_wait.cpp \
   $(STC)/q6_client.cpp


################################################################################
#
# src/tests/
#
################################################################################

ST = $(top_srcdir)/src/tests

TEST_LIBS = lib/libtest.a $(LDADD)

tests_cpu_binding_SOURCES = $(ST)/test_cpu_binding.cpp
tests_rand_array_access_SOURCES = $(ST)/test_rand_array_access.cpp
tests_trace_SOURCES = $(ST)/test_trace.cpp
tests_itimer_keypress_SOURCES = $(ST)/test_itimer_keypress.cpp
tests_itimer_threads_SOURCES = $(ST)/test_itimer_threads.cpp
#tests_thread_cputime_SOURCES = $(ST)/test_thread_cputime.cpp
#tests_max_timers_SOURCES = $(ST)/test_max_timers.cpp
tests_constants_SOURCES = $(ST)/test_constants.cpp
tests_bitvector_SOURCES = $(ST)/test_bitvector.cpp
tests_hashtable_SOURCES = $(ST)/test_hashtable.cpp
tests_send_tuples_SOURCES = $(ST)/test_send_tuples.cpp
tests_tuple_buffer_SOURCES = $(ST)/test_tuple_buffer.cpp
tests_context_set_SOURCES = = $(ST)/test_context_set.cpp
tests_context_makecontext_SOURCES = $(ST)/test_context_makecontext.cpp
tests_context_producer_consumer_SOURCES = $(ST)/test_context_producer_consumer.cpp

tests_fdump_stage_SOURCES = $(ST)/test_fdump_stage.cpp
tests_merge_stage_SOURCES = $(ST)/test_merge_stage.cpp
tests_sort_stage_SOURCES = $(ST)/test_sort_stage.cpp
tests_fscan_stage_SOURCES = $(ST)/test_fscan_stage.cpp
tests_tscan_stage_SOURCES = $(ST)/test_tscan_stage.cpp
tests_customer_load_SOURCES = $(ST)/test_customer_load.cpp
tests_func_call_stage_SOURCES = $(ST)/test_func_call_stage.cpp
tests_aggregate_stage_SOURCES = $(ST)/test_aggregate_stage.cpp
tests_hash_join_stage_SOURCES = $(ST)/test_hash_join_stage.cpp
tests_bnl_join_stage_SOURCES = $(ST)/test_bnl_join_stage.cpp
tests_bnl_in_stage_SOURCES = $(ST)/test_bnl_in_stage.cpp
tests_tpch_struct_sizes_SOURCES = $(ST)/tpch_struct_sizes.cpp
tests_q4_SOURCES = $(ST)/test_q4.cpp
tests_q6_SOURCES = $(ST)/test_q6.cpp
tests_q13_SOURCES = $(ST)/test_q13.cpp
tests_q14_SOURCES = $(ST)/test_q14.cpp
tests_q12_SOURCES = $(ST)/test_q12.cpp
tests_q16_SOURCES = $(ST)/test_q16.cpp
tests_q19_SOURCES = $(ST)/test_q19.cpp
tests_q6_disp_SOURCES = $(ST)/test_q6_disp.cpp
tests_q1_SOURCES = $(ST)/test_q1.cpp
tests_hash_map_SOURCES = $(ST)/test_hash_map.cpp
tests_grouper_SOURCES = $(ST)/grouper.cpp
tests_busy_wait_SOURCES = $(ST)/test_busy_wait.cpp
tests_busy_wait_all_SOURCES = $(ST)/test_busy_wait_all.cpp
tests_payment_SOURCES = $(ST)/test_payment.cpp
tests_payment_baseline_SOURCES = $(ST)/test_payment_baseline.cpp
tests_acounter_SOURCES = $(ST)/test_acounter.cpp
tests_tpch_load_SOURCES = $(ST)/test_tpch_load.cpp
tests_tpcc_load_SOURCES = $(ST)/test_tpcc_load.cpp
tests_tpcc_load_confirm_SOURCES = $(ST)/test_tpcc_load_confirm.cpp

tests_cpu_binding_LDADD = $(TEST_LIBS)
tests_rand_array_access_LDADD = $(TEST_LIBS)
#tests_thread_cputime_LDADD = $(TEST_LIBS) -lrt
#tests_max_timers_LDADD = lib/libutil.a -lrt
tests_trace_LDADD = $(TEST_LIBS)
tests_tuple_buffer_LDADD = $(TEST_LIBS)
tests_bitvector_LDADD = $(TEST_LIBS)
tests_fdump_stage_LDADD = $(TEST_LIBS)
tests_merge_stage_LDADD = $(TEST_LIBS)
tests_sort_stage_LDADD = $(TEST_LIBS)
tests_fscan_stage_LDADD = $(TEST_LIBS)
tests_tscan_stage_LDADD = $(TEST_LIBS)
tests_func_call_stage_LDADD = $(TEST_LIBS)
tests_aggregate_stage_LDADD = $(TEST_LIBS)
tests_hash_join_stage_LDADD = $(TEST_LIBS)
tests_bnl_join_stage_LDADD = $(TEST_LIBS)
tests_bnl_in_stage_LDADD = $(TEST_LIBS)
tests_tpch_struct_sizes_LDADD = $(TEST_LIBS)
tests_q4_LDADD = $(TEST_LIBS)
tests_q6_LDADD = $(TEST_LIBS)
tests_q13_LDADD = $(TEST_LIBS)
tests_q14_LDADD = $(TEST_LIBS)
tests_q12_LDADD = $(TEST_LIBS)
tests_q16_LDADD = $(TEST_LIBS)
tests_q19_LDADD = $(TEST_LIBS)
tests_q6_disp_LDADD = $(TEST_LIBS)
tests_q1_LDADD = $(TEST_LIBS)
tests_hash_map_LDADD = $(TEST_LIBS)
tests_grouper_LDADD = $(TEST_LIBS)
tests_busy_wait_LDADD = $(TEST_LIBS)
tests_busy_wait_all_LDADD = $(TEST_LIBS)
tests_payment_LDADD = $(TEST_LIBS)
tests_payment_baseline_LDADD = $(TEST_LIBS)
tests_tpch_load_LDADD = $(TEST_LIBS)
tests_tpcc_load_LDADD = $(TEST_LIBS)
tests_tpcc_load_confirm_LDADD = $(TEST_LIBS)


################################################################################
#
# src/server/
#
################################################################################

SE = $(top_srcdir)/src/server

# FIXME: FRJ: removed because CC doesn't like it. Need to track down
# its version and select between the two properly.
#qpipe_LDFLAGS = -Wl,-export-dynamic

# FIXME: FRJ: hack to make CC find <ltdl.h> on lomond. Shouldn't hurt anybody else...
# FIXME: IP: addition to make gcc @ ia64 find <ltdl.h>. Should't hurt anybody else...
qpipe_CXXFLAGS = $(AM_CXX_FLAGS) -I/usr/local/include -I/usr/share/libtool/libltdl
qpipe_LDADD = $(LDADD) \
	-lreadline \
	-lltdl \
	-lcurses

COMMANDS = \
   $(SE)/command/printer.cpp \
   $(SE)/command/load_handler.cpp \
   $(SE)/command/tpch_handler.cpp \
   $(SE)/command/tpcc_handler.cpp \
   $(SE)/command/tracer.cpp


qpipe_SOURCES = \
   $(SE)/server.cpp \
   $(SE)/command_set.cpp \
   $(SE)/interactive_mode.cpp \
   $(SE)/history.cpp \
   $(SE)/process_next_command_using_fgets.cpp \
   $(SE)/network_mode.cpp \
   $(COMMANDS)
