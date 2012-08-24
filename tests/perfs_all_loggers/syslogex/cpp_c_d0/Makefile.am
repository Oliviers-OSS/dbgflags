AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=syslogex

noinst_PROGRAMS = perfcpp_dbgflags_0_c_syslogex
perfcpp_dbgflags_0_c_syslogex_SOURCES = main.cpp
perfcpp_dbgflags_0_c_syslogex_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
perfcpp_dbgflags_0_c_syslogex_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
