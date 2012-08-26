#ifndef _DEBUG_MACROS_H_
#define _DEBUG_MACROS_H_

#define STRING(x) #x
#define TO_STRING(x) STRING(x)

#ifndef DEBUG_EOL
#define DEBUG_EOL "\r\n"
#endif /* DEBUG_EOL */

#ifndef DEBUG_LOG_HEADER
/*#define DEBUG_LOG_HEADER "####: "*/
#define DEBUG_LOG_HEADER ""
#endif /* DEBUG_LOG_HEADER */

#ifndef LOG_OPTS
#define LOG_OPTS  0
#endif /* LOG_OPTS*/

#ifndef LOGGER
#if defined(_SYS_SYSLOG_EX_H)
#define LOGGER syslogex
#elif defined(_SYS_SYSLOG_H)
#define LOGGER syslog
#else
#include <dbgflags/loggers.h>
#define LOGGER consoleLogger 
#endif
#endif /* LOGGER */

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

/* define Branch prediction hints macros for GCC 4.0.0 and upper */
#if (GCC_VERSION > 40000) /* GCC 4.0.0 */
#ifndef likely
#define likely(x)   __builtin_expect(!!(x),1)
#endif /* likely */
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x),0)
#endif /* unlikely */
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif /* GCC 4.0.0 */

#if (GCC_VERSION > 42100) /* GCC 4.2.1 */
#pragma GCC diagnotic push
#pragma GCC diagnostic ignored "-Wno-variadic-macros"
#else 
#pragma GCC system_header /* oops ! just to disable the nasty warning "warning: anonymous variadic macros were introduced in C99" */
#endif /* 4.2.1 */

#ifdef _DEBUGFLAGS_H_
#ifndef FILTER
#define FILTER if (unlikely(MODULE_FLAG == (debugFlags.mask &  MODULE_FLAG)))
#endif /* FILTER */
#else /* _DEBUGFLAGS_H_ */
#define FILTER
#endif /* _DEBUGFLAGS_H_ */

#define SIMPLE_DEBUG_LOG_HEADER DEBUG_LOG_HEADER ":"
#define DEBUG_LOG_HEADER_POS    " [ %s ("  __FILE__ ":%d)]:"

#include <syslog.h> /* to define log levels */
#include <dbgflags/ctxLogger.h>

#ifdef __cplusplus
#include <dbgflags/loggers_streams>

// WARNING: user program MUST allocate (at least) one global instance of one of
// the following class for each used (syslog's) log level to be able to use cpp log streams:
// just copy the extern declaration below in one of yours cpp file without the extern keywork for example.
typedef TLSInstanceOf<loggerStream<LOG_EMERG, Logger<char,LOGGER>, char> > emergencyLogger;
typedef TLSInstanceOf<loggerStream<LOG_ALERT, Logger<char,LOGGER>,char> > alertLogger;
typedef TLSInstanceOf<loggerStream<LOG_CRIT, Logger<char,LOGGER>,char> > criticalLogger;
typedef TLSInstanceOf<loggerStream<LOG_ERR, Logger<char,LOGGER>,char> > errorLogger;
typedef TLSInstanceOf<loggerStream<LOG_WARNING, Logger<char,LOGGER>,char> > warningLogger;
typedef TLSInstanceOf<loggerStream<LOG_NOTICE, Logger<char,LOGGER>,char> > noticeLogger;
typedef TLSInstanceOf<loggerStream<LOG_INFO, Logger<char,LOGGER>,char> > infoLogger;
typedef TLSInstanceOf<loggerStream<LOG_DEBUG, Logger<char,LOGGER>,char> > debugLogger;
typedef TLSInstanceOf<loggerStream<LOG_DEBUG, Logger<char,ctxLogger>,char> > contextLogger;

extern emergencyLogger    logEmergency;
extern alertLogger         logAlert;
extern criticalLogger     logCritical;
extern errorLogger         logError;
extern warningLogger      logWarning;
extern noticeLogger       logNotice;
extern infoLogger          logInfo;
extern debugLogger         logDebug;
extern contextLogger       logContext;

#define CPP_SIMPLE_DEBUG_LOG_HEADER     DEBUG_LOG_HEADER ":"
#define CPP_DEBUG_LOG_HEADER_POS        " [ " << __FUNCTION__ << " (" << __FILE__ << ":" << __LINE__ << ")]:"

#endif /* __cplusplus */

/*
 * DEBUG MACROS PER LEVELS
 */

