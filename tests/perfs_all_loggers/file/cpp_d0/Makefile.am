AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=fileLogger

noinst_PROGRAMS = test_perf_cpp_dbgflags_0_file
test_perf_cpp_dbgflags_0_file_SOURCES = main.cpp
test_perf_cpp_dbgflags_0_file_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags/ -lpthread
test_perf_cpp_dbgflags_0_file_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
