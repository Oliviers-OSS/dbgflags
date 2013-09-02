PROGRAM = zo
VERSION = 1.1
AUTOMAKE_OPTIONS = gnu

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) -I$(top_srcdir)/libdbgflags
AM_CFLAGS = -D_GNU_SOURCE -DPROGRAM_NAME=$(PROGRAM) -DPROGRAM_VERSION=$(VERSION)

bin_PROGRAMS = $(PROGRAM)
zo_SOURCES = main.c
zo_LDFLAGS = -ldbgflags -L../libdbgflags/ -lpthread
zo_DEPENDENCIES = ../libdbgflags/.libs/libdbgflags.so
