# -*- makefile -*-

SUBDIRS = common

INCLUDES = -I$(top_srcdir)/include

LDADD = \
   common/libtestcommon.a \
   ../workload/libworkload.a \
   ../trace/libtrace.a  \
   ../engine/libqpipe.a \
   $(BDB_HOME)/lib/libdb_cxx.a

# define programs here
bin_PROGRAMS = \
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
   test_q6_2cl \
   test_q6_disp \
   test_q13 \
   test_q14 \
   test_q12 \
   test_q16 \
   test_q19 \
   test_q1 \
   test_q4 \
   test_bitvector \
   test_hash_map

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
test_q14_SOURCES = test_q14.cpp
test_q12_SOURCES = test_q12.cpp
test_q16_SOURCES = test_q16.cpp
test_q19_SOURCES = test_q19.cpp
test_q6_2cl_SOURCES = test_q6_2cl.cpp
test_q6_disp_SOURCES = test_q6_disp.cpp
test_q1_SOURCES = test_q1.cpp
test_hash_map_SOURCES = test_hash_map.cpp
