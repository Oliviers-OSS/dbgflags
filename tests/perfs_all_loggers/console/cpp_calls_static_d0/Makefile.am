AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=consoleLogger

noinst_PROGRAMS = test_perf_cpp_calls_static_d0
test_perf_cpp_calls_static_d0_SOURCES = main.cpp
test_perf_cpp_calls_static_d0_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
test_perf_cpp_calls_static_d0_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
