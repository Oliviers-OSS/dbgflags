PROGRAM = logproc
VERSION = 1.0
AUTOMAKE_OPTIONS = gnu

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) -I$(top_builddir)/../include -I../libdbgflags
#AM_CFLAGS=-DLOGGER=consoleLogger
AM_CFLAGS = -D_GNU_SOURCE -DPROGRAM_NAME=$(PROGRAM) -DPROGRAM_VERSION=$(VERSION)

bin_PROGRAMS = $(PROGRAM)
logproc_SOURCES = main.c 
#include_HEADERS = ModuleVersionInfo.h
logproc_LDFLAGS = -ldbgflags -lpthread -lrt -L../libdbgflags/
logproc_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
