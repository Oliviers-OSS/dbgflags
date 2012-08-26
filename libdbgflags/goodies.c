#include "config.h"
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <strings.h>
#define SYSLOG_NAMES
#include <syslog.h>

#include <dbgflags/dbgflags.h>
#include <dbgflags/syslogex.h>
#include <dbgflags/loggers.h>
#include <dbgflags/goodies.h>

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
    } else { /* !(isdigit(syslogLevel[0])) */
        CODE *cursor = prioritynames;
        while(cursor->c_name != NULL) {
            if (strcasecmp(syslogLevel, cursor->c_name) != 0) {
                cursor++;
   } else {
                syslogLevelValue =  cursor->c_val;
                break;
            }
        } /* while(cursor->c_name != NULL) */

        if (NULL == cursor->c_name) {
#ifdef _DEBUG_
        if (isalnum(syslogLevel[0])) {
            DEBUG_VAR(syslogLevel, "%s");
        } else {
            DEBUG_VAR(syslogLevel[0], "%d");
        }
#endif	/* _DEBUG_ */
            errno = EINVAL;
        } /* (NULL == cursor->c_name) */
    } /* !(isdigit(syslogLevel[0])) */
    return syslogLevelValue;
}

int stringToFacility(const char *facilityString) {
    int facility = -1;
    if (isdigit(facilityString[0])) {
      char *endptr = NULL;
      const int n = strtoul(facilityString,&endptr,0);
      if (endptr != facilityString) {
        if (   ((n >= 0) && (n < 12))
            || ((n > 15) && (n < 24))) {
            facility = (n << 3);
        }
      }
    } else {
        CODE *cursor = facilitynames;
        while(cursor->c_name != NULL) {
            if (strcasecmp(facilityString, cursor->c_name) != 0) {
                cursor++;
            } else {
                facility =  cursor->c_val;
                break;
            }
        } /* while(cursor->c_name != NULL) */

        if (NULL == cursor->c_name) {
        ERROR_MSG("unknown facility value (%s)",facilityString);
        errno = EINVAL;
        } /* (NULL == cursor->c_name) */
    } /* !(isdigit(facilityString[0])) */
    return facility;
}

size_t parseSize(const char *string) {
    char *end = NULL;
    size_t size = strtol(string, &end, 10);
    if ( (errno != ERANGE || (size != LONG_MAX && size != LONG_MIN)) && (errno == 0 || size != 0)) {
    while (*end != '\0') {
            /* check if there is the value is followed by a valid unity: kB 1000, Ko 1024, MB 1000*1000, Mo 1024*1024 */
        switch (*end) {
            case ' ':
                end++;
                    break;
            case 'M':
                /* break is missing */
            case 'm':
                    end++;
                switch (*end) {
                    case 'B': /* MB = 1000*1000 */
                        size = size * 1000 * 1000;
                        break;
                    case ' ': /* M = Mo = 1024*1024 */
                        /* break is missing */
                    case 'O':
                        /* break is missing */
                    case 'o':
                        end++;
                        /* break is missing */
                    case '\0':
                        size = size * 1024 * 1024;
                        break;
                    default:
                            //WARNING_MSG("bad unit (M%s)", end);
                        break;
                } /* switch(*end) M* */
                break;
            case 'K':
                /* break is missing */
            case 'k':
                    end++;
                switch (*end) {
                    case 'B': /* kB = 1000 */
                        size = size * 1000;
                        break;
                    case ' ': /* k = ko = 1024 */
                        /* break is missing */
                    case 'O':
                        /* break is missing */
                    case 'o':
                        end++;
                        /* break is missing */
                    case '\0':
                        size = size * 1024;
                        break;
                    default:
                            //WARNING_MSG("bad unit (k%s)", end);
                        break;
                } /* switch(*end) k* */
                break;
            case 'o':
                /* break is missing */
            case 'b':
                end++;
                break;
            default:
                    //WARNING_MSG("bad unit (%s)", end);
                end++;
        } /* switch(*end) */
    } /* while (*end != '\0') */
    } else {
        //WARNING_MSG("strtol error");
        size = 0;
    }

    return size;
}

static inline const char *parseInteger(const char *string,unsigned int *n) {
    register const char *cursor = string;
    register unsigned int i = 0;
    while(isdigit(*cursor)) {
        i = (i * 10) + (*cursor - '0');
        cursor++;
    }
    *n = i;
    return cursor;
}

