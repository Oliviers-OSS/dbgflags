AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=consoleLogger

noinst_PROGRAMS = test_perf_cpp_dbgflags_1
test_perf_cpp_dbgflags_1_SOURCES = main.cpp
test_perf_cpp_dbgflags_1_LDFLAGS = -ldbgflags -L../../../libdbgflags/ -lpthread
test_perf_cpp_dbgflags_1_DEPENDENCIES = ../../../libdbgflags/.libs/libdbgflags.so
