AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=syslogex

noinst_PROGRAMS = test_perf_cpp_c_syslogex
test_perf_cpp_c_syslogex_SOURCES = main.cpp
test_perf_cpp_c_syslogex_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags/ -lpthread
test_perf_cpp_c_syslogex_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
