# -*- makefile -*-

INCLUDES = -I$(top_srcdir)/include

TESTCOMMON = \
   drive_stage.cpp \
   int_tuple_writer.cpp \
   copy_string.cpp \
   q6_packet.cpp \
   q6_client.cpp

# encapsulates all basic classes needed in qpipe
lib_LIBRARIES = libtestcommon.a

libtestcommon_a_SOURCES = \
   $(TESTCOMMON)
