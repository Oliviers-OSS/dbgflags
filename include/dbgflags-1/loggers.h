#ifndef _LOGGERS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdarg.h>
#include <dbgflags/syslogex.h>

#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define CHECK_PARAMS(f,p)		__attribute__ ((format (printf, f, p)))
#ifndef PROFILING_HOOKS_ENABLED
#define NO_PROFILING __attribute__ ((no_instrument_function))
#else 
#define NO_PROFILING
#endif /*PROFILING_HOOKS_ENABLED*/
#if (GCC_VERSION > 40000) /* GCC 4.0.0 */
#define CHECK_NON_NULL_PTR(n)	__attribute__ ((nonnull(n)))
#else
#define CHECK_NON_NULL_PTR(n)
#endif /* (GCC_VERSION > 40000) */
#else 
#define CHECK_PARAMS(f,p)
#define CHECK_NON_NULL_PTR(n)
#endif /* __GNUC__ */

void setConsoleLoggerOpt(const char *ident, int logstat, int logfac) NO_PROFILING;
void consoleLogger(int priority, const char *format, ...) CHECK_PARAMS(2,3) CHECK_NON_NULL_PTR(2) NO_PROFILING;

#define LOG_FILE_ROTATE     0x200
#define LOG_FILE_HISTO      0x400
void openLogFile(const char *ident, int logstat, int logfac) NO_PROFILING;
void fileLogger(int priority, const char *format, ...) CHECK_PARAMS(2,3) CHECK_NON_NULL_PTR(2) NO_PROFILING;
int setFileLoggerMaxSize(unsigned int maxSizeInBytes) NO_PROFILING;
int setFileLoggerDirectory(const char *directory) CHECK_NON_NULL_PTR(1) NO_PROFILING;
void closeLogFile(void) NO_PROFILING;

#ifdef __cplusplus
}
#endif

#endif /* _LOGGERS_H_ */
