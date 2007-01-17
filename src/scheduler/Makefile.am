INCLUDES = -I$(top_srcdir)/include

lib_LIBRARIES = libscheduler.a
libscheduler_a_SOURCES = \
	cpu.cpp \
	cpu_set.cpp

test_prefetch_LDADD = libscheduler.a $(top_srcdir)/src/util/libutil.a
bin_PROGRAMS = test_prefetch
test_prefetch_SOURCES = test_prefetch.cpp