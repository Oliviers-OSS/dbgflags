man1_MANS = zo.1 logproc.1
man3_MANS = syslogex.3 consoleLogger.3 fileLogger.3 threadFileLogger.3 ctxLogger.3
syslogex_MAN_CONTENT = openlogex closelogex setlogmaskex vsyslogex avsyslogex asyslogex syslogproc
consoleLogger_MAN_CONTENT = openConsoleLogger setConsoleLoggerOpt vconsoleLogger
fileLogger_MAN_CONTENT = openLogFile closeLogFile vfileLogger setFileLoggerMaxSize setFileLoggerMaxDuration setFileLoggerDirectory
threadFileLogger_MAN_CONTENT = openLogThreadFile closeLogThreadFile vthreadFileLogger setThreadFileLoggerMaxSize setThreadFileLoggerMaxDuration setThreadFileLoggerDirectory
ctxLogger_MAN_CONTENT = ctxLogInit vctxLogger ctxLogUnInit ctxLog dumpCtxLog
registerDebugFlags_MAN_CONTENT = registerDebugFlags registerLibraryDebugFlags unregisterDebugFlags
#EXTRA_DIST = $(man_MANS)
# MAINTAINERCLEANFILES = $(man_MANS)

install-data-hook:
	cd ${prefix}/man/man3 && gzip -f -9 $(man3_MANS) && \
	for f in ${syslogex_MAN_CONTENT} ; do $(LN_S) -f -v syslogex.3.gz $$f.3.gz ; done && \
	for f in ${consoleLogger_MAN_CONTENT} ; do $(LN_S) -f -v consoleLogger.3.gz $$f.3.gz ; done && \
	for f in ${fileLogger_MAN_CONTENT} ; do $(LN_S) -f -v fileLogger.3.gz $$f.3.gz ; done && \
	for f in ${threadFileLogger_MAN_CONTENT} ; do $(LN_S) -f -v threadFileLogger.3.gz $$f.3.gz ; done && \
	for f in ${ctxLogger_MAN_CONTENT} ; do $(LN_S) -f -v ctxLogger.3.gz $$f.3.gz ; done && \
	for f in ${registerDebugFlags_MAN_CONTENT} ; do $(LN_S) -f -v registerDebugFlags.3.gz $$f.3.gz ; done
	cd ${prefix}/man/man1 && gzip -f -9 $(man1_MANS)

#install-exec-hook:
# $(LN_S)