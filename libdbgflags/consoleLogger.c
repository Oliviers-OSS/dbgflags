#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <dbgflags/syslogex.h>
#include <dbgflags/loggers.h>
#include "system.h"


static int	LogStat = 0;		/* status bits, set by openlog() */
static const char *LogTag = NULL;		/* string to tag the entry with */
static int	LogFacility = LOG_USER;	/* default facility code */
static int	LogMask = 0xff;		/* mask of priorities to be logged */

void setConsoleLoggerOpt(const char *ident, int logstat, int logfac) {
    if (ident != NULL) {
		LogTag = ident;
    }
	LogStat = logstat;
    if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0) {
		LogFacility = logfac;
    }
}

void consoleLogger(int priority, const char *format, ...) {
    int n = 0;
    const int LogMask = setlogmask(0);

    /* Check priority against setlogmask values. */  
    if ((LOG_MASK (LOG_PRI (priority)) & LogMask) != 0) {
        va_list optional_arguments;
        char logFormat[1024];
        char *cursor = logFormat;
        struct tm now_tm;
        time_t now;

        (void) time(&now);
        cursor += strftime(cursor,sizeof(logFormat),"%h %e %T ",localtime_r(&now, &now_tm));

        if (LogTag) {
           cursor += sprintf (cursor,"%s: ",LogTag);
        }

        if (LogStat & LOG_PID) {
            if (LogStat & LOG_TID) {
                const pid_t tid = gettid();
                n = sprintf (cursor, "[%d:%d]", (int) getpid (),(int) tid);
            } else {
                n = sprintf (cursor, "[%d]", (int) getpid ());
            }
            cursor += n;
        }

        if (LogStat & LOG_RDTSC) {
            const unsigned long long int  t = rdtsc();
            cursor += sprintf (cursor, "(%llu)",t);
        } /* (LogStat & LOG_CLOCK) */

        va_start(optional_arguments, format);
        switch(LOG_PRI(priority)) {
        case LOG_EMERG:
            n = sprintf(cursor,"* Emergency * %s",format);
            break;
        case LOG_ALERT:
            n = sprintf(cursor,"* Alert * %s",format);
            break;
        case LOG_CRIT:
            n = sprintf(cursor,"* Critical * %s",format);
            break;
        case LOG_ERR:
            n = sprintf(cursor,"* Error * %s",format);
            break;
        case LOG_WARNING:
            n = sprintf(cursor,"* Warning * %s",format);
            break;
        case LOG_NOTICE:
            n = sprintf(cursor,"* Notice * %s",format);
            break;
        case LOG_INFO:
            n = sprintf(cursor,"* Info * %s",format);
            break;
        case LOG_DEBUG:
            n = sprintf(cursor,"* Debug * %s",format);
            break;
        default:
            n = sprintf(cursor,"* <%d> * %s",priority,format);
        } /* switch(priority) */        

        n = vfprintf(stderr,logFormat,optional_arguments);
        va_end(optional_arguments);
    }

    /*return n;*/
}

#include "ModuleVersionInfo.h"
MODULE_NAME(consoleLogger);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(0.1);
MODULE_FILE_VERSION(0.0);
MODULE_DESCRIPTION(logger to error console with the same signature than syslog);
MODULE_COPYRIGHT(LGPL);


