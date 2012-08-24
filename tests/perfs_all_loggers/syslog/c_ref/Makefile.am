AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) 
AM_CFLAGS=-DLOGGER=syslog

noinst_PROGRAMS = test_perf_ref_syslog
test_perf_ref_syslog_SOURCES = main.c
test_perf_ref_syslog_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
test_perf_ref_syslog_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
