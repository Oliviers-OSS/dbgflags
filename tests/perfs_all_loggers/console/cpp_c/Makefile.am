AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=consoleLogger

noinst_PROGRAMS = test_perf_cpp_c_console
test_perf_cpp_c_console_SOURCES = main.cpp
test_perf_cpp_c_console_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags/ -lpthread
test_perf_cpp_c_console_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