static const char *moveToNextToken(const char *string) {
    register const char *cursor = string;
    while(isspace(*cursor)) {
        cursor++;
    }
    return cursor;
}

static const char *moveToEnd(const char *string) {
    register const char *cursor = string;
    while( *cursor != '\0') {
        ++cursor;
    }
    return cursor;
}

typedef enum durationParserStates_ {
    init
    ,days
    ,hours
    ,minutes
    ,seconds
} durationParserStates;

time_t parseDuration(const char *string) {
    /*
     * Allowed format:
     * jj hh:mn:ss
     *  hh:mn:ss
     *  nn
     *  nn s
     *  nn mn
     *  nn h
     *  nn j
     */

    durationParserStates state = init;
    time_t duration = 0;
    register const char *cursor = string;
    unsigned int n;
    while(*cursor != '\0') {
        cursor = parseInteger(cursor,&n);
        cursor = moveToNextToken(cursor);
        switch(*cursor) {
            case '\0':
                duration = duration * 60 + n;
                break;            
            case 's':
                duration += n;
                cursor++;
                break;
            case 'm':
                cursor++;
                if ('n' == *cursor) {
                    duration += n*60;
                    cursor++;
                } else {
                    WARNING_MSG("unknow time unit at m%s", cursor);
                    duration = 0;
                    cursor = moveToEnd(cursor);
                }                
                break;
            case 'h':
                duration += n*3600;
                cursor++;
                break;
            case 'j':
                duration += n*24*3600;
                cursor++;
                break;
            case ':':
                /* hh:mn:ss */
                switch(state) {
                    case init:
                        duration = n;
                        state = hours; /* hours read */
                        break;
                    case days:
                        duration = duration * 24  + n;
                        state = hours; /* hours read */
                        break;
                    case hours:
                        duration = duration * 60 + n;
                        state = minutes; /* minutes read */
                        break;
                    case minutes:
                        duration = (duration + n) * 60;
                        state = seconds; /* seconds read */
                    case seconds:
                        WARNING_MSG("bad string time format %s",string);
                        duration = 0;
                        cursor = moveToEnd(cursor) - 1;
                        break;
                } /* switch(state) */
                cursor++;
                break;
            default:
                if (isdigit(*cursor)) {
                    /* jj hh:mn:ss */
                    state = days; /* days read */
                    duration = n;
                } else {
                    WARNING_MSG("bad string format %s",string);
                    duration = 0;
                    cursor = moveToEnd(cursor) - 1;
                }
        } /* switch(*cursor) */
    } /*(cursor != '\0')*/

    return duration;
}

#define ADD_FLAG_IF(Flags,Flag,Item)		if (strcasecmp(#Flag,Item) == 0) { Flags |= Flag; } else

unsigned int parseFlagsOptions(const char *flagsOptions) {
	unsigned int flags = 0;  
	const char *i= flagsOptions;
	
	while( *i != '\0') {	
		const size_t n = strlen(i);
		ADD_FLAG_IF(flags,LOG_PID,i)
		ADD_FLAG_IF(flags,LOG_CONS,i)
		ADD_FLAG_IF(flags,LOG_ODELAY,i)
		ADD_FLAG_IF(flags,LOG_NDELAY,i)
		ADD_FLAG_IF(flags,LOG_NOWAIT,i)
		ADD_FLAG_IF(flags,LOG_PERROR,i)
		ADD_FLAG_IF(flags,LOG_TID,i)
		ADD_FLAG_IF(flags,LOG_RDTSC,i)
		ADD_FLAG_IF(flags,LOG_LEVEL,i)
		ADD_FLAG_IF(flags,LOG_CLOCK,i)
		ADD_FLAG_IF(flags,LOG_FILE_ROTATE,i)
		ADD_FLAG_IF(flags,LOG_FILE_HISTO,i)
		ADD_FLAG_IF(flags,LOG_FILE_DURATION,i)
		WARNING_MSG("unknown flag name %s",i);
		i += n + 1;
	}
	
	return flags;
}
#undef ADD_FLAG_IF

#include <dbgflags/ModuleVersionInfo.h>
MODULE_NAME(goodies);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(1.2);
MODULE_FILE_VERSION(1.3);
MODULE_DESCRIPTION(goodies functions);
MODULE_COPYRIGHT(LGPL);

