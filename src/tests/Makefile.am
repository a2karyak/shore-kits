# -*- makefile -*-

SUBDIRS = common

INCLUDES = -I$(top_srcdir)/include

LDBASE = $(top_srcdir)/src

LDADD = \
	common/libtestcommon.a \
	$(LDBASE)/workload/libworkload.a \
	$(LDBASE)/scheduler/libscheduler.a \
	$(LDBASE)/stages/libstages.a \
	$(LDBASE)/core/libcore.a \
	$(LDBASE)/util/libutil.a \
	$(BDB_HOME)/lib/libdb_cxx.a \
	$(LDBASE)/bdb-ext/libbdbext.a \
	$(LDBASE)/util/libutil.a 


# define programs here
bin_PROGRAMS = \
   test_odirect \
   test_cpu_binding \
   test_rand_array_access \
   load_db_tables \
   test_trace \
   test_tuple_buffer \
   test_tpch_load \
   tpch_struct_sizes \
   test_fscan_stage \
   test_fdump_stage \
   test_tscan_stage \
   test_func_call_stage \
   test_aggregate_stage \
   test_merge_stage \
   test_sort_stage \
   test_hash_join_stage \
   test_bnl_join_stage \
   test_bnl_in_stage \
   test_q4 \
   test_q6 \
   test_q6_idx \
   test_q6_disp \
   test_q13 \
   test_q13_pagg \
   test_q14 \
   test_q12 \
   test_q16 \
   test_q19 \
   test_q1 \
   test_q4 \
   test_bitvector \
   test_hash_map \
   test_mpool \
   grouper

test_odirect_SOURCES = test_odirect.cpp
test_cpu_binding_SOURCES = test_cpu_binding.cpp
test_rand_array_access_SOURCES = test_rand_array_access.cpp
load_db_tables_SOURCES = load_db_tables.cpp
test_trace_SOURCES = test_trace.cpp
test_tuple_buffer_SOURCES = test_tuple_buffer.cpp
test_bitvector_SOURCES = test_bitvector.cpp
test_tpch_load_SOURCES = test_tpch_load.cpp
test_fdump_stage_SOURCES = test_fdump_stage.cpp
test_merge_stage_SOURCES = test_merge_stage.cpp
test_sort_stage_SOURCES = test_sort_stage.cpp
test_fscan_stage_SOURCES = test_fscan_stage.cpp
test_tscan_stage_SOURCES = test_tscan_stage.cpp
test_func_call_stage_SOURCES = test_func_call_stage.cpp
test_aggregate_stage_SOURCES = test_aggregate_stage.cpp
test_hash_join_stage_SOURCES = test_hash_join_stage.cpp
test_bnl_join_stage_SOURCES = test_bnl_join_stage.cpp
test_bnl_in_stage_SOURCES = test_bnl_in_stage.cpp
tpch_struct_sizes_SOURCES = tpch_struct_sizes.cpp
test_q4_SOURCES = test_q4.cpp
test_q6_SOURCES = test_q6.cpp
test_q6_idx_SOURCES = test_q6_idx.cpp
test_q13_SOURCES = test_q13.cpp
test_q13_pagg_SOURCES = test_q13_pagg.cpp
test_q14_SOURCES = test_q14.cpp
test_q12_SOURCES = test_q12.cpp
test_q16_SOURCES = test_q16.cpp
test_q19_SOURCES = test_q19.cpp
test_q6_disp_SOURCES = test_q6_disp.cpp
test_q1_SOURCES = test_q1.cpp
test_hash_map_SOURCES = test_hash_map.cpp
test_mpool_SOURCES = test_mpool.cpp
grouper_SOURCES = grouper.cpp
