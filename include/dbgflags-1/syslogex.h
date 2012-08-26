#ifndef _SYS_SYSLOG_EX_H
#define _SYS_SYSLOG_EX_H 1

#include <stdio.h>
#include <stdarg.h>
#include <sys/syslog.h>

#ifdef __cplusplus
extern "C"
{
#endif

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

#define LOG_TID    0x040  /* log the TID (Thread ID) with each message                             */
#define LOG_RDTSC  0x080  /* log the number of clock cycles since the CPU was powered up or reset  */
#define LOG_LEVEL  0x100  /* log the msg level                                                     */
#define LOG_CLOCK  0x1000  /* log a time that represents monotonic time since some unspecified starting point */

void openlogex (const char *ident, int logstat, int logfac) NO_PROFILING;
int  setlogmaskex (int pmask) NO_PROFILING;
void syslogex(int pri, const char *fmt, ...) CHECK_PARAMS(2,3) CHECK_NON_NULL_PTR(2) NO_PROFILING;
void vsyslogex(int pri, const char *fmt, va_list ap) CHECK_PARAMS(2,0) CHECK_NON_NULL_PTR(2) NO_PROFILING;
void asyslogex(int pri, const char *fmt, ...) CHECK_PARAMS(2,3) CHECK_NON_NULL_PTR(2) NO_PROFILING;
void avsyslogex(int pri, const char *fmt, va_list ap) CHECK_PARAMS(2,0) CHECK_NON_NULL_PTR(2) NO_PROFILING;
void closelogex (void) NO_PROFILING;
int syslogproc(const char *cmd,char *argv[],const int option,const int facility,int stdOutLogLevel,int stdErrLogLevel);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_SYSLOG_EX_H */
