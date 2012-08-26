AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CPPFLAGS=-DLOGGER=syslogex

noinst_PROGRAMS = basic_test
basic_test_SOURCES = main.cpp
basic_test_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
basic_test_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