/* full system (not module) will become unusable (avoid to use) */
#define EMERG_MSG(fmt,...)   FILTER LOGGER(LOG_EMERG|LOG_OPTS,DEBUG_LOG_HEADER_POS fmt DEBUG_EOL,__FUNCTION__,__LINE__, ##__VA_ARGS__)

/* action must be taken immediately */
#define ALERT_MSG(fmt,...)   FILTER LOGGER(LOG_ALERT|LOG_OPTS,DEBUG_LOG_HEADER_POS fmt DEBUG_EOL,__FUNCTION__,__LINE__, ##__VA_ARGS__)

/* critical conditions */
#define CRIT_MSG(fmt,...)    FILTER LOGGER(LOG_CRIT|LOG_OPTS,DEBUG_LOG_HEADER_POS fmt DEBUG_EOL,__FUNCTION__,__LINE__, ##__VA_ARGS__)

#ifdef __cplusplus
#define EMERG_STREAM FILTER logEmergency.getInstance() << CPP_DEBUG_LOG_HEADER_POS
#define ALERT_STREAM FILTER logAlert.getInstance() << CPP_DEBUG_LOG_HEADER_POS
#define CRITICAL_STREAM  FILTER logCritical.getInstance() << CPP_DEBUG_LOG_HEADER_POS
#endif /* __cplusplus */

#ifndef _RETAIL_

/* error conditions */
#define ERROR_MSG(fmt,...)   FILTER LOGGER(LOG_ERR|LOG_OPTS,DEBUG_LOG_HEADER_POS fmt DEBUG_EOL,__FUNCTION__,__LINE__, ##__VA_ARGS__)

/* warning conditions */
#define WARNING_MSG(fmt,...) FILTER LOGGER(LOG_WARNING|LOG_OPTS,SIMPLE_DEBUG_LOG_HEADER fmt DEBUG_EOL, ##__VA_ARGS__)

/* normal, but significant, condition */
#define NOTICE_MSG(fmt,...)  FILTER LOGGER(LOG_NOTICE|LOG_OPTS,SIMPLE_DEBUG_LOG_HEADER fmt DEBUG_EOL, ##__VA_ARGS__)

#ifdef __cplusplus
#define ERROR_STREAM              FILTER logError.getInstance() << CPP_DEBUG_LOG_HEADER_POS
#define WARNING_STREAM            FILTER logWarning.getInstance()  << CPP_SIMPLE_DEBUG_LOG_HEADER
#define NOTICE_STREAM             FILTER  logNotice.getInstance()  << CPP_SIMPLE_DEBUG_LOG_HEADER
#endif /* __cplusplus */

#else /* _RETAIL_ */

#define ERROR_MSG(fmt,...)
#define WARNING_MSG(fmt,...)
#define NOTICE_MSG(fmt,...)

#ifdef __cplusplus
#define ERROR_STREAM      if (0) logError.getInstance()
#define WARNING_STREAM    if (0) logWarning.getInstance()
#define NOTICE_STREAM     if (0) logNotice.getInstance()
#endif /* __cplusplus */

#endif /* _RETAIL_ */

#ifdef _DEBUG_
/* informational message */
#define INFO_MSG(fmt,...)    FILTER LOGGER(LOG_INFO|LOG_OPTS,SIMPLE_DEBUG_LOG_HEADER fmt DEBUG_EOL, ##__VA_ARGS__)

/* debug-level message */
#define DEBUG_MSG(fmt,...)    FILTER LOGGER(LOG_DEBUG|LOG_OPTS,SIMPLE_DEBUG_LOG_HEADER fmt DEBUG_EOL, ##__VA_ARGS__)
#define DEBUG_POSMSG(fmt,...) FILTER LOGGER(LOG_DEBUG|LOG_OPTS,DEBUG_LOG_HEADER_POS fmt DEBUG_EOL,__FUNCTION__,__LINE__, ##__VA_ARGS__)
/*#define DEBUG_FUNC           FILTER LOGGER(LOG_DEBUG|LOG_OPTS, SIMPLE_DEBUG_LOG_HEADER " ("__FUNCTION__  __FILE__ ": %d)" DEBUG_EOL,__LINE__)*/
#define DEBUG_MARK            FILTER LOGGER(LOG_DEBUG|LOG_OPTS, SIMPLE_DEBUG_LOG_HEADER __FILE__ "(%d) in %s" DEBUG_EOL,__LINE__,__FUNCTION__)
#define DEBUG_VAR(x,f)        FILTER LOGGER(LOG_DEBUG|LOG_OPTS, SIMPLE_DEBUG_LOG_HEADER __FILE__ "(%d) %s: " #x " = " f DEBUG_EOL,__LINE__,__FUNCTION__,x)
#define DEBUG_VAR_BOOL(Var)   FILTER LOGGER(LOG_DEBUG|LOG_OPTS, SIMPLE_DEBUG_LOG_HEADER __FILE__ "(%d) %s: " #Var " = %s" DEBUG_EOL,__LINE__,__FUNCTION__,(Var?"True":"False"))

