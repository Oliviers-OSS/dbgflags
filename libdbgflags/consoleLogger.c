#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <dbgflags/syslogex.h>
#include <dbgflags/loggers.h>
#include <dbgflags/debug_macros.h>
#include "system.h"


static int	LogStat = 0;		/* status bits, set by openlog() */
static const char *LogTag = NULL;		/* string to tag the entry with */
static int	LogFacility = LOG_USER;	/* default facility code */
//static int	LogMask = 0xff;		/* mask of priorities to be logged */

void openConsoleLogger(const char *ident, int logstat, int logfac) {
    if (ident != NULL) {
		LogTag = ident;
    }
    LogStat = logstat;
    if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0) {
		LogFacility = logfac;
    }
}

extern void setConsoleLoggerOpt(const char *ident, int logstat, int logfac) __attribute((alias("openConsoleLogger")));
   
void vconsoleLogger(int priority, const char *format, va_list optional_arguments) {
    /*int n = 0;*/
    const int saved_errno = errno;
    const int LogMask = setlogmask(0);

    /* no cancellation point is currently used in this function 
    * (according to Advanced Programming in the Unix Environment 2nd ed p411) 
     * so there is no thread cancellation clean-up handlers defined      
     */
    
    /* Check for invalid bits. */
    if (unlikely(priority & ~(LOG_PRIMASK | LOG_FACMASK))) {
        /*syslog(INTERNALLOG,
               "syslog: unknown facility/priority: %x", pri);*/
        WARNING_MSG("unknown facility/priority: %x", priority);
        priority &= LOG_PRIMASK | LOG_FACMASK;
    }

    /* Check priority against setlogmask values. */  
    if ((LOG_MASK (LOG_PRI (priority)) & LogMask) != 0) {
        char *buf = 0;
        size_t bufsize = 1024;
        /*char logFormat[1024];
        char *cursor = logFormat;*/
        FILE *f = open_memstream(&buf, &bufsize);
	if (f != NULL) {                
	  struct tm now_tm;
	  time_t now;

          (void) time(&now);
          /*cursor += strftime(cursor,sizeof(logFormat),"%h %e %T ",localtime_r(&now, &now_tm));*/
	  f->_IO_write_ptr += strftime(f->_IO_write_ptr,f->_IO_write_end - f->_IO_write_ptr,"%h %e %T ",localtime_r(&now, &now_tm));

	  if (LogTag) {
	    //cursor += sprintf (cursor,"%s: ",LogTag);
	    fprintf(f,"%s: ",LogTag);
	  }

	  if (LogStat & LOG_PID) {
	      if (LogStat & LOG_TID) {
		  const pid_t tid = gettid();
		  /*cursor += sprintf (cursor, "[%d:%d]", (int) getpid (),(int) tid);*/
		  fprintf(f,"[%d:%d]", (int) getpid (),(int) tid);
	      } else {
		  /*cursor += sprintf (cursor, "[%d]", (int) getpid ());*/
		  fprintf(f,"[%d]", (int) getpid ());
	      }             
	  }

	  if (LogStat & LOG_RDTSC) {
	      const unsigned long long int  t = rdtsc();
	      /*cursor += sprintf (cursor, "(%llu)",t);*/
	      fprintf(f,"(%llu)",t);
	  } /* (LogStat & LOG_RDTSC) */

	  if (LogStat & LOG_CLOCK) {
	      #if HAVE_CLOCK_GETTIME
		  struct timespec timeStamp;
		  if (clock_gettime(CLOCK_MONOTONIC,&timeStamp) == 0) {
		      /*cursor += sprintf (cursor, "(%lu.%.9d)",timeStamp.tv_sec,timeStamp.tv_nsec);*/
		      fprintf(f,"(%lu.%.9d)",timeStamp.tv_sec,timeStamp.tv_nsec);
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
			fprintf(f,"[EMERG] %s",format);
			break;
		case LOG_ALERT:
			/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Alert * %s",format);*/
			fprintf(f,"[ALERT] %s",format);
			break;
		case LOG_CRIT:
			/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Critical * %s",format);*/
			fprintf(f,"[CRIT] %s",format);
			break;
		case LOG_ERR:
			/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Error * %s",format);*/
			fprintf(f,"[ERROR] %s",format);
			break;
		case LOG_WARNING:
			/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Warning * %s",format);*/
			fprintf(f,"[WARNING] %s",format);
			break;
		case LOG_NOTICE:
			/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Notice * %s",format); */
			fprintf(f,"[NOTICE] %s",format);
			break;
		case LOG_INFO:
			/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Info * %s",format);*/
			fprintf(f,"[INFO] %s",format);
			break;
		case LOG_DEBUG:
			/*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"* Debug * %s",format);*/
			fprintf(f,"[DEBUG] %s",format);
			break;
		default:
			/*cursor += sprintf(cursor,"* <%d> * %s",priority,format);*/
			fprintf(f,"[<%d>] %s",priority,format);
		} /* switch(priority) */        
	  } else { /* (LogStat & LOG_LEVEL) */
		  /*cursor += snprintf(cursor,sizeof(logFormat) - (cursor - logFormat),"%s",format);*/
		  fprintf(f,"%s",format);
	  }

	  /*n =*/ /*vfprintf(stderr,logFormat,optional_arguments);*/
	  /* Close the memory stream; this will finalize the data
           into a malloc'd buffer in BUF.  */
	  fclose(f);
	  errno = saved_errno; /* restore errno for %m format */
	  vfprintf(stderr,buf,optional_arguments);
	  free(buf);
	} else {
	  /* We cannot get a stream: try to write directly to the console (warning may be splitted by other msg) */
	  struct tm now_tm;
	  time_t now;
	  char buffer[20];	  

	  WARNING_MSG("failed to get stream buffer, using the console without buffering instead");
          (void) time(&now);
	  strftime(buffer,sizeof(buffer),"%h %e %T ",localtime_r(&now, &now_tm));
	  fprintf(stderr,"%s",buffer);
	  
	  if (LogTag) {	    
	    printf("%s: ",LogTag);
	  }

	  if (LogStat & LOG_PID) {
	      if (LogStat & LOG_TID) {
		  const pid_t tid = gettid();		  
		  fprintf(stderr,"[%d:%d]", (int) getpid (),(int) tid);
	      } else {		  
		  fprintf(stderr,"[%d]", (int) getpid ());
	      }             
	  }

	  if (LogStat & LOG_RDTSC) {
	      const unsigned long long int  t = rdtsc();	      
	      fprintf(stderr,"(%llu)",t);
	  } /* (LogStat & LOG_RDTSC) */
	  
	  if (LogStat & LOG_CLOCK) {
	      #if HAVE_CLOCK_GETTIME
		  struct timespec timeStamp;
		  if (clock_gettime(CLOCK_MONOTONIC,&timeStamp) == 0) {		      
		      fprintf(stderr,"(%lu.%.9d)",timeStamp.tv_sec,timeStamp.tv_nsec);
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
			fprintf(stderr,"* Emergency * ");
			break;
		case LOG_ALERT:			
			fprintf(stderr,"* Alert * ");
			break;
		case LOG_CRIT:
			fprintf(stderr,"* Critical * ");
			break;
		case LOG_ERR:			
			fprintf(stderr,"* Error * ");
			break;
		case LOG_WARNING:			
			fprintf(stderr,"* Warning * ");
			break;
		case LOG_NOTICE:			
			fprintf(stderr,"* Notice * ");
			break;
		case LOG_INFO:			
			fprintf(stderr,"* Info * ");
			break;
		case LOG_DEBUG:			
			fprintf(stderr,"* Debug * ");
			break;
		default:			
			fprintf(f,"* <%d> * ",priority);
		} /* switch(priority) */        
	  } 
	  
	  vfprintf(stderr,format,optional_arguments);
	}
    } /* ((LOG_MASK (LOG_PRI (priority)) & LogMask) != 0) */
    /* message has not to be displayed because of the current LogMask and its priority */

    /*return n;*/
}

void consoleLogger(int priority, const char *format, ...) {    
    va_list optional_arguments;
    va_start(optional_arguments, format);
    vconsoleLogger(priority, format,  optional_arguments);
    va_end(optional_arguments);
    /*return n;*/
}

#include <dbgflags/ModuleVersionInfo.h>
MODULE_NAME(consoleLogger);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(0.1);
MODULE_FILE_VERSION(1.0);
MODULE_DESCRIPTION(logger to error console with the same signature than syslog);
MODULE_COPYRIGHT(LGPL);


