AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CPPFLAGS=-DLOGGER=dynamicLogger

noinst_PROGRAMS = dynamic_test
dynamic_test_SOURCES = main.cpp
dynamic_test_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
dynamic_test_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
