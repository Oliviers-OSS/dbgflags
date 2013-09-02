#include "config.h"
#define _XOPEN_SOURCE 600

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

#define MODULE_FLAG FLAG_FILE

static int	LogStat = 0;		/* status bits, set by openlog() */
static const char *LogTag = NULL;		/* string to tag the entry with */
static int	LogFacility = LOG_USER;	/* default facility code */
//static int LogMask = 0xff; /* mask of priorities to be logged */
static size_t maxSize = (1024*100);
static int logFile = -1;
static char fullProcessName[PATH_MAX];
static char fullFileName[PATH_MAX];
static char directory[PATH_MAX];
static char *processName = NULL;
static char *fileDirectory = NULL;
static pthread_mutex_t fileLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static size_t fileSize = 0;
static time_t maxDuration = 24*60*60; /* 24 hours */
static time_t startTime = 0;

#include "filesUtils.h"
#include "setFullFileName.h"

static inline int createFile(void) {

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
		int flags = O_WRONLY|O_CREAT|O_TRUNC;

		if (likely((!(LogStat & LOG_FILE_WITHOUT_SYNC)) && (!(LogStat & LOG_FILE_SYNC_ON_ERRORS_ONLY)))) {
			flags |= O_SYNC; /* enable synchronous I/O to avoid data lost in case of crash */
		}

		if (logFile != -1) {
			WARNING_MSG("logFile was NOT NULL");
			if (close(logFile) != 0) {
				internalError = errno;
				ERROR_MSG("close %d error %d (%m)", logFile, internalError);
			}
			logFile = -1;
		}

		if (unlikely(NULL == processName)) {
			setProcessName();
		}
		setFullFileName();

		logFile = open(fullFileName,flags,S_IRUSR|S_IWUSR|S_IRGRP);
		if (likely(logFile != -1)) {
			fileSize = 0;
			if (LOG_FILE_DURATION & LogStat) {
				startTime = time(NULL);
			}
		} else {
			error = errno;
			ERROR_MSG("open %s for creation error %d (%m)", fullFileName, error);
		}

		internalError = pthread_mutex_unlock(&fileLock);
		if (internalError != EXIT_SUCCESS) {
			ERROR_MSG("pthread_mutex_lock fileLock error %d (%s)", internalError, strerror(internalError));
			if (EXIT_SUCCESS == error) {
				error = internalError;
			}
		}

	} else {
		ERROR_MSG("pthread_mutex_lock fileLock error %d (%m)", error);
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

	if (LogStat & LOG_NDELAY) {
		createFile();
	}
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
		/*if (args->logBuffer != NULL) {
	free(args->logBuffer);
	args->logBuffer = NULL;
      }*/
	}
	/* Free the lock.  */
	pthread_mutex_unlock(&fileLock);
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

