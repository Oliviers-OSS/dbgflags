#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <dbgflags/syslogex.h>
#include <dbgflags/dbgflags.h>
#include <dbgflags/loggers.h>
#include "system.h"

#define DEBUG_LOG_HEADER "fileLogger"
#define LOGGER consoleLogger
#define LOG_OPTS  0


#ifdef _DEBUGFLAGS_H_
static DebugFlags debugFlags =
{
  "fileLogger",
  {
    "file"
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""}
  ,0xFFFFFFFF
};
#define FLAG_FILE  FLAG_1

#else /* _DEBUGFLAGS_H_ */
#define FILTER
#define FLAG_FILE
#endif /* _DEBUGFLAGS_H_ */

#include <dbgflags/debug_macros.h>
/*
 *  WARNING: NOT SIGNAL SAFE !!!!
 *  TODO: manage cancellation points
 */

#define MODULE_FLAG FLAG_FILE

static int	LogStat = 0;		/* status bits, set by openlog() */
static const char *LogTag = NULL;		/* string to tag the entry with */
static int	LogFacility = LOG_USER;	/* default facility code */
static int	LogMask = 0xff;		/* mask of priorities to be logged */
static unsigned int MaxSize = (1024*100);
static int logFile = -1;
static char fullProcessName[PATH_MAX];
static char fullFileName[PATH_MAX];
static char directory[PATH_MAX];
char *processName = NULL;
char *fileDirectory = NULL;
pthread_mutex_t fileLock =   PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static unsigned int fileSize = 0;

static __inline void setDirectory(char *processNamePosition) {
    if (processNamePosition != NULL) {
        *processNamePosition = '\0';
        strcpy(directory,fullProcessName);
        *processNamePosition = '/';
    } else {
        char *homeDir = getenv("$HOME");
        if (homeDir != NULL) {
            strcpy(directory,homeDir);
        } else {
            strcpy(directory,"/tmp");
        }
    }
    DEBUG_VAR(directory,"%s");
    fileDirectory = directory;
}

static __inline void setProcessName(void) {
    int error = getCurrentFullProcessName(fullProcessName);
    if (likely(EXIT_SUCCESS == error)) {
        char *start = strrchr(fullProcessName,'/');
        DEBUG_VAR(fullProcessName,"%s");
        if (unlikely(NULL == fileDirectory)) {
            setDirectory(start);
        } else {
          setDirectory("/tmp");
        }

        if (start != NULL) {
            processName = start+1;
        } else {
            processName = fullProcessName;
        }
    } else { /* EXIT_SUCCESS != getCurrentFullProcessName */
        const pid_t pid = getpid();
        ERROR_MSG("getCurrentFullProcessName error %m",error);
        setDirectory(NULL);
        sprintf(fullProcessName,"process_%u",pid);
        processName = fullProcessName;        
    }
    DEBUG_VAR(processName,"%s");
}

static __inline int setFullFileName(void) {
    int error = EXIT_SUCCESS;

    if (LOG_FILE_ROTATE & LogStat) {
        static char *oldFileName = NULL;
        static char oldFileNameBuffer[PATH_MAX];
        if (unlikely(NULL == oldFileName)) {
            sprintf(oldFileNameBuffer,"%s/%s.bak",directory,processName);
            sprintf(fullFileName,"%s/%s.log",directory,processName);
            oldFileName = oldFileNameBuffer;
        }

        if (unlink(oldFileNameBuffer) == -1) {
            error = errno;
            if (error != ENOENT) {
                ERROR_MSG("unlink %s error (%m)",oldFileNameBuffer,error);
            }
        }

        if (rename(fullFileName,oldFileNameBuffer) == -1) {
            error = errno;
            if (error != ENOENT) {
                ERROR_MSG("unlink %s error (%m)",oldFileNameBuffer,error);
            }
        }

    } else if (LOG_FILE_HISTO & LogStat) {
        time_t currentTime = time(NULL);        
        if (unlikely(((time_t)-1) == currentTime)) {
            currentTime = 0;
        }
        sprintf(fullFileName,"%s/%s_%d.log",directory,processName,currentTime);
    } else {
        sprintf(fullFileName,"%s/%s.log",directory,processName);            
    }
    DEBUG_VAR(fullFileName,"%s");
    return error;
}

