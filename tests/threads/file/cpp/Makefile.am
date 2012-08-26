AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CPPFLAGS=-DLOGGER=fileLogger -D_DEBUG_

noinst_PROGRAMS = threads_test
threads_test_SOURCES = main.cpp
threads_test_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread #-lduma
threads_test_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
