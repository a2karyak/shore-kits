# -*- makefile -*-

AUTOMAKE_OPTIONS = subdir-objects

# Shore related 
SHORE_SRC = @SHORE_HOME@/src
SHORE_CXXFLAGS = -DSOLARIS2 -DSTHREAD_CORE_PTHREAD -DARCH_LP64 -DNO_FASTNEW -DW_LARGEFILE -D_POSIX_PTHREAD_SEMANTICS -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS -w

SHORE_INCLUDES = -I$(SHORE_SRC)/fc -I$(SHORE_SRC)/sm -I$(SHORE_SRC)/common -I$(SHORE_SRC)/sthread -I$(SHORE_SRC)/../config $(SHORE_CXXFLAGS) -I$(SHORE_SRC)/atomic_ops

SHORE_LDADD = \
	$(SHORE_SRC)/sm/libsm.a \
	$(SHORE_SRC)/common/libcommon.a \
	$(SHORE_SRC)/sthread/libsthread.a \
	$(SHORE_SRC)/fc/libfc.a


### -lmtmalloc <-- which malloc library will be included is responsibility of configure

SHORE_LDADD += $(SHORE_SRC)/atomic_ops/libatomic_ops.a

READLINE_INCLUDES = @READLINE_INCLUDE@
READLINE_LIBS = @READLINE_LIB@

if HAVE_GLIBTOP
CPUMON_INCLUDES = @GLIBTOP_INCLUDE@
CPUMON_LIBS = @GLIBTOP_LIB@
CPUMON_SRC = src/util/linux_procstat.cpp src/util/topinfo.cpp
else
if HAVE_PROCFS
CPUMON_INCLUDES = @PROCFS_INCLUDE@
CPUMON_LIBS = @PROCFS_LIB@
CPUMON_SRC = src/util/sunos_procstat.cpp src/util/prcinfo.cpp
endif
endif

VTUNE_INCLUDES = @VTUNE_INCLUDES@
VTUNE_LIBS = @VTUNE_LIBS@

# GLOBAL includes/ld
AM_CPPFLAGS = -I$(top_srcdir)/include $(SHORE_INCLUDES) \
	$(READLINE_INCLUDES) $(CPUMON_INCLUDES) $(VTUNE_INCLUDES)
LDADD = $(lib_LIBRARIES) $(SHORE_LDADD) $(READLINE_LIBS) $(CPUMON_LIBS) $(VTUNE_LIBS)

# List of libraries

DORA_LIBS = lib/libdoraworkload.a lib/libdora.a

if USE_QPIPE
QPIPE_LIBS = lib/libqpipe.a lib/libqpipeutil.a
else
QPIPE_LIBS =
endif

lib_LIBRARIES = $(DORA_LIBS) \
      lib/libworkload.a $(QPIPE_LIBS) lib/libsm.a lib/libutil.a

FE = \
   shore_kits

bin_PROGRAMS = $(FE)

debug-shore.so: debug-shore.cpp
	$(CXXCOMPILE) -xcode=pic13 -G -o $@ $<


################################################################################
#
# src/util/
#
################################################################################

UTIL = \
	src/util/c_str.cpp \
	src/util/thread.cpp \
	src/util/time_util.cpp \
	src/util/trace.cpp \
	src/util/chomp.cpp \
        src/util/progress.cpp \
	src/util/pool_alloc.cpp \
        src/util/store_string.cpp \
        src/util/history.cpp \
        src/util/countdown.cpp \
        src/util/confparser.cpp \
        src/util/envvar.cpp \
        src/util/stl_pool.cpp \
        src/util/random_input.cpp \
        src/util/shell.cpp \
        src/util/tcp.cpp \
        src/util/w_strlcpy.cpp \
	src/util/procstat.cpp \
	src/util/skewer.cpp \
        $(CPUMON_SRC)

UTIL_CMD = \
        src/util/command/tracer.cpp \
        src/util/command/printer.cpp


lib_libutil_a_CXXFLAGS = $(AM_CXXFLAGS) 

lib_libutil_a_SOURCES = \
	$(UTIL) \
        $(UTIL_CMD)



################################################################################
#
# src/sm/
#
################################################################################

