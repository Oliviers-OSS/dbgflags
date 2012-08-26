AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=syslogex

noinst_PROGRAMS = test_perf_cpp_calls_dynamic
test_perf_cpp_calls_dynamic_SOURCES = main.cpp
test_perf_cpp_calls_dynamic_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
test_perf_cpp_calls_dynamic_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
