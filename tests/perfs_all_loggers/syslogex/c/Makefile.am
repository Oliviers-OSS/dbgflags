AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CFLAGS=-DLOGGER=syslogex

#EXTRA_PROGRAMS = perfc1
noinst_PROGRAMS = test_perf_c_syslogex
test_perf_c_syslogex_SOURCES = main.c
test_perf_c_syslogex_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags/ -lpthread
test_perf_c_syslogex_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