SM_SHORE = \
   src/sm/shore/shore_file_desc.cpp \
   src/sm/shore/shore_field.cpp \
   src/sm/shore/shore_table.cpp \
   src/sm/shore/shore_row.cpp \
   src/sm/shore/shore_index.cpp \
   src/sm/shore/shore_asc_sort_buf.cpp \
   src/sm/shore/shore_desc_sort_buf.cpp \
   src/sm/shore/shore_reqs.cpp \
   src/sm/shore/shore_flusher.cpp \
   src/sm/shore/shore_env.cpp \
   src/sm/shore/shore_helper_loader.cpp \
   src/sm/shore/shore_client.cpp \
   src/sm/shore/shore_worker.cpp \
   src/sm/shore/shore_trx_worker.cpp \
   src/sm/shore/shore_iter.cpp \
   src/sm/shore/shore_shell.cpp

lib_libsm_a_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)

lib_libsm_a_SOURCES = \
	$(SM_SHORE)





################################################################################
#
# src/workload/
#
################################################################################



WL_TPCC_SHORE = \
   src/workload/tpcc/tpcc_random.cpp \
   src/workload/tpcc/tpcc_input.cpp \
   src/workload/tpcc/shore_tpcc_schema.cpp \
   src/workload/tpcc/shore_tpcc_schema_man.cpp \
   src/workload/tpcc/shore_tpcc_env.cpp \
   src/workload/tpcc/shore_tpcc_xct.cpp \
   src/workload/tpcc/shore_tpcc_client.cpp

WL_TM1_SHORE = \
   src/workload/tm1/tm1_input.cpp \
   src/workload/tm1/shore_tm1_schema.cpp \
   src/workload/tm1/shore_tm1_schema_man.cpp \
   src/workload/tm1/shore_tm1_env.cpp \
   src/workload/tm1/shore_tm1_xct.cpp \
   src/workload/tm1/shore_tm1_client.cpp

WL_TPCB_SHORE = \
   src/workload/tpcb/tpcb_input.cpp \
   src/workload/tpcb/shore_tpcb_schema.cpp \
   src/workload/tpcb/shore_tpcb_schema_man.cpp \
   src/workload/tpcb/shore_tpcb_env.cpp \
   src/workload/tpcb/shore_tpcb_xct.cpp \
   src/workload/tpcb/shore_tpcb_client.cpp

WL_TPCH_SHORE = \
   src/workload/tpch/tpch_random.cpp \
   src/workload/tpch/tpch_input.cpp \
   src/workload/tpch/tpch_util.cpp \
   src/workload/tpch/shore_tpch_schema.cpp \
   src/workload/tpch/shore_tpch_schema_man.cpp \
   src/workload/tpch/shore_tpch_env.cpp \
   src/workload/tpch/shore_tpch_xct.cpp \
   src/workload/tpch/shore_tpch_client.cpp

WL_TPCH_DBGEN_SHORE = \
   src/workload/tpch/dbgen/build.cpp \
   src/workload/tpch/dbgen/driver.cpp \
   src/workload/tpch/dbgen/bm_utils.cpp \
   src/workload/tpch/dbgen/rnd.cpp \
   src/workload/tpch/dbgen/print.cpp \
   src/workload/tpch/dbgen/load_stub.cpp \
   src/workload/tpch/dbgen/bcd2.cpp \
   src/workload/tpch/dbgen/speed_seed.cpp \
   src/workload/tpch/dbgen/text.cpp \
   src/workload/tpch/dbgen/permute.cpp \
   src/workload/tpch/dbgen/rng64.cpp

WL_TPCH_QPIPE = \
   src/workload/tpch/qpipe_tpch_xct.cpp \
   src/workload/tpch/qpipe/qpipe_tpch_qlineitem.cpp \
   src/workload/tpch/qpipe/qpipe_tpch_qcustomer.cpp \
   src/workload/tpch/qpipe/qpipe_tpch_qnation.cpp \
   src/workload/tpch/qpipe/qpipe_tpch_qregion.cpp \
   src/workload/tpch/qpipe/qpipe_tpch_qpart.cpp \
   src/workload/tpch/qpipe/qpipe_tpch_qpartsupp.cpp \
   src/workload/tpch/qpipe/qpipe_tpch_qsupplier.cpp \
   src/workload/tpch/qpipe/qpipe_tpch_qorders.cpp \
   src/workload/tpch/qpipe/qpipe_q1.cpp \
   src/workload/tpch/qpipe/qpipe_q2.cpp \
   src/workload/tpch/qpipe/qpipe_q3.cpp \
   src/workload/tpch/qpipe/qpipe_q4.cpp \
   src/workload/tpch/qpipe/qpipe_q5.cpp \
   src/workload/tpch/qpipe/qpipe_q6.cpp \
   src/workload/tpch/qpipe/qpipe_q7.cpp \
   src/workload/tpch/qpipe/qpipe_q8.cpp \
   src/workload/tpch/qpipe/qpipe_q9.cpp \
   src/workload/tpch/qpipe/qpipe_q10.cpp \
   src/workload/tpch/qpipe/qpipe_q11.cpp \
   src/workload/tpch/qpipe/qpipe_q12.cpp \
   src/workload/tpch/qpipe/qpipe_q13.cpp \
   src/workload/tpch/qpipe/qpipe_q14.cpp \
   src/workload/tpch/qpipe/qpipe_q15.cpp \
   src/workload/tpch/qpipe/qpipe_q16.cpp \
   src/workload/tpch/qpipe/qpipe_q17.cpp \
   src/workload/tpch/qpipe/qpipe_q18.cpp \
   src/workload/tpch/qpipe/qpipe_q19.cpp \
   src/workload/tpch/qpipe/qpipe_q20.cpp \
   src/workload/tpch/qpipe/qpipe_q21.cpp \
   src/workload/tpch/qpipe/qpipe_q22.cpp

