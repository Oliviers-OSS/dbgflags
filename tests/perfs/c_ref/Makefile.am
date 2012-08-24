AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) 
AM_CFLAGS=-DLOGGER=consoleLogger

noinst_PROGRAMS = test_perf_ref_console
test_perf_ref_console_SOURCES = main.c
test_perf_ref_console_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
test_perf_ref_console_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
