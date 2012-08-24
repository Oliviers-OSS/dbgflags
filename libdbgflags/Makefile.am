AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_builddir)/include -I$(top_srcdir) -I$(top_builddir)/../include
lib_LTLIBRARIES = libdbgflags.la
AM_CFLAGS = -D_GNU_SOURCE

library_includedir = $(includedir)/dbgflags-${LIBDBGFLAGS_VERSION}
libdbgflags_include_path = $(top_srcdir)/include/dbgflags-${LIBDBGFLAGS_VERSION}
library_include_HEADERS = $(libdbgflags_include_path)/dbgflags.h $(libdbgflags_include_path)/syslogex.h $(libdbgflags_include_path)/debug_macros.h $(libdbgflags_include_path)/loggers.h $(libdbgflags_include_path)/ctxLogger.h $(libdbgflags_include_path)/loggers_streams
#libdbgflags_ladir = $(top_builddir)/include
#libdbgflags_la_HEADERS = buffers.h debug.h LibrariesNameBuffer.h LibraryDebugFlagsEntry.h networkUtils.h ProcDebugFlagsUDSEntry.h protocol.h UdsManagement.h utils.h system.h ModuleVersionInfo.h
noinst_HEADERS = buffers.h debug.h LibrariesNameBuffer.h LibraryDebugFlagsEntry.h networkUtils.h ProcDebugFlagsUDSEntry.h protocol.h UdsManagement.h UdsServerManagement.h utils.h system.h ModuleVersionInfo.h RemoteStatus.h
libdbgflags_la_SOURCES = dbgFlagsUDS.c utils.c syslogex.c consoleLogger.c fileLogger.c ctxLogger.c $(noinst_HEADERS)

libdbgflags_la_LDFLAGS = -version-info ${LIBDBGFLAGS_VERSION}:${LIBDBGFLAGS_REVISION}:${LIBDBGFLAGS_INTERFACE_AGE}

