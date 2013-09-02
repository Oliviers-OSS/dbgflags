AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=syslog

noinst_PROGRAMS = perfcpp_dbgflags_1_c_syslog
perfcpp_dbgflags_1_c_syslog_SOURCES = main.cpp
perfcpp_dbgflags_1_c_syslog_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
perfcpp_dbgflags_1_c_syslog_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