WL_SSB_SHORE = \
   src/workload/ssb/ssb_random.cpp \
   src/workload/ssb/ssb_input.cpp \
   src/workload/ssb/ssb_util.cpp \
   src/workload/ssb/shore_ssb_schema.cpp \
   src/workload/ssb/shore_ssb_schema_man.cpp \
   src/workload/ssb/shore_ssb_env.cpp \
   src/workload/ssb/shore_ssb_xct.cpp \
   src/workload/ssb/shore_ssb_client.cpp

WL_SSB_DBGEN_SHORE = \
   src/workload/ssb/dbgen/ssb_build.cpp \
   src/workload/ssb/dbgen/ssb_driver.cpp \
   src/workload/ssb/dbgen/ssb_bm_utils.cpp \
   src/workload/ssb/dbgen/ssb_rnd.cpp \
   src/workload/ssb/dbgen/ssb_print.cpp \
   src/workload/ssb/dbgen/ssb_load_stub.cpp \
   src/workload/ssb/dbgen/ssb_bcd2.cpp \
   src/workload/ssb/dbgen/ssb_speed_seed.cpp \
   src/workload/ssb/dbgen/ssb_text.cpp \
   src/workload/ssb/dbgen/ssb_permute.cpp 

WL_SSB_QPIPE = \
   src/workload/ssb/qpipe_ssb_xct.cpp \
   src/workload/ssb/qpipe/qpipe_q1_1.cpp \
   src/workload/ssb/qpipe/qpipe_q1_2.cpp \
   src/workload/ssb/qpipe/qpipe_q1_3.cpp \
   src/workload/ssb/qpipe/qpipe_q2_1.cpp \
   src/workload/ssb/qpipe/qpipe_q2_2.cpp \
   src/workload/ssb/qpipe/qpipe_q2_3.cpp \
   src/workload/ssb/qpipe/qpipe_q3_1.cpp \
   src/workload/ssb/qpipe/qpipe_q3_2.cpp \
   src/workload/ssb/qpipe/qpipe_q3_3.cpp \
   src/workload/ssb/qpipe/qpipe_q3_4.cpp \
   src/workload/ssb/qpipe/qpipe_q4_1.cpp \
   src/workload/ssb/qpipe/qpipe_q4_2.cpp \
   src/workload/ssb/qpipe/qpipe_q4_3.cpp \
   src/workload/ssb/qpipe/qpipe_qdate.cpp \
   src/workload/ssb/qpipe/qpipe_qpart.cpp \
   src/workload/ssb/qpipe/qpipe_qsupplier.cpp \
   src/workload/ssb/qpipe/qpipe_qcustomer.cpp \
   src/workload/ssb/qpipe/qpipe_qlineorder.cpp \
   src/workload/ssb/qpipe/qpipe_qtest.cpp 

WL_TPCE_SHORE = \
   src/workload/tpce/tpce_input.cpp \
   src/workload/tpce/shore_tpce_schema.cpp \
   src/workload/tpce/shore_tpce_schema_man.cpp \
   src/workload/tpce/shore_tpce_env.cpp \
   src/workload/tpce/shore_tpce_client.cpp

WL_TPCE_SHORE_XCTS = \
   src/workload/tpce/shore_tpce_xct_populate.cpp \
   src/workload/tpce/shore_tpce_xct_trade_order.cpp \
   src/workload/tpce/shore_tpce_xct_trade_result.cpp \
   src/workload/tpce/shore_tpce_xct_trade_lookup.cpp \
   src/workload/tpce/shore_tpce_xct_customer_position.cpp \
   src/workload/tpce/shore_tpce_xct_market_feed.cpp \
   src/workload/tpce/shore_tpce_xct_trade_cleanup.cpp \
   src/workload/tpce/shore_tpce_xct_market_watch.cpp \
   src/workload/tpce/shore_tpce_xct_data_maintenance.cpp \
   src/workload/tpce/shore_tpce_xct_security_detail.cpp \
   src/workload/tpce/shore_tpce_xct_trade_status.cpp \
   src/workload/tpce/shore_tpce_xct_broker_volume.cpp \
   src/workload/tpce/shore_tpce_xct_trade_update.cpp

