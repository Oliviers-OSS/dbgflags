AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) 
AM_CFLAGS=-DLOGGER=fileLogger

#EXTRA_PROGRAMS = perfc1
noinst_PROGRAMS = test_perf_c_file
test_perf_c_file_SOURCES = main.c
test_perf_c_file_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags/ -lpthread
test_perf_c_file_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
