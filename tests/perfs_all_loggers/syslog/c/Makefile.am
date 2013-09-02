AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CFLAGS=-DLOGGER=syslog

#EXTRA_PROGRAMS = perfc1
noinst_PROGRAMS = test_perf_c_syslog
test_perf_c_syslog_SOURCES = main.c
test_perf_c_syslog_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags/ -lpthread
test_perf_c_syslog_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