#include <stdio.h>

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */

static __inline void dumpMemory(const char *memoryName,const void *address, unsigned int size) {
    const unsigned int nbBytesPerLines = 16;
    char hexa[(nbBytesPerLines+1) * 3];
    char ascii[(nbBytesPerLines+1)];
    register const unsigned char *cursor = (unsigned char *)address;
    const unsigned char *limit = cursor + size;

    LOGGER(LOG_DEBUG|LOG_OPTS,SIMPLE_DEBUG_LOG_HEADER " *** begin of memory dump of %s (size = %d bytes) ***" DEBUG_EOL,memoryName,size);
    while(cursor < limit) {
        register unsigned int i;
        register char *hexaCursor = hexa;
        register char *asciiCursor = ascii;
        const unsigned int remaining = limit-cursor;
        const unsigned int lineSize = MIN(nbBytesPerLines,remaining);

        for(i=0;i<lineSize;i++) {            
            hexaCursor += sprintf(hexaCursor,"%.2X ",*cursor);            
            if ((*cursor >= 0x20) && (*cursor<= 0x7A)) {
                asciiCursor += sprintf(asciiCursor,"%c",*cursor);
            } else {
                asciiCursor += sprintf(asciiCursor,".");
            }
            cursor++;
        }
        LOGGER(LOG_DEBUG|LOG_OPTS, SIMPLE_DEBUG_LOG_HEADER " %s\t%s" DEBUG_EOL,hexa,ascii);
    }
    LOGGER(LOG_DEBUG|LOG_OPTS,SIMPLE_DEBUG_LOG_HEADER " *** end of memory dump of %s (size = %d bytes) ***" DEBUG_EOL,memoryName,size);
}

#define DEBUG_DUMP_MEMORY(Var,Size) FILTER dumpMemory(#Var,Var,Size)

/* context messages */
#define CTX_MSG(fmt,...)    FILTER ctxLog(LOG_DEBUG|LOG_OPTS,SIMPLE_DEBUG_LOG_HEADER fmt DEBUG_EOL, ##__VA_ARGS__)

#ifdef __cplusplus
#define INFO_STREAM            FILTER  logInfo.getInstance()  << CPP_SIMPLE_DEBUG_LOG_HEADER
#define DEBUG_STREAM           FILTER  logDebug.getInstance() << CPP_SIMPLE_DEBUG_LOG_HEADER
#define CTX_STREAM             FILTER  logContext.getInstance()   << CPP_SIMPLE_DEBUG_LOG_HEADER

template <class T> inline void DebugVar(const T &v,const char *name, const char *file, const unsigned int line, const char *function) {
    logDebug.getInstance() << CPP_SIMPLE_DEBUG_LOG_HEADER << file << "(" << line << ") " << function << ": " << name << " = " << v << std::endl;
}

inline void DebugVar(const bool v,const char *name, const char *file, const unsigned int line, const char *function) {
    logDebug.getInstance() << CPP_SIMPLE_DEBUG_LOG_HEADER << file << "(" << line << ") " << function << ": " << name << " = " << (v?"true":"false") << std::endl;
}

#define DEBUG_CPP_VAR(x)  FILTER DebugVar(x,#x,__FILE__,__LINE__,__FUNCTION__)

#endif  /* __cplusplus */

#else /* debug */

#define INFO_MSG(fmt,...)
#define DEBUG_MSG(fmt,...)
#define DEBUG_POSMSG(fmt,...)
#define DEBUG_MARK 
#define DEBUG_VAR(x,f)
#define DEBUG_VAR_BOOL(Var)
#define DEBUG_DUMP_MEMORY(Var,Size)
#define CTX_MSG(fmt,...)

#ifdef __cplusplus
#define INFO_STREAM            if (0) logInfo.getInstance() 
#define DEBUG_STREAM           if (0) logDebug.getInstance()
#define CTX_STREAM             if (0) logContext.getInstance()
#define DEBUG_CPP_VAR(x)
#endif  /*__cplusplus*/

#endif /* debug */

#if (GCC_VERSION > 42100) /* GCC 4.2.1 */
#pragma GCC diagnotic pop
#endif /* 4.2.1 */

#endif /* _DEBUG_MACROS_H_ */

