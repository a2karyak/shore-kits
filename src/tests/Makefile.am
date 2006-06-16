# -*- makefile -*-

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/include/tests

LDADD = ../engine/libqpipe.a ../trace/libtrace.a $(BDB_HOME)/lib/libdb_cxx.a

# define programs here
bin_PROGRAMS = \
   test_trace \
   test_tuple_buffer \
   test_fscan_stage \
   test_fdump_stage \
   lineitem_struct_sizes \
   test_tscan_stage \
   test_func_call_stage \
   test_aggregate_stage \
   test_merge_stage \
   test_sort_stage \
   test_hash_join_stage \
   test_q6 \
   test_q6_2cl \
   test_q1

#\
   test_bnl_caching_join_stage



test_trace_SOURCES = test_trace.cpp
test_tuple_buffer_SOURCES = test_tuple_buffer.cpp
test_merge_stage_SOURCES = test_merge_stage.cpp
test_sort_stage_SOURCES = test_sort_stage.cpp
test_fdump_stage_SOURCES = test_fdump_stage.cpp
test_fscan_stage_SOURCES = test_fscan_stage.cpp
test_bnl_caching_join_stage_SOURCES = test_bnl_caching_join_stage.cpp
test_tscan_stage_SOURCES = test_tscan_stage.cpp
test_func_call_stage_SOURCES = test_func_call_stage.cpp
test_aggregate_stage_SOURCES = test_aggregate_stage.cpp
test_hash_join_stage_SOURCES = test_hash_join_stage.cpp
lineitem_struct_sizes_SOURCES = lineitem_struct_sizes.cpp
test_q6_SOURCES = test_q6.cpp
test_q6_2cl_SOURCES = test_q6_2cl.cpp
test_q6_mult_SOURCES = test_q6_mult.cpp
test_q1_SOURCES = test_q1.cpp

