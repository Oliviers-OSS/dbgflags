AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CFLAGS=-DLOGGER=syslogex

noinst_PROGRAMS = test_perf_c_dbgflags_on_syslogex
test_perf_c_dbgflags_on_syslogex_SOURCES = main.c
test_perf_c_dbgflags_on_syslogex_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
test_perf_c_dbgflags_on_syslogex_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
