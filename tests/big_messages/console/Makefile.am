AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CFLAGS=-DLOGGER=consoleLogger -D_DEBUG_

#EXTRA_PROGRAMS = perfc1
noinst_PROGRAMS = test_perf_c_console
test_perf_c_console_SOURCES = main.c
test_perf_c_console_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags/ -lpthread
test_perf_c_console_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
