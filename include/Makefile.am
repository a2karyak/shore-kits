# The dummy program forces autoconf to define $(CXXCOMPILE), which we
# need to create the precompiled headers properly
bin_PROGRAMS = dummy
dummy_SOURCES = dummy.cpp

# This is the real point of the Makefile
BUILT_SOURCES = util_impl.h.gch core_impl.h.gch 
CLEANFILES = $(BUILT_SOURCES)

.SUFFIXES = .h .gch
UTIL_H = \
	util/c_str.h \
	util/exception.h \
	util/fnv.h \
	util/guard.h \
	util/hash_functions.h \
	util/namespace.h \
	util/static_hash_map.h \
	util/stopwatch.h \
	util/sync.h \
	util/thread.h \
	util/time.h \
	util/tmpfile.h \
	util/trace.h

CORE_H = \
        core/dispatcher.h \
        core/dispatcher_cpu.h \
        core/functors.h \
        core/packet.h \
        core/stage.h \
        core/stage_container.h \
        core/tuple.h \
        core/tuple_fifo.h

%.h.gch: %.h
	$(CXXCOMPILE) -x c++ -c -o $@ $<

util_impl.h.gch: $(UTIL_H)
core_impl.h.gch: $(CORE_H)