AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CFLAGS=-DLOGGER=threadFileLogger

noinst_PROGRAMS = test_perf_ref_file
test_perf_ref_file_SOURCES = main.c
test_perf_ref_file_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
test_perf_ref_file_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
