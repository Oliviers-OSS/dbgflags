#include "config.h"
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>

#include <dbgflags/dbgflags.h>
#include <dbgflags/syslogex.h>
#define FILTER
#include <dbgflags/debug_macros.h>


int stringToSyslogLevel(const char *syslogLevel) {
    int syslogLevelValue = -1;    
    if (isdigit(syslogLevel[0])) { /* 0x... (hexa value) ) 0.. (octal value or zero) 1 2 3 4 5 6 7 8 9 (decimal value) */
        char *endptr = NULL;
        syslogLevelValue = strtoul(syslogLevel, &endptr, 0);
        if (endptr != syslogLevel) {
            if (syslogLevelValue > LOG_DEBUG) {
                syslogLevelValue = -1;
                errno = EINVAL;
                ERROR_MSG("bad syslog value %d", syslogLevelValue);
            }
            DEBUG_VAR(syslogLevelValue, "%d");
        } else {
            syslogLevelValue = -1;
            errno = EINVAL;
        }
    } else if (strcasecmp(syslogLevel, "debug") == 0) {
        syslogLevelValue = LOG_DEBUG;
        DEBUG_VAR(syslogLevelValue, "%d");
    } else if (strcasecmp(syslogLevel, "info") == 0) {
        syslogLevelValue = LOG_INFO;
        DEBUG_VAR(syslogLevelValue, "%d");
    } else if (strcasecmp(syslogLevel, "notice") == 0) {
        syslogLevelValue = LOG_NOTICE;
        DEBUG_VAR(syslogLevelValue, "%d");
    } else if (strcasecmp(syslogLevel, "warning") == 0) {
        syslogLevelValue = LOG_WARNING;
        DEBUG_VAR(syslogLevelValue, "%d");
    } else if (strcasecmp(syslogLevel, "error") == 0) {
        syslogLevelValue = LOG_ERR;
        DEBUG_VAR(syslogLevelValue, "%d");
    } else if (strcasecmp(syslogLevel, "critical") == 0) {
        syslogLevelValue = LOG_CRIT;
        DEBUG_VAR(syslogLevelValue, "%d");
    } else if (strcasecmp(syslogLevel, "alert") == 0) {
        syslogLevelValue = LOG_ALERT;
        DEBUG_VAR(syslogLevelValue, "%d");
    } else if (strcasecmp(syslogLevel,"emergency") == 0) { /* emergency message are not maskable */
        syslogLevelValue = LOG_EMERG;
	DEBUG_VAR(syslogLevelValue,"%d");
   } else {
#ifdef _DEBUG_
        if (isalnum(syslogLevel[0])) {
            DEBUG_VAR(syslogLevel, "%s");
        } else {
            DEBUG_VAR(syslogLevel[0], "%d");
        }
#endif	/* _DEBUG_ */
        errno = EINVAL;
    }
    return syslogLevelValue;
}