WL_TPCE_SHORE_EGEN = \
   src/workload/tpce/egen/DateTime.cpp \
   src/workload/tpce/egen/error.cpp \
   src/workload/tpce/egen/Random.cpp \
   src/workload/tpce/egen/Money.cpp \
   src/workload/tpce/egen/EGenVersion.cpp \
   src/workload/tpce/egen/locking.cpp \
   src/workload/tpce/egen/threading.cpp \
   src/workload/tpce/egen/BaseLogger.cpp \
   src/workload/tpce/egen/EGenLogFormatterTab.cpp \
   src/workload/tpce/egen/EGenLoader.cpp \
   src/workload/tpce/egen/MEE.cpp \
   src/workload/tpce/egen/MEEPriceBoard.cpp \
   src/workload/tpce/egen/MEESecurity.cpp \
   src/workload/tpce/egen/MEETickerTape.cpp \
   src/workload/tpce/egen/MEETradingFloor.cpp \
   src/workload/tpce/egen/WheelTime.cpp \
   src/workload/tpce/egen/AddressTable.cpp \
   src/workload/tpce/egen/CustomerSelection.cpp \
   src/workload/tpce/egen/CustomerTable.cpp \
   src/workload/tpce/egen/InputFlatFilesStructure.cpp \
   src/workload/tpce/egen/Person.cpp \
   src/workload/tpce/egen/ReadRowFunctions.cpp \
   src/workload/tpce/egen/TradeGen.cpp \
   src/workload/tpce/egen/FlatFileLoader.cpp \
   src/workload/tpce/egen/CE.cpp \
   src/workload/tpce/egen/CETxnInputGenerator.cpp \
   src/workload/tpce/egen/CETxnMixGenerator.cpp \
   src/workload/tpce/egen/DM.cpp \
   src/workload/tpce/egen/EGenGenerateAndLoad.cpp \
   src/workload/tpce/egen/EGenValidate.cpp \
   src/workload/tpce/egen/strutil.cpp \
   src/workload/tpce/egen/progressmeter.cpp \
   src/workload/tpce/egen/progressmeterinterface.cpp \
   src/workload/tpce/egen/bucketsimulator.cpp


lib_libworkload_a_SOURCES = \
   $(WL_SG_TPCC_SHORE) \
   $(WL_TPCC_SHORE) \
   $(WL_TM1_SHORE) \
   $(WL_TPCB_SHORE) \
   $(WL_TPCH_DBGEN_SHORE) \
   $(WL_TPCH_SHORE) \
   $(WL_TPCE_SHORE_EGEN) \
   $(WL_TPCE_SHORE) \
   $(WL_TPCE_SHORE_XCTS)


if USE_QPIPE
lib_libworkload_a_SOURCES += $(WL_TPCH_QPIPE)
lib_libworkload_a_SOURCES += $(WL_SSB_QPIPE) $(WL_SSB_DBGEN_SHORE) $(WL_SSB_SHORE)
endif


lib_libworkload_a_INCLUDES = $(AM_CPPFLAGS) -I$(top_srcdir)/include/workload $(SHORE_INCLUDES)




##### USE_QPIPE ######
if USE_QPIPE

################################################################################
#
# src/util/ *** Needed by QPipe ***
#
################################################################################

QPIPE_UTIL = \
	src/util/busy_delay.cpp \
	src/util/rio.cpp \
	src/util/resource_pool.cpp \
	src/util/static_list_node.cpp \
	src/util/static_list.cpp \
	src/util/fileops.cpp \
	src/util/tmpfile.cpp \
	src/util/static_hash_map.cpp \
	src/util/fnv.cpp


lib_libqpipeutil_a_CXXFLAGS = $(AM_CXXFLAGS) 

lib_libqpipeutil_a_SOURCES = \
	$(QPIPE_UTIL)




################################################################################
#
# src/qpipe
#
################################################################################

QPIPE_SCHEDULER = \
   src/qpipe/scheduler/cpu_set.cpp \
   src/qpipe/scheduler/cpu.cpp

