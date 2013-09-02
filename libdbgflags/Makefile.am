AUTOMAKE_OPTIONS = foreign
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir) -I$(top_srcdir)/../include
lib_LTLIBRARIES = libdbgflags.la
AM_CFLAGS = -D_GNU_SOURCE

library_includedir = $(includedir)/dbgflags-${LIBDBGFLAGS_VERSION}
libdbgflags_include_path = $(top_srcdir)/include/dbgflags-${LIBDBGFLAGS_VERSION}
libdbgflags_ladir = $(library_includedir)
libdbgflags_la_HEADERS = $(libdbgflags_include_path)/dbgflags.h $(libdbgflags_include_path)/syslogex.h $(libdbgflags_include_path)/debug_macros.h $(libdbgflags_include_path)/loggers.h $(libdbgflags_include_path)/ctxLogger.h $(libdbgflags_include_path)/loggers_streams $(libdbgflags_include_path)/goodies.h $(libdbgflags_include_path)/ModuleVersionInfo.h $(libdbgflags_include_path)/version.h
# tmp
#library_include_HEADERS += dbgflags/dbgflags.h dbgflags/syslogex.h dbgflags/debug_macros.h dbgflags/loggers.h dbgflags/ctxLogger.h dbgflags/loggers_streams dbgflags/goodies.h dbgflags/ModuleVersionInfo.h dbgflags/version.h
#libdbgflags_ladir = $(top_builddir)/include
#libdbgflags_la_HEADERS = buffers.h debug.h LibrariesNameBuffer.h LibraryDebugFlagsEntry.h networkUtils.h ProcDebugFlagsUDSEntry.h protocol.h UdsManagement.h utils.h system.h ModuleVersionInfo.h
noinst_HEADERS = buffers.h debug.h LibrariesNameBuffer.h LibraryDebugFlagsEntry.h networkUtils.h ProcDebugFlagsUDSEntry.h protocol.h UdsManagement.h UdsServerManagement.h utils.h system.h RemoteStatus.h filesUtils.h dynamicLogger.h configurationFile.h configurationFileData.h setFullFileName.h
libdbgflags_la_SOURCES = dbgFlagsUDS.c utils.c syslogex.c consoleLogger.c fileLogger.c ctxLogger.c goodies.c dynamicLogger.c threadFileLogger.c printf_hook.c 
libdbgflags_la_SOURCES += $(noinst_HEADERS) libdbgflags.map

libdbgflags_la_LDFLAGS = -version-info ${LIBDBGFLAGS_VERSION}:${LIBDBGFLAGS_REVISION}:${LIBDBGFLAGS_INTERFACE_AGE}
libdbgflags_la_LDFLAGS += -Wl,--version-script=$(top_srcdir)/libdbgflags/libdbgflags.map

install-exec-hook:
	test -L $(includedir)/dbgflags && /bin/rm $(includedir)/dbgflags &>/dev/null || true
	mkdir -p $(includedir)/dbgflags-${LIBDBGFLAGS_VERSION}
	$(LN_S) $(includedir)/dbgflags-${LIBDBGFLAGS_VERSION} $(includedir)/dbgflags