static int createFile(void) {

#ifdef _DEBUGFLAGS_H_
  {
    static unsigned int registered = 0;
    if (unlikely(0 == registered)) {
      registered = 1; /* dirty work around to avoid deadlock: syslogex->register->syslogex */
      registered = (registerLibraryDebugFlags(&debugFlags) == EXIT_SUCCESS);
    }
  }
#endif /*_DEBUGFLAGS_H_*/

    int error = pthread_mutex_lock(&fileLock);         

    if (likely(EXIT_SUCCESS == error)) {
        int internalError = EXIT_SUCCESS;        

        if (logFile != -1) {
            WARNING_MSG("logFile was NOT NULL");
            if (close(logFile) != 0) {
                internalError = errno;
                ERROR_MSG("close %d error %m",logFile,internalError);
            }
            logFile = -1;            
        }

        if (unlikely(NULL == processName)) {
            setProcessName();
        } 
        setFullFileName();

        logFile = open(fullFileName,O_WRONLY|O_CREAT|O_TRUNC|O_SYNC,S_IRUSR|S_IWUSR|S_IRGRP);
        if (likely(logFile != -1)) {
            fileSize = 0;
        } else {
            error = errno;
            ERROR_MSG("open %s for creation error (%m)",fullFileName,error);
        }

        internalError = pthread_mutex_unlock(&fileLock);
        if (internalError != EXIT_SUCCESS) {
            ERROR_MSG("pthread_mutex_lock fileLock error (%m)",internalError);        
            if (EXIT_SUCCESS == error) {
                error = internalError;
            }
        }

    } else {
        ERROR_MSG("pthread_mutex_lock fileLock error (%m)",error);        
    }
    
    return error;
}

void openLogFile(const char *ident, int logstat, int logfac) {
    if (ident != NULL) {
		LogTag = ident;
    }
	LogStat = logstat;
    if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0) {
		LogFacility = logfac;
    }
}

void fileLogger(int priority, const char *format, ...) {
    int n = 0;
    int error = EXIT_SUCCESS;
    int internalError = EXIT_SUCCESS;
    const int LogMask = setlogmask(0);

    /* Check priority against setlogmask values. */  
    if ((LOG_MASK (LOG_PRI (priority)) & LogMask) != 0) {
        va_list optional_arguments;   
        char logMsg[2048];
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

        n = vsprintf(logMsg,logFormat,optional_arguments);

        error = pthread_mutex_lock(&fileLock);
        if (likely(EXIT_SUCCESS == error)) {

            if (unlikely(-1 == logFile)) {        
                error = createFile();
                /* error already logged */
            }

            if (likely(EXIT_SUCCESS == error)) {
                ssize_t written = write(logFile,logMsg,n);
                if (written > 0) {
                    if (unlikely(written != n)) {
                        ERROR_MSG("only %d byte(s) of %d has been written to %s",written,n,fullFileName);
                    }
                    fileSize += written;

                    if ((LOG_FILE_ROTATE & LogStat) || (LOG_FILE_HISTO & LogStat)) {
                        if (fileSize >= MaxSize) {
                            close(logFile);
                            logFile = -1;
                        }
                    }
                } else if (0 == written) {
                    ERROR_MSG("nothing has been written in %s",fullFileName);
                } else {
                    error = errno;
                    ERROR_MSG("write to %s error (%m)",fullFileName,error);
                }
            }

            internalError = pthread_mutex_unlock(&fileLock);
            if (internalError != EXIT_SUCCESS) {
                ERROR_MSG("pthread_mutex_lock fileLock error (%m)",internalError);        
                if (EXIT_SUCCESS == error) {
                    error = internalError;
                }
            }
        } else {
            ERROR_MSG("pthread_mutex_lock fileLock error (%m)",error);        
        }            
        va_end(optional_arguments);
    }

    /*return n;*/
}

int setFileLoggerMaxSize(unsigned int maxSizeInBytes) {
    if (maxSizeInBytes != 0) {
        MaxSize = maxSizeInBytes;
    }
    return MaxSize;
}

int setFileLoggerDirectory(const char *logfileDirectory) {
    int error = EINVAL;

    if (logfileDirectory != NULL)
    {
        if (logfileDirectory[0] != '$') {
            strcpy(directory,logfileDirectory);
            DEBUG_VAR(directory,"%s");
            fileDirectory = directory;
        } else {
            char *envVar = getenv(logfileDirectory+1);
            if (envVar != NULL) {
                strcpy(directory,envVar);
                DEBUG_VAR(directory,"%s");
                fileDirectory = directory;
            } else {
                ERROR_MSG("environment variable %s is not found",logfileDirectory);
                error = ENOENT;
            }
        }        
    }

    return error;
}

void closeLogFile(void) {
    if (logFile != -1) {
        close(logFile);
        logFile = -1;
    }
}

#include "ModuleVersionInfo.h"
MODULE_NAME(fileLogger);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(1.0);
MODULE_FILE_VERSION(1.0);
MODULE_DESCRIPTION(logger to file with the same signature than syslog);
MODULE_COPYRIGHT(LGPL);