QPIPE_CORE = \
   src/qpipe/core/tuple_fifo_directory.cpp \
   src/qpipe/core/stage_container.cpp \
   src/qpipe/core/dispatcher.cpp \
   src/qpipe/core/packet.cpp \
   src/qpipe/core/tuple.cpp \
   src/qpipe/core/tuple_fifo.cpp

QPIPE_STAGES = \
   src/qpipe/stages/merge.cpp \
   src/qpipe/stages/bnl_in.cpp \
   src/qpipe/stages/hash_join.cpp \
   src/qpipe/stages/partial_aggregate.cpp \
   src/qpipe/stages/tscan.cpp \
   src/qpipe/stages/fdump.cpp \
   src/qpipe/stages/sieve.cpp \
   src/qpipe/stages/echo.cpp \
   src/qpipe/stages/pipe_hash_join.cpp \
   src/qpipe/stages/aggregate.cpp \
   src/qpipe/stages/sorted_in.cpp \
   src/qpipe/stages/func_call.cpp \
   src/qpipe/stages/sort.cpp \
   src/qpipe/stages/hash_aggregate.cpp \
   src/qpipe/stages/delay_writer.cpp \
   src/qpipe/stages/bnl_join.cpp \
   src/qpipe/stages/sort_merge_join.cpp \
   src/qpipe/stages/fscan.cpp \
   src/qpipe/stages/register_stage_containers.cpp

QPIPE_COMMON = \
   src/qpipe/common/process_query.cpp \
   src/qpipe/common/predicates.cpp

lib_libqpipe_a_SOURCES = \
   $(QPIPE_SCHEDULER) \
   $(QPIPE_CORE) \
   $(QPIPE_STAGES) \
   $(QPIPE_COMMON)

lib_libqpipe_a_INCLUDES = $(AM_CPPFLAGS) -I$(top_srcdir)/include/qpipe $(SHORE_INCLUDES)

endif
##### EOF USE_QPIPE ######



##### USE_DORA ######

################################################################################
#
# src/dora/
#
################################################################################

DO_BASE = \
   src/dora/dkey_ranges_map.cpp \
   src/dora/base_action.cpp \
   src/dora/rvp.cpp \
   src/dora/logical_lock.cpp \
   src/dora/base_partition.cpp \
   src/dora/partition.cpp \
   src/dora/dflusher.cpp \
   src/dora/worker.cpp \
   src/dora/part_table.cpp \
   src/dora/range_part_table.cpp \
   src/dora/dora_env.cpp

lib_libdora_a_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)

lib_libdora_a_SOURCES = \
   $(DO_BASE)



################################################################################
#
# src/dora/{workload}/
#
################################################################################

DW_TPCC = \
   src/dora/tpcc/dora_payment.cpp \
   src/dora/tpcc/dora_new_order.cpp \
   src/dora/tpcc/dora_order_status.cpp \
   src/dora/tpcc/dora_stock_level.cpp \
   src/dora/tpcc/dora_delivery.cpp \
   src/dora/tpcc/dora_mbench.cpp \
   src/dora/tpcc/dora_tpcc.cpp \
   src/dora/tpcc/dora_tpcc_xct.cpp \
   src/dora/tpcc/dora_tpcc_client.cpp

DW_TM1 = \
   src/dora/tm1/dora_tm1_impl.cpp \
   src/dora/tm1/dora_tm1.cpp \
   src/dora/tm1/dora_tm1_xct.cpp \
   src/dora/tm1/dora_tm1_client.cpp

DW_TPCB = \
   src/dora/tpcb/dora_tpcb_impl.cpp \
   src/dora/tpcb/dora_tpcb.cpp \
   src/dora/tpcb/dora_tpcb_xct.cpp \
   src/dora/tpcb/dora_tpcb_client.cpp

lib_libdoraworkload_a_SOURCES = \
   $(DW_TPCC) \
   $(DW_TM1) \
   $(DW_TPCB)


lib_libdoraworkload_a_INCLUDES = $(AM_CPPFLAGS) -I$(top_srcdir)/include/dora $(SHORE_INCLUDES)


##### EOF USE_DORA ######





################################################################################
#
# shore_kits exec
#
################################################################################

shore_kits_SOURCES = src/tests/shore_kits.cpp
shore_kits_CXXFLAGS = $(AM_CXXFLAGS) $(SHORE_INCLUDES)
if SPARC_MACHINE
shore_kits_LDADD = $(LDADD)
else
shore_kits_LDADD = $(LDADD) -ldl -lm -lpthread -lrt -lncurses
endif

debug_%.so: debug_%.cpp
	$(CXXCOMPILE) -g -shared -fPIC -o $@ $<
