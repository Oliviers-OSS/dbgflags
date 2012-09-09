AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CPPFLAGS=-DLOGGER=threadFileLogger -D_DEBUG_

noinst_PROGRAMS = threads_test
threads_test_SOURCES = main.c
threads_test_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread 
threads_test_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
