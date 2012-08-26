/* 
 * File:   dynamicLogger.h
 * Author: oc
 *
 * Created on August 1, 2011, 3:06 AM
 */

#ifndef _DYNAMIC_LOGGER_H_
#define	_DYNAMIC_LOGGER_H_

#include <stdarg.h>
#include <pthread.h>
#include <dbgflags/loggers.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef void (*openLogFctPtr) (const char *ident, int logstat, int logfac);
typedef void (*loggerFctPtr)(int pri, const char *fmt, ...);
typedef void (*vLoggerFctPtr)(int pri, const char *fmt, va_list ap);
typedef int  (*setLogMaskFctPtr) (int pmask);
typedef void (*closeLogFctPtr) (void);

typedef struct dynamicLoggerData_ {
    const dynamicLoggerConfiguration *configuration;
    openLogFctPtr open;
    loggerFctPtr logger;
    vLoggerFctPtr vlogger;
    setLogMaskFctPtr setMask;
    closeLogFctPtr close;
#ifdef _GNU_SOURCE
    pthread_rwlock_t lock;
#else /*  _GNU_SOURCE */
    pthread_mutex_t lock;
#endif /* _GNU_SOURCE */
} dynamicLoggerData;

typedef struct LoggersDirectory_ {
    const char *Name;
    openLogFctPtr Open;
    loggerFctPtr logger;
    vLoggerFctPtr vlogger;
    setLogMaskFctPtr setMask;
    closeLogFctPtr Close;
} LoggersDirectory;

#ifndef TO_STRING
#define STRING(x) #x
#define TO_STRING(x) STRING(x)
#endif

#define DECLARE_LOGGER(open,logger,vlogger,setMask,close)  { TO_STRING(logger),open,logger,vlogger,setMask,close},

#ifdef	__cplusplus
}
#endif

#endif	/* _DYNAMIC_LOGGER_H_ */

