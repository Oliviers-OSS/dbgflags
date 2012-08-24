AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=fileLogger

noinst_PROGRAMS = test_perf_cpp_streams
test_perf_cpp_streams_SOURCES = main.cpp
test_perf_cpp_streams_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
test_perf_cpp_streams_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
