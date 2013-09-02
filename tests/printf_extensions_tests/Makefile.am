AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
#AM_CPPFLAGS=-DLOGGER=syslogex

noinst_PROGRAMS = printf_extensions_tests
printf_extensions_tests_SOURCES = main.cpp
printf_extensions_tests_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
printf_extensions_tests_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
