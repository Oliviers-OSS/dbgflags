AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CPPFLAGS=-DLOGGER=dynamicLogger

noinst_PROGRAMS = dynamic_test_confXML
dynamic_test_confXML_SOURCES = main.cpp
dynamic_test_confXML_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
dynamic_test_confXML_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
