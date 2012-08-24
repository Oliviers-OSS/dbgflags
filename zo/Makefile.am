PROGRAM = zo
VERSION = 1.0
AUTOMAKE_OPTIONS = gnu

INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) -I$(top_builddir)/../include -I../libdbgflags
AM_CFLAGS = -D_GNU_SOURCE -DPROGRAM_NAME=$(PROGRAM) -DPROGRAM_VERSION=$(VERSION)

bin_PROGRAMS = $(PROGRAM)
zo_SOURCES = main.c
zo_LDFLAGS = -ldbgflags -L../libdbgflags/ -lpthread
zo_DEPENDENCIES = ../libdbgflags/.libs/libdbgflags.so
