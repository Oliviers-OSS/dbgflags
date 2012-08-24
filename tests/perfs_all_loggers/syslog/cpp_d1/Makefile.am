AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=syslog

noinst_PROGRAMS = test_perf_cpp_dbgflags_1_syslog
test_perf_cpp_dbgflags_1_syslog_SOURCES = main.cpp
test_perf_cpp_dbgflags_1_syslog_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags/ -lpthread
test_perf_cpp_dbgflags_1_syslog_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
