INCLUDES = -I$(top_srcdir)/include

.SUFFIXES = .s

bin_PROGRAMS = testctx

lib_LIBRARIES = libutil.a
libutil_a_SOURCES = \
	c_str.cpp \
	fnv.cpp \
	hash_functions.cpp \
	static_hash_map.cpp \
	thread.cpp \
	time.cpp \
	tmpfile.cpp \
	trace.cpp \
	tcp.cpp \
	rio.cpp \
	ctx.cpp \
	ctx_swap.cpp \
	chomp.cpp

testctx_SOURCES = \
	c_str.cpp \
	thread.cpp \
	ctx.cpp \
	ctx_test.cpp \
	ctx_swap.cpp 

CLEANFILES = ctx_swap.s

# compiled separately to avoid debug info (ironically, this allows
# debugging the assembly code) and ensure that it gets optimized
# (strangely, this is required for correctness)
ctx_swap.o: ctx_swap.cpp
	$(CXX) -Wall $(INCLUDES) -O2 -S $< -o ctx_swap.s
	$(CXXCOMPILE) -c ctx_swap.s -o $@