void vfileLogger(int priority, const char *format, va_list optional_arguments) {
	int n = 0;
	int error = EXIT_SUCCESS;
	const int saved_errno = errno;
	int internalError = EXIT_SUCCESS;
	const int LogMask = setlogmask(0);

	/* Check priority against setlogmask values. */
	if ((LOG_MASK (LOG_PRI (priority)) & LogMask) != 0) {
		//char logMsg[2048];
		//char logFormat[1024];
		//char *cursor = logFormat;
		char *buf = 0;
		size_t bufsize = 1024;
		FILE *f = open_memstream(&buf, &bufsize);
		if (f != NULL) {
			struct tm now_tm;
			time_t now;

			(void) time(&now);
			/*cursor += strftime(cursor,sizeof(logFormat),"%h %e %T ",localtime_r(&now, &now_tm));*/
			n = strftime(f->_IO_write_ptr,f->_IO_write_end - f->_IO_write_ptr,"%h %e %T ",localtime_r(&now, &now_tm));
			//f->_IO_write_ptr += strftime(f->_IO_write_ptr,f->_IO_write_end - f->_IO_write_ptr,"%h %e %T ",localtime_r(&now, &now_tm));
			f->_IO_write_ptr += n;

			if (LogTag) {
				/*cursor += sprintf (cursor,"%s: ",LogTag);*/
				n += fprintf(f,"%s: ",LogTag);
			}

			if (LogStat & LOG_PID) {
				if (LogStat & LOG_TID) {
					const pid_t tid = gettid();
					/*cursor += sprintf (cursor, "[%d:%d]", (int) getpid (),(int) tid);*/
					n += fprintf(f,"[%d:%d]", (int) getpid (),(int) tid);
				} else {
					/*cursor += sprintf (cursor, "[%d]", (int) getpid ());*/
					n += fprintf(f,"[%d]", (int) getpid ());
				}
			}

			if (LogStat & LOG_RDTSC) {
				const unsigned long long int  t = rdtsc();
				/*cursor += sprintf (cursor, "(%llu)",t);*/
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
					/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Emergency * %s",format);*/
					n += fprintf(f, "[EMERG]");
					break;
				case LOG_ALERT:
					/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Alert * %s",format);*/
					n += fprintf(f, "[ALERT]");
					break;
				case LOG_CRIT:
					/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Critical * %s",format);*/
					n += fprintf(f, "[CRIT]");
					break;
				case LOG_ERR:
					/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Error * %s",format);*/
					n += fprintf(f, "[ERROR]");
					break;
				case LOG_WARNING:
					/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Warning * %s",format);*/
					n += fprintf(f, "[WARNING]");
					break;
				case LOG_NOTICE:
					/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Notice * %s",format); */
					n += fprintf(f, "[NOTICE]");
					break;
				case LOG_INFO:
					/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Info * %s",format);*/
					n += fprintf(f, "[INFO]");
					break;
				case LOG_DEBUG:
					/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Debug * %s",format);*/
					n += fprintf(f, "[DEBUG]");
					break;
				default:
					/*cursor += sprintf(cursor,"* <%d> * %s",priority,format);*/
					n += fprintf(f,"[<%d>]",priority);
				} /* switch(priority) */
			} /* (LogStat & LOG_LEVEL) */

			errno = saved_errno; /* restore errno for %m format */
			n += vfprintf(f, format, optional_arguments);

			/* Close the memory stream; this will finalize the data
		into a malloc'd buffer in BUF.  */
			fclose(f);
			/*
			 * begin of the critical section.
			 * Some of the function used below are thread cancellation points
			 * (according to Advanced Programming in the Unix Environment 2nd ed p411)
			 * so set the handler to avoid deadlocks and memory leaks
			 */
			cleanup_args cancelArgs;
			cancelArgs.buffer = buf;
			pthread_cleanup_push(cancel_handler, &cancelArgs);

			error = pthread_mutex_lock(&fileLock);
			if (likely(EXIT_SUCCESS == error)) {

				if (unlikely(LogStat & LOG_PERROR)) {
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
				if (unlikely(LOG_FILE_DURATION & LogStat)) {
					const time_t currentTime = time(NULL);
					const time_t elapsedTime = currentTime - startTime;
					if (unlikely(elapsedTime >= maxDuration)) {
						close(logFile);
						logFile = -1;
					}
				} /* (LOG_FILE_DURATION & LogStat) */

				if (unlikely(-1 == logFile)) {
					error = createFile();
					/* error already logged */
				}

				if (likely(EXIT_SUCCESS == error)) {
					const ssize_t written = write(logFile,buf,n);
					if (written > 0) {
						if (unlikely(written != n)) {
							ERROR_MSG("only %d byte(s) of %d has been written to %s",written,n,fullFileName);
							if (LogStat & LOG_CONS) {
								int fd = open("/dev/console", O_WRONLY | O_NOCTTY, 0);
								if (fd >= 0 ) {
									dprintf(fd,"logMsg");
									close(fd);
									fd = -1;
								}
							} /* (LogStat & LOG_CONS) */

							if (unlikely((LogStat & LOG_FILE_SYNC_ON_ERRORS_ONLY) && (LOG_PRI(priority) <= LOG_ERR))) {
								/* flush data if the log priority is "upper" or equal to error */
								error = fdatasync(logFile);
								if (error != 0) {
									error = errno;
									ERROR_MSG("fdatasync to %s error %d (%m)",fullFileName,error);
								}
								error = posix_fadvise(logFile, 0,0,POSIX_FADV_DONTNEED); /* tell the OS that log message bytes could be released from the file system cache */
								if (error != 0) {
								  ERROR_MSG("posix_fadvise to %s error %d (%m)",fullFileName,error);
								}
							} /* (unlikely((LogStat & LOG_FILE_SYNC_ON_ERRORS_ONLY) && (LOG_PRI(priority) <= LOG_ERR))) */
						} /* (unlikely(written != n)) */
						fileSize += written;

#ifdef FILESYSTEM_PAGE_CACHE_FLUSH_THRESHOLD
						static size_t currentPageCacheMaxSize = 0;
						currentPageCacheMaxSize += written;
						if (currentPageCacheMaxSize >= FILESYSTEM_PAGE_CACHE_FLUSH_THRESHOLD) {
							/* tell the OS that log message bytes could be released from the file system cache */
							if (likely(posix_fadvise(logFile, 0,0,POSIX_FADV_DONTNEED) == 0)) {
								currentPageCacheMaxSize = 0;
								DEBUG_MSG("used file system cache allowed to be flushed (size was %u)",currentPageCacheMaxSize);
							} else {
								NOTICE_MSG("posix_fadvise to %s error %d (%m), current page cache max size is %u",fullFileName,error,currentPageCacheMaxSize);
							}
						}
#endif /* FILESYSTEM_PAGE_CACHE_FLUSH_THRESHOLD */

						if ((LOG_FILE_ROTATE & LogStat) || (LOG_FILE_HISTO & LogStat)) {
							if (fileSize >= maxSize) {
								close(logFile);
								logFile = -1;
							}
						}
					} else if (0 == written) {
						WARNING_MSG("nothing has been written in %s", fullFileName);
					} else {
						error = errno;
						ERROR_MSG("write to %s error %d (%m)", fullFileName, error);
					}
				} /*(likely(EXIT_SUCCESS == error)) */

				/* End of critical section.  */
				/*pthread_cleanup_pop(0); implementation can't allow to set this instruction here */

				/* free allocated ressources */
				internalError = pthread_mutex_unlock(&fileLock);
				if (internalError != EXIT_SUCCESS) {
					ERROR_MSG("pthread_mutex_lock fileLock error %d (%s)", internalError, strerror(internalError));
					if (EXIT_SUCCESS == error) {
						error = internalError;
					}
				} /* (internalError != EXIT_SUCCESS) */
			} else {
				ERROR_MSG("pthread_mutex_lock fileLock error %d (%s)", error, strerror(error));
			}

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
	} /* ((LOG_MASK (LOG_PRI (priority)) & LogMask) != 0) */
	/* message has not to be displayed because of the current LogMask and its priority */

	/*return n;*/
}

void fileLogger(int priority, const char *format, ...) {
	va_list optional_arguments;
	va_start(optional_arguments, format);
	vfileLogger(priority, format,  optional_arguments);
	va_end(optional_arguments);
}

int setFileLoggerMaxSize_1_0(unsigned int maxSizeInBytes) {
	if (maxSizeInBytes != 0) {
		maxSize = maxSizeInBytes;
	}
	return maxSize;
}
/* function only available in the DSO to keep compatibility with the previous release */
#if defined(__pic__) || defined(__PIC__) || defined(PIC)
asm(".symver setFileLoggerMaxSize_1_0,setFileLoggerMaxSize@VERS_1.0");
#endif  /* defined(__pic__) || defined(__PIC__) || defined(PIC) */


int setFileLoggerMaxSize_1_1(const size_t maxSizeInBytes) {
	if (maxSizeInBytes != 0) {
		maxSize = maxSizeInBytes;
	}
	return maxSize;
}

/* new function available in the DSO and the static library */
#if defined(__pic__) || defined(__PIC__) || defined(PIC)
asm(".symver setFileLoggerMaxSize_1_1,setFileLoggerMaxSize@@VERS_1.1");
#else /* defined(__pic__) || defined(__PIC__) || defined(PIC) */
extern setFileLoggerMaxSize(size_t maxSizeInBytes) __attribute((alias("setFileLoggerMaxSize_1_1")));
#endif  /* defined(__pic__) || defined(__PIC__) || defined(PIC) */

int setFileLoggerMaxDuration(const time_t maxDurationInSeconds) {
	if (maxDurationInSeconds != 0) {
		maxDuration = maxDurationInSeconds;
	}
	return maxDuration;
}

int setFileLoggerDirectory(const char *logfileDirectory) {
	int error = EINVAL;

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
			fileDirectory = directory;
		}
	} /* (logfileDirectory != NULL) */

	return error;
}

void closeLogFile(void) {
	if (logFile != -1) {
		close(logFile);
		logFile = -1;
	}
}

#include <dbgflags/ModuleVersionInfo.h>
MODULE_NAME(fileLogger);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(1.0);
MODULE_FILE_VERSION(1.2);
MODULE_DESCRIPTION(logger to files with the same signature than syslog using POSIX API);
MODULE_COPYRIGHT(LGPL);
