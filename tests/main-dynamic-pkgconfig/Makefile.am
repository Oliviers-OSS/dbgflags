AUTOMAKE_OPTIONS = foreign
#PKG_CONFIG_PATH=(top_builddir)
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)  `PKG_CONFIG_PATH=$(top_builddir) pkg-config --errors-to-stdout --cflags dbgflags`
AM_CPPFLAGS=-DLOGGER=dynamicLogger

noinst_PROGRAMS = dynamic_test_pkgconfig
dynamic_test_pkgconfig_SOURCES = main.cpp
dynamic_test_pkgconfig_LDFLAGS = -L$(top_builddir)/libdbgflags `PKG_CONFIG_PATH=$(top_builddir) pkg-config --errors-to-stdout --libs dbgflags`
dynamic_test_pkgconfig_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so

