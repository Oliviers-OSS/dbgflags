AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=syslogex

noinst_PROGRAMS = test_perf_cpp_syslogex
test_perf_cpp_syslogex_SOURCES = main.cpp
test_perf_cpp_syslogex_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
test_perf_cpp_syslogex_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
