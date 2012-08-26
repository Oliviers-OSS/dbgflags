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

#define DEBUG_LOG_HEADER "threadFileLogger"
#define LOGGER consoleLogger
#define LOG_OPTS  0
#define MAX_LOGTAG_SIZE	128

#ifdef _DEBUGFLAGS_H_
static DebugFlags debugFlags =
{
  "threadFileLogger",
  {
    "threadFile"
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
#define FLAG_THREAD_FILE  FLAG_1

#else /* _DEBUGFLAGS_H_ */
#define FILTER
#define FLAG_FILE
#endif /* _DEBUGFLAGS_H_ */

#include <dbgflags/debug_macros.h>

#define MODULE_FLAG FLAG_THREAD_FILE

#ifdef _DEBUG_
#include <assert.h>
#define ASSERT assert
#else
#define ASSERT(x)
#endif _DEBUG_

static char fullProcessName[PATH_MAX];
static char directory[PATH_MAX];
static char *processName = NULL;
static char *fileDirectory = NULL;
//static pthread_mutex_t fileLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

//static int LogMask = 0xff; /* mask of priorities to be logged */

#include "filesUtils.h"

typedef struct ThreadFileData_ {
  int	LogStat;
  int	LogFacility;
  char fullFileName[PATH_MAX];
  /*char directory[PATH_MAX];  useless */
  char LogTag[MAX_LOGTAG_SIZE];		/* string to tag the entry with */
  size_t maxSize; /* in bytes */
  time_t maxDuration; /* in seconds */
  int logFile; /* file descriptor */
  size_t fileSize; /* in bytes */ 
  time_t startTime; /* UNIX EPOCH time */
} ThreadFileData;

static inline void initThreadFileData(ThreadFileData *data) {
  /* set default values */
  data->LogStat = 0; /* status bits, set by openlog() */
  data->LogFacility = LOG_USER; /* default facility code */
  data->fullFileName[0] = '\0'; /* will be set by the createfile function */
  /*data->directory[0] = '\0';*/ /* will be set by the createfile function or the openLogThreadFile function */
  data->LogTag[0] = '\0';
  data->maxSize = (1024*100); /* 100 ko */
  data->maxDuration = 24*60*60; /* 24 hours */
  data->logFile = -1; 
  data->fileSize = 0;
  data->startTime = 0;
}

/* TSD key used to store information used to manage on file per thread */
static pthread_key_t TLSThreadFileKey;

/* Once-only initialisation of the key */
static pthread_once_t TLSThreadFileKeyOnce = PTHREAD_ONCE_INIT;

static void freeThreadFileData(void * buffer) {  
  if (buffer) {
    ThreadFileData *threadFileData = (ThreadFileData *)buffer;
    if (threadFileData->logFile != -1) {
      close(threadFileData->logFile);
      threadFileData->logFile = -1;
    }
    free(buffer);
  }
}

static void initThreadFileLoggers() {
  pthread_key_create(&TLSThreadFileKey, freeThreadFileData);
  setProcessName();
}

static inline ThreadFileData *getThreadFileData() {
  pthread_once(&TLSThreadFileKeyOnce, initThreadFileLoggers);
  ThreadFileData *threadFileData = (ThreadFileData *)pthread_getspecific(TLSThreadFileKey);
  if (unlikely(NULL == threadFileData)) {
     /* first call for this thread:  
      * allocate memory to store information for it and initialize them
      */
     threadFileData = malloc(sizeof(ThreadFileData));
     if (likely(threadFileData != NULL)) {
       initThreadFileData(threadFileData);
       pthread_setspecific(TLSThreadFileKey, threadFileData);
     } else {       
       const pid_t tid = gettid();
       ERROR_MSG("failed to allocate %u bytes to manage the log file for the thread %u",sizeof(ThreadFileData),tid);
     }
  } //else already set
  return threadFileData;
}

static __inline int setThreadFullFileName(ThreadFileData *threadFileData) {
    int error = EXIT_SUCCESS;
    ASSERT(threadFileData);
    const pid_t tid = gettid();
    const int LogStat = threadFileData->LogStat;    
    char *fullFileName = threadFileData->fullFileName;
    
    if (LOG_FILE_ROTATE & LogStat) {        
      /* 
       * naming is done according to the following pattern:
       * directory / processName_<tid>_<LogTag> [.log|.bak]
       */       
       char oldFileNameBuffer[PATH_MAX];
       size_t n = strlen(threadFileData->fullFileName);
       if (unlikely(0 == n)) {
	  /* first call, currently we don't manage the case the logger is used BEFORE the LogTag is set */
	  /*const pid_t tid = gettid();*/	  
	  if (threadFileData->LogTag[0]) {
	    sprintf(oldFileNameBuffer,"%s/%s_%u_%s.bak", directory, processName,tid,threadFileData->LogTag);
	    sprintf(fullFileName,"%s/%s_%u_%s.log", directory, processName,tid,threadFileData->LogTag);
	  } else {
	    sprintf(oldFileNameBuffer, "%s/%s_%u.bak", directory, processName,tid);
	    sprintf(fullFileName,"%s/%s_%u.log", directory, processName,tid);
	  }
       } else {
	 /* compute the oldFileNameBuffer from the current file's name */
	 const size_t n = strlen(fullFileName);
	 char *suffixe = NULL;
	 strcpy(oldFileNameBuffer,fullFileName);
	 suffixe = oldFileNameBuffer + n - strlen(".log"); /* strlen will be computed during the compilation */
	 strcpy(suffixe,".bak");	 
       }                   

      if (unlink(oldFileNameBuffer) == -1) {
	  error = errno;
	  if (error != ENOENT) {
	      ERROR_MSG("unlink %s error %d (%m)", oldFileNameBuffer, error);
	  }
      }

      if (rename(fullFileName, oldFileNameBuffer) == -1) {
	  error = errno;
	  if (error != ENOENT) {
	      ERROR_MSG("unlink %s error %d (%m)", oldFileNameBuffer, error);
	  }
      }

    } else if (LOG_FILE_HISTO & LogStat) {
      /* 
       * naming is done according to the following pattern:
       * directory / processName_<tid>_<LogTag>_<time>.log
       */       
        time_t currentTime = time(NULL);
        if (unlikely(((time_t) - 1) == currentTime)) {
            currentTime = 0;
        }        
        if (threadFileData->LogTag[0]) {
	  sprintf(fullFileName, "%s/%s_%u_%s_%u.log", directory, processName,tid,threadFileData->LogTag,currentTime);
	} else {
	  sprintf(fullFileName, "%s/%s_%u_%u.log", directory, processName,tid,currentTime);
	}
    } else {
        sprintf(fullFileName, "%s/%s_%u.log", directory, processName,tid);
    }
    DEBUG_VAR(fullFileName, "%s");
    return error;
}

static inline int createFile(ThreadFileData *threadFileData) {

#ifdef _DEBUGFLAGS_H_
  {
    static unsigned int registered = 0;
    if (unlikely(0 == registered)) {
      registered = 1; /* dirty work around to avoid deadlock: syslogex->register->syslogex */
      registered = (registerLibraryDebugFlags(&debugFlags) == EXIT_SUCCESS);
    }
  }
#endif /*_DEBUGFLAGS_H_*/
    int error = EXIT_SUCCESS;
    ASSERT(threadFileData);
        
    int internalError = EXIT_SUCCESS;        
    if (threadFileData->logFile != -1) {
	WARNING_MSG("logFile was NOT NULL");
	if (close(threadFileData->logFile) != 0) {
	    internalError = errno;
	    ERROR_MSG("close %d error %d (%m)",threadFileData->logFile, internalError);
	}
	threadFileData->logFile = -1;
    }
    
    setThreadFullFileName(threadFileData);

    threadFileData->logFile = open(threadFileData->fullFileName,O_WRONLY|O_CREAT|O_TRUNC|O_SYNC,S_IRUSR|S_IWUSR|S_IRGRP);
    if (likely(threadFileData->logFile != -1)) {
      threadFileData->fileSize = 0;
      if (LOG_FILE_DURATION & threadFileData->LogStat) {
	  threadFileData->startTime = time(NULL);
      }
    } else {
	error = errno;
	ERROR_MSG("open %s for creation error %d (%m)",threadFileData->fullFileName, error);
    }     
    
    return error;
}

void openLogThreadFile(const char *ident, int logstat, int logfac) {
    ThreadFileData *threadFileData = getThreadFileData();
    if (likely(threadFileData)) {
      if (ident != NULL) {
	strncpy(threadFileData->LogTag,ident,MAX_LOGTAG_SIZE);
	threadFileData->LogTag[MAX_LOGTAG_SIZE-1] = '\0';
      }
      threadFileData->LogStat = logstat;
      if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0) {
	  threadFileData->LogFacility = logfac;
      }
    } //error already printed       
}

typedef struct cleanup_args_ {
    void *buffer;    
    //char *logBuffer;
    /* mutex is a static global variable so... */
} cleanup_args;

static void cancel_handler(void *ptr) {
    cleanup_args *args = (cleanup_args *)ptr;
    if ((args != NULL)){
      if (args->buffer != NULL) {
	free(args->buffer);
	args->buffer = NULL;
      }      
    }    
}

/* strcpy then strcat function using the same cursor */
static inline void strcpyNcat(char *buffer, const char *source1,const char *source2) {
  register char *p = buffer;
  
  register const char *s = source1;
  while(*s != '\0') {
    *p = *s;
    p++;
    s++;
  }
  
  s = source2;
  while(*s != '\0') {
    *p = *s;
    p++;
    s++;
  }  
  
  *p = '\0';
}

void vthreadFileLogger(int priority, const char *format, va_list optional_arguments) {
    int n = 0;
    int error = EXIT_SUCCESS;
    const int saved_errno = errno;
    int internalError = EXIT_SUCCESS;
    const int LogMask = setlogmask(0);

    /* Check priority against setlogmask values. */  
    if ((LOG_MASK (LOG_PRI (priority)) & LogMask) != 0) {        
        ThreadFileData *threadFileData = getThreadFileData();
	if (likely(threadFileData)) {
	  char *buf = 0;
	  size_t bufsize = 1024;
	  FILE *f = open_memstream(&buf, &bufsize);
	  if (f != NULL) {
	    struct tm now_tm;
	    time_t now;
	    const int LogStat = threadFileData->LogStat;
	    
	    (void) time(&now);	 
	    n = strftime(f->_IO_write_ptr,f->_IO_write_end - f->_IO_write_ptr,"%h %e %T ",localtime_r(&now, &now_tm));
	    f->_IO_write_ptr += n;
	    
	    if (threadFileData->LogTag[0]) {
	      n += fprintf(f,"%s: ",threadFileData->LogTag);
	    }
	    
	    if (LogStat & LOG_PID) {
	      if (LogStat & LOG_TID) {
		  const pid_t tid = gettid();		  
		  n +=fprintf(f,"[%d:%d]", (int) getpid (),(int) tid);
	      } else {		  
		  n += fprintf(f,"[%d]", (int) getpid ());
	      }             
	    } /* (LogStat & LOG_PID) */
	    
	    if (LogStat & LOG_RDTSC) {
	      const unsigned long long int  t = rdtsc();	      
	      n += fprintf(f,"(%llu)",t);
	    } /* (LogStat & LOG_RDTSC) */
	  
	    if (LogStat & LOG_CLOCK) {
		#if HAVE_CLOCK_GETTIME
		    struct timespec timeStamp;
		    if (clock_gettime(CLOCK_MONOTONIC,&timeStamp) == 0) {
			/*cursor += sprintf (cursor, "(%lu.%.9d)",timeStamp.tv_sec,timeStamp.tv_nsec);*/
			n += fprintf(f,"(%lu.%.9d)",timeStamp.tv_sec,timeStamp.tv_nsec);
		    } else {
			const int error = errno;
			ERROR_MSG("clock_gettime CLOCK_MONOTONIC error %d (%m)",error);
		    }
		#else
		    static unsigned int alreadyPrinted = 0; /* to avoid to print this error msg on each call */
		    if (unlikely(0 == alreadyPrinted)) {
			ERROR_MSG("clock_gettime  not available on this system");
			alreadyPrinted = 1;
		    }
		#endif
	    } /* (LogStat & LOG_CLOCK) */
	    
	    if (LogStat & LOG_LEVEL) {
		  switch(LOG_PRI(priority)) {
		  case LOG_EMERG:			  
			  n += fprintf(f, "[EMERG]");
			  break;
		  case LOG_ALERT:			  
			  n += fprintf(f, "[ALERT]");
			  break;
		  case LOG_CRIT:			  
			  n += fprintf(f, "[CRIT]");
			  break;
		  case LOG_ERR:			  
			  n += fprintf(f, "[ERROR]");
			  break;
		  case LOG_WARNING:			  
			  n += fprintf(f, "[WARNING]");
			  break;
		  case LOG_NOTICE:			  
			  n += fprintf(f, "[NOTICE]");
			  break;
		  case LOG_INFO:			  
			  n += fprintf(f, "[INFO]");
			  break;
		  case LOG_DEBUG:			  
			  n += fprintf(f, "[DEBUG]");
			  break;
		  default:			  
			  fprintf(f,"[<%d>]",priority);
		  } /* switch(priority) */        
	    } /* (LogStat & LOG_LEVEL) */
	    
	    errno = saved_errno; /* restore errno for %m format */  
	    n += vfprintf(f, format, optional_arguments);
		      
	    /* Close the memory stream; this will finalize the data
		into a malloc'd buffer in BUF.  */
	    fclose(f);
	    
	    /*
	     * Some of the function used below are thread cancellation points
	     * (according to Advanced Programming in the Unix Environment 2nd ed p411) 
	     * so set the handler to avoid deadlocks and memory leaks
	     */
	    cleanup_args cancelArgs;
	    cancelArgs.buffer = buf;
	    pthread_cleanup_push(cancel_handler, &cancelArgs);
	    
	    if (LogStat & LOG_PERROR) {
	      /* add the format argument to the header */
	      const size_t n = strlen(format);
	      const size_t size = bufsize + n + 1;
	      char *logFormatBuffer = (char *)malloc(size);
	      if (logFormatBuffer) {
		pthread_cleanup_push(free,logFormatBuffer);
		strcpyNcat(logFormatBuffer,buf,format);
		vfprintf(stderr, logFormatBuffer, optional_arguments);
		pthread_cleanup_pop(1); /* pop the handler and free the allocated memory */
	      } else {
		ERROR_MSG("failed to allocate %u bytes to print the msg on stderr",size);
	      }		  
	    } /* (LogStat & LOG_PERROR) */
	    
	    /* file management */
	    if (LOG_FILE_DURATION & LogStat) {
		const time_t currentTime = time(NULL);
		const time_t elapsedTime = currentTime - threadFileData->startTime;
		if (unlikely(elapsedTime >= threadFileData->maxDuration)) {
		    close(threadFileData->logFile);
		    threadFileData->logFile = -1;                    
		}
	    } /* (LOG_FILE_DURATION & LogStat) */
	    
	    if (unlikely(-1 == threadFileData->logFile)) {        
		error = createFile(threadFileData);
		/* error already logged */
	    }
	    
	    if (likely(EXIT_SUCCESS == error)) {
	      const ssize_t written = write(threadFileData->logFile,buf,n);
	      if (written > 0) {
		  if (unlikely(written != n)) {
		      ERROR_MSG("only %d byte(s) of %d has been written to %s",written,n,threadFileData->fullFileName);
		      if (LogStat & LOG_CONS) {
			  int fd = open("/dev/console", O_WRONLY | O_NOCTTY, 0);
			  if (fd >= 0 ) {
			      dprintf(fd,"logMsg");
			      close(fd);
			      fd = -1;
			  }
		      } /* (LogStat & LOG_CONS) */
		  } /* (unlikely(written != n)) */
		  threadFileData->fileSize += written;

		  if ((LOG_FILE_ROTATE & LogStat) || (LOG_FILE_HISTO & LogStat)) {
		      if (threadFileData->fileSize >= threadFileData->maxSize) {
			  close(threadFileData->logFile);
			  threadFileData->logFile = -1;
		      }
		  }
	      } else if (0 == written) {
		  WARNING_MSG("nothing has been written in %s", threadFileData->fullFileName);
	      } else {
		  error = errno;
		  ERROR_MSG("write to %s error %d (%m)", threadFileData->fullFileName, error);
	      }
	  } /*(likely(EXIT_SUCCESS == error)) */
	    
	    pthread_cleanup_pop(0);  /* moved to avoid a build error, use the preprocessor to understand why 
	                              * or have a look on man 3 pthread_cleanup_push: 
				      * push & pop MUST BE in the SAME lexical nesting level !!! 
				      */	    
	    free(buf);
	    cancelArgs.buffer = buf = NULL;
	  } else { /* open_memstream(&buf, &bufsize) == NULL */
	    ERROR_MSG("failed to get stream buffer");
	    /* TMP! display the raw message to the console */
	    vfprintf(stderr,format,optional_arguments);
	    /* TODO: write the raw message to the file */
	  }
	} /* (likely(threadFileData)) error already logged */
    } /* ((LOG_MASK (LOG_PRI (priority)) & LogMask) != 0) */
    /* message has not to be displayed because of the current LogMask and its priority */

    /*return n;*/
}

void threadFileLogger(int priority, const char *format, ...) {
    va_list optional_arguments;
    va_start(optional_arguments, format);
    vthreadFileLogger(priority, format,  optional_arguments);
    va_end(optional_arguments);
}

int setThreadFileLoggerMaxSize(const size_t maxSizeInBytes) {
  int maxSize = 0;
  ThreadFileData *threadFileData = getThreadFileData();
  if (likely(threadFileData)) {
    if (maxSizeInBytes != 0) {            
        threadFileData->maxSize = maxSizeInBytes;
    } 
    maxSize = threadFileData->maxSize;
  } else { /* error already logged */
    maxSize = -ENOMEM;
  }
  return maxSize;
}

int setThreadFileLoggerMaxDuration(const time_t maxDurationInSeconds) {
    int maxDuration = 0;
    ThreadFileData *threadFileData = getThreadFileData();
    if (likely(threadFileData)) {
      if (maxDurationInSeconds != 0) {
        threadFileData->maxDuration = maxDurationInSeconds;
      }
      maxDuration = threadFileData->maxDuration;
    } else { /* error already logged */
      maxDuration = -ENOMEM;
    }
    return maxDuration;
}

int setThreadFileLoggerDirectory(const char *logfileDirectory) {
    int error = EINVAL;
    ThreadFileData *threadFileData = getThreadFileData();
    if (likely(threadFileData)) {
      if (logfileDirectory != NULL) {        
	  if (logfileDirectory[0] != '$') {
	      error = checkDirectory(logfileDirectory);            
	  } else {
	      char *envVar = getenv(logfileDirectory+1);
	      if (envVar != NULL) {
		  error = checkDirectory(logfileDirectory);
	      } else {
		  error = ENOENT;
		  ERROR_MSG("environment variable %s is not found",logfileDirectory);
	      }        
	  } /* !(logfileDirectory[0] != '$') */
	  if (EXIT_SUCCESS == error) {
	      strcpy(directory, logfileDirectory);
	      DEBUG_VAR(directory, "%s");	      
	  }
      } /* (logfileDirectory != NULL) */
    } else { /* error already logged */
      error = ENOMEM;
    }

    return error;
}

void closeLogThreadFile(void) {
  ThreadFileData *threadFileData = getThreadFileData();
  if (likely(threadFileData)) {
    if (threadFileData->logFile != -1) {
        close(threadFileData->logFile);
        threadFileData->logFile = -1;
    }
  }
}

#include <dbgflags/ModuleVersionInfo.h>
MODULE_NAME(threadFileLogger);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(1.0);
MODULE_FILE_VERSION(1.0);
MODULE_DESCRIPTION(logger to files (one file per thread) with the same signature than syslog using POSIX API);
MODULE_COPYRIGHT(LGPL);
