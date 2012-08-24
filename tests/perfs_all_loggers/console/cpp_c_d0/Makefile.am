AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=consoleLogger

noinst_PROGRAMS = perfcpp_dbgflags_0_c_console
perfcpp_dbgflags_0_c_console_SOURCES = main.cpp
perfcpp_dbgflags_0_c_console_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
perfcpp_dbgflags_0_c_console_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
