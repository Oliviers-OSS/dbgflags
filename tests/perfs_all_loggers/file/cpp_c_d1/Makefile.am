AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) 
AM_CXXFLAGS=-DLOGGER=fileLogger

noinst_PROGRAMS = perfcpp_dbgflags_1_c_file
perfcpp_dbgflags_1_c_file_SOURCES = main.cpp
perfcpp_dbgflags_1_c_file_LDFLAGS = -ldbgflags -L$(top_builddir)/libdbgflags -lpthread
perfcpp_dbgflags_1_c_file_DEPENDENCIES = $(top_builddir)/libdbgflags/.libs/libdbgflags.so
