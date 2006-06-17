# -*- makefile -*-

INCLUDES = -I$(top_srcdir)/include


TESTCOMMON = \
   drive_stage.cpp \
   int_tuple_writer.cpp \
   copy_string.cpp \
   tpch_compare.cpp \
   tpch_type_convert.cpp

# encapsulates all basic classes needed in qpipe
lib_LIBRARIES = libtestcommon.a

libtestcommon_a_SOURCES = \
   $(TESTCOMMON)
