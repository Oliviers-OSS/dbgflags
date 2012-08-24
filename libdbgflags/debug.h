#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <dbgflags/syslogex.h>
#define DEBUG_LOG_HEADER "libdbgflags"
#define FILTER
#include <dbgflags/debug_macros.h>

#if 0
#define LOG_POS_HEADER LOG_NAME " (" /*__FUNCTION__*/ __FILE__ ":%d):"
#define LOG_HEADER LOG_NAME ":"
#define LOG_OPTS  LOG_CONS|LOG_PERROR|LOG_PID
#define DEBUG_EOL "\n"
#define ERROR_MSG(fmt,...)   syslog(LOG_ERR|LOG_OPTS,LOG_POS_HEADER fmt,__LINE__, ##__VA_ARGS__)
#define WARNING_MSG(fmt,...)   syslog(LOG_WARNING|LOG_OPTS,LOG_POS_HEADER fmt,__LINE__, ##__VA_ARGS__)
#define DEBUG_MSG(fmt,...)   syslog(LOG_DEBUG|LOG_OPTS,LOG_HEADER fmt, ##__VA_ARGS__)
#define DEBUG_VAR(x,f)       syslog(LOG_DEBUG|LOG_OPTS, LOG_HEADER __FILE__ "(%d) %s: " #x " = " f DEBUG_EOL,__LINE__,__FUNCTION__,x)
#endif

#endif /* _DEBUG_H_ */
