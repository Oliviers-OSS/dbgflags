/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * =====================================================================
 *                              HISTORY
 * =====================================================================
 * 1.0 | initial version: additionals flags...
 * ---------------------------------------------------------------------
 * 1.1 | asynchonous methods added
 * ---------------------------------------------------------------------
 * 1.2 | new functions for the tools proclog 
 * ---------------------------------------------------------------------
 * 1.3 | RHEL5 compatibility: removed last clibs internals locks
 * ---------------------------------------------------------------------
 * 1.4 | Debian6 compatibility: all references to __have_sock_cloexec removed 
 * =====================================================================
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)syslogex based on syslog.c	8.4 (Berkeley) 3/18/94";
#endif /* LIBC_SCCS and not lint */

#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <dbgflags/syslogex.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netdb.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <bits/libc-lock.h>
#include <signal.h>
#include <locale.h>
#include <pthread.h>
#include <linux/unistd.h>
#include <sys/times.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/wait.h>

#include "system.h"

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <dbgflags/dbgflags.h>
#define DEBUG_LOG_HEADER "syslogex"
#ifdef LOGGER
#undef LOGGER
#endif
#define LOGGER syslog
#define LOG_OPTS  0 /*LOG_CONS|LOG_PERROR|LOG_PID*/
#include <dbgflags/debug_macros.h>

/*#include <libio/iolibio.h>
#include <math_ldbl_opt.h>*/

#ifndef BUFFER_READ_TIMEOUT
#define BUFFER_READ_TIMEOUT 500
#endif /* BUFFER_MAX_READ_DELAY */

#ifndef BUFFER_WRITE_TIMEOUT
#define BUFFER_WRITE_TIMEOUT  (250*1000)
#endif /* BUFFER_WRITE_TIMEOUT */

#define MS_TO_SEC(x) ((x)/1000)
#define MS_TO_NS(x)  ((x)*1000000)
#define MS_TO_NSEC_MINUS_SEC(x)  (((x)-(1000*MS_TO_SEC(x)))*1000000)

#if (GCC_VERSION < 40000) /* GCC 4.0.0 */
#define __vfprintf_chk(f, flag, fmt, ap) vfprintf(f, fmt, ap)
#endif /* GCC 4.0.0 */

/*#define ftell(s) INTUSE(_IO_ftell) (s)*/

static int LogType = SOCK_DGRAM; /* type of socket connection */
static int LogFile = -1; /* fd for log */
static int connected; /* have done connect */
static int LogStat; /* status bits, set by openlog() */
static const char *LogTag; /* string to tag the entry with */
static int LogFacility = LOG_USER; /* default facility code */
//static int LogMask = 0xff; /* mask of priorities to be logged */
extern char *__progname; /* Program name, from crt0. */

/* Define the lock.  */
static pthread_mutex_t syslogex_lock = PTHREAD_MUTEX_INITIALIZER;
/*__libc_lock_define_initialized (static, syslogex_lock)*/

static void openlog_internal(const char *, int, int);
static void closelog_internal(void);
#ifndef NO_SIGPIPE
static void sigpipe_handler(int);
#endif

#ifndef send_flags
#define send_flags 0
#endif

#ifdef _DEBUGFLAGS_H_
static DebugFlags debugFlags ={
    "syslogex",
    {
        "runtime"
        , "async"
        , "buffers"
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
        , ""
    }
    , 0xFFFFFFFF
};

#define FLAG_RUNTIME  FLAG_1
#define FLAG_ASYNC    FLAG_2
#define FLAG_BUFFER   FLAG_3

#else /* __DEBUGFLAGS_H_ */
#define FLAG_RUNTIME
#define FLAG_ASYNC
#define FLAG_BUFFER
#undef FILTER
#define FILTER
#endif /*__DEBUGFLAGS_H_ */

#define MODULE_FLAG FLAG_RUNTIME

struct cleanup_arg {
    void *buf;
    struct sigaction *oldaction;
};

static void
cancel_handler(void *ptr) {
#ifndef NO_SIGPIPE
    /* Restore the old signal handler.  */
    struct cleanup_arg *clarg = (struct cleanup_arg *) ptr;

    if (clarg != NULL && clarg->oldaction != NULL)
        __sigaction(SIGPIPE, clarg->oldaction, NULL);
#endif

    if (clarg != NULL && clarg->buf != NULL) {
      free(clarg->buf);
      clarg->buf = NULL;
    }
    
    /* Free the lock.  */
    pthread_mutex_unlock(&syslogex_lock);
}

static void __set_errno(int new_errno) {
    errno = new_errno;
}

static void
__vsyslogex_chk(int pri, int flag, pid_t cpid, pid_t ctid, const char *fmt, va_list ap) {
    struct tm now_tm;
    time_t now;
    int fd;
    FILE *f;
    char *buf = 0;
    size_t bufsize = 0;
    size_t prioff, msgoff;
#ifndef NO_SIGPIPE
    struct sigaction action, oldaction;
    int sigpipe;
#endif
    int saved_errno = errno;
    char failbuf[3 * sizeof (pid_t) + sizeof "out of memory []"];
    const int LogMask = setlogmask(0);

#ifdef _DEBUGFLAGS_H_
    {
        static unsigned int registered = 0;
        if (unlikely(0 == registered)) {
            registered = 1; /* dirty work around to avoid deadlock: syslogex->register->syslogex */
            registered = (registerLibraryDebugFlags(&debugFlags) == EXIT_SUCCESS);
        }
    }
#endif /*_DEBUGFLAGS_H_*/

#define INTERNALLOG LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID
    /* Check for invalid bits. */
    if (unlikely(pri & ~(LOG_PRIMASK | LOG_FACMASK))) {
        /*syslog(INTERNALLOG,
               "syslog: unknown facility/priority: %x", pri);*/
        WARNING_MSG("unknown facility/priority: %x", pri);
        pri &= LOG_PRIMASK | LOG_FACMASK;
    }

    /* Check priority against setlogmask values. */
    if (unlikely((LOG_MASK(LOG_PRI(pri)) & LogMask) == 0))
        return;

    /* Set default facility if none specified. */
    if (unlikely((pri & LOG_FACMASK) == 0))
        pri |= LogFacility;

    /* Build the message in a memory-buffer stream.  */
    f = open_memstream(&buf, &bufsize);
    if (unlikely(f == NULL)) {
        /* We cannot get a stream.  There is not much we can do but
           emitting an error messages with the Process ID.  */
        char numbuf[3 * sizeof (pid_t)];
        char *nump;
        char *endp = __stpcpy(failbuf, "out of memory [");
        pid_t pid = getpid();

        nump = numbuf + sizeof (numbuf);
        /* The PID can never be zero.  */
        do {
            *--nump = '0' + pid % 10;
        } while ((pid /= 10) != 0);

        endp = mempcpy((void*) endp, (const void*) nump, (size_t) ((numbuf + sizeof (numbuf)) - nump));
        *endp++ = ']';
        *endp = '\0';
        buf = failbuf;
        bufsize = endp - failbuf;
        msgoff = 0;
    } else {
        __fsetlocking(f, FSETLOCKING_BYCALLER);
        prioff = fprintf(f, "<%d>", pri);
        (void) time(&now);
        f->_IO_write_ptr += strftime(f->_IO_write_ptr,
                f->_IO_write_end - f->_IO_write_ptr,
                "%h %e %T ",
                localtime_r(&now, &now_tm));
        /*f->_IO_write_ptr += strftime_l (f->_IO_write_ptr,
                                          f->_IO_write_end - f->_IO_write_ptr,
                                          "%h %e %T ",
                                          localtime_r (&now, &now_tm));*/
        msgoff = ftell(f);
        if (LogTag == NULL)
            LogTag = __progname;
        if (LogTag != NULL)
            fputs_unlocked(LogTag, f);
        if (LogStat & LOG_PID) {
            const pid_t pid = ((0 == cpid) ? getpid() : cpid);
            if (LogStat & LOG_TID) {
                const pid_t tid = ((0 == ctid) ? gettid() : ctid);
                fprintf(f, "[%d:%d]", (int) pid, (int) tid);
            } else {
                fprintf(f, "[%d]", (int) pid);
            }
        }

        if (LogStat & LOG_RDTSC) {
            const unsigned long long int t = rdtsc();
            fprintf(f, "(%llu)", t);
        } /* (LogStat & LOG_RDTSC) */

        if (LogStat & LOG_CLOCK) {
            #if HAVE_CLOCK_GETTIME
                struct timespec timeStamp;
                if (clock_gettime(CLOCK_MONOTONIC,&timeStamp) == 0) {
                    fprintf(f,"(%lu.%.9d)",timeStamp.tv_sec,timeStamp.tv_nsec);
                } else {
                    const int error = errno;
                    ERROR_MSG("clock_gettime CLOCK_MONOTONIC error %d (%m)",error);
            }
            #else
                static unsigned int alreadyPrinted = 0;
                if (unlikely(0 == alreadyPrinted)) {
                    ERROR_MSG("clock_gettime  not available on this system");
                    alreadyPrinted = 1;
                }
            #endif
        }  /* (LogStat & LOG_CLOCK) */

        if (LogStat & LOG_LEVEL) {
            switch (LOG_PRI(pri)) {
                case LOG_EMERG:
                    fprintf(f, "[EMERG]");
                    break;
                case LOG_ALERT:
                    fprintf(f, "[ALERT]");
                    break;
                case LOG_CRIT:
                    fprintf(f, "[CRIT]");
                    break;
                case LOG_ERR:
                    fprintf(f, "[ERROR]");
                    break;
                case LOG_WARNING:
                    fprintf(f, "[WARNING]");
                    break;
                case LOG_NOTICE:
                    fprintf(f, "[NOTICE]");
                    break;
                case LOG_INFO:
                    fprintf(f, "[INFO]");
                    break;
                case LOG_DEBUG:
                    fprintf(f, "[DEBUG]");
                    break;
            } /* switch(LOG_PRI(pri))*/
        } /* (LogStat & LOG_LEVEL) */

        if (LogTag != NULL) {
            putc_unlocked(':', f);
            putc_unlocked(' ', f);
        }

        /* Restore errno for %m format.  */
        __set_errno(saved_errno);

        /* We have the header.  Print the user's format into the
         buffer.  */
        if (flag == -1) {
            vfprintf(f, fmt, ap);
        } else {
            __vfprintf_chk(f, flag, fmt, ap);
        }

        /* Close the memory stream; this will finalize the data
           into a malloc'd buffer in BUF.  */
        fclose(f);
    }

    /* Output to stderr if requested. */
    if (LogStat & LOG_PERROR) {
        struct iovec iov[2];
        register struct iovec *v = iov;

        v->iov_base = buf + msgoff;
        v->iov_len = bufsize - msgoff;
        /* Append a newline if necessary.  */
        if (buf[bufsize - 1] != '\n') {
            ++v;
            v->iov_base = (char *) "\n";
            v->iov_len = 1;
        }

        pthread_cleanup_push(free, buf == failbuf ? NULL : buf);

        /* writev is a cancellation point.  */
        (void) writev(STDERR_FILENO, iov, v - iov + 1);

        pthread_cleanup_pop(0);
    }

    /* Prepare for multiple users.  We have to take care: open and
  write are cancellation points.  */
    struct cleanup_arg clarg;
    clarg.buf = buf;
    clarg.oldaction = NULL;
    pthread_cleanup_push(cancel_handler, &clarg);
    pthread_mutex_lock(&syslogex_lock);

#ifndef NO_SIGPIPE
    /* Prepare for a broken connection.  */
    memset(&action, 0, sizeof (action));
    action.sa_handler = sigpipe_handler;
    sigemptyset(&action.sa_mask);
    sigpipe = __sigaction(SIGPIPE, &action, &oldaction);
    if (sigpipe == 0)
        clarg.oldaction = &oldaction;
#endif

    /* Get connected, output the message to the local logger. */
    if (!connected) {
        openlog_internal(LogTag, LogStat | LOG_NDELAY, 0);
    }

    /* If we have a SOCK_STREAM connection, also send ASCII NUL as
  a record terminator.  */
    if (LogType == SOCK_STREAM) {
        ++bufsize;
    }

    if (!connected || __send(LogFile, buf, bufsize, send_flags) < 0) {
        if (connected) {
            /* Try to reopen the syslog connection.  Maybe it went
          down.  */
            closelog_internal();
            openlog_internal(LogTag, LogStat | LOG_NDELAY, 0);
        }

        if (!connected || __send(LogFile, buf, bufsize, send_flags) < 0) {
            closelog_internal(); /* attempt re-open next time */
            /*
             * Output the message to the console; don't worry
             * about blocking, if console blocks everything will.
             * Make sure the error reported is the one from the
             * syslogd failure.
             */
            if (LogStat & LOG_CONS &&
                    (fd = __open(_PATH_CONSOLE, O_WRONLY | O_NOCTTY, 0)) >= 0) {
                dprintf(fd, "%s\r\n", buf + msgoff);
                (void) __close(fd);
            }
        }
    }

#ifndef NO_SIGPIPE
    if (sigpipe == 0)
        __sigaction(SIGPIPE, &oldaction, (struct sigaction *) NULL);
#endif

    /* End of critical section.  */
    pthread_cleanup_pop(0);
    pthread_mutex_unlock(&syslogex_lock);

    if (buf != failbuf) {
        free(buf);
    }
}

/*
 * syslog, vsyslog --
 *	print message on log file; output is intended for syslogd(8).
 */
void
syslogex(int pri, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    __vsyslogex_chk(pri, -1, 0, 0, fmt, ap);
    va_end(ap);
}

static void
__syslogex_chk(int pri, int flag, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    __vsyslogex_chk(pri, flag, 0, 0, fmt, ap);
    va_end(ap);
}

static void
__syslogex_pid_chk(pid_t pid, int pri, int flag, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    __vsyslogex_chk(pri, flag, pid, 0, fmt, ap);
    va_end(ap);
}

void vsyslogex(int pri, const char *fmt, va_list ap) {
    __vsyslogex_chk(pri, -1, 0, 0, fmt, ap);
}

static struct sockaddr_un SyslogAddr; /* AF_UNIX address of local logger */

static void
openlog_internal(const char *ident, int logstat, int logfac) {
    if (ident != NULL)
        LogTag = ident;
    LogStat = logstat;
    if (logfac != 0 && (logfac &~LOG_FACMASK) == 0)
        LogFacility = logfac;

    int retry = 0;
    while (retry < 2) {
        if (LogFile == -1) {
            SyslogAddr.sun_family = AF_UNIX;
            (void) strncpy(SyslogAddr.sun_path, _PATH_LOG,
                    sizeof (SyslogAddr.sun_path));
            if (LogStat & LOG_NDELAY) {
#ifdef SOCK_CLOEXEC
#ifndef __ASSUME_SOCK_CLOEXEC
                //if (__have_sock_cloexec >= 0) {
#endif /* __ASSUME_SOCK_CLOEXEC */
                    LogFile = socket(AF_UNIX,
                            LogType
                            | SOCK_CLOEXEC, 0);
#ifndef __ASSUME_SOCK_CLOEXEC
                  /*  if (__have_sock_cloexec == 0)
                        __have_sock_cloexec
                            = ((LogFile != -1
                            || errno != EINVAL)
                            ? 1 : -1);
                }*/
#endif /* __ASSUME_SOCK_CLOEXEC */
#endif /* SOCK_CLOEXEC */
#ifndef __ASSUME_SOCK_CLOEXEC
#ifdef SOCK_CLOEXEC
                //if (__have_sock_cloexec < 0)
#endif /* SOCK_CLOEXEC */
                    LogFile = socket(AF_UNIX, LogType, 0);
#endif /* __ASSUME_SOCK_CLOEXEC */
                if (LogFile == -1)
                    return;
#ifndef __ASSUME_SOCK_CLOEXEC
#ifdef SOCK_CLOEXEC
                //if (__have_sock_cloexec < 0)
#endif /* SOCK_CLOEXEC */
                    __fcntl(LogFile, F_SETFD, FD_CLOEXEC);
#endif /* __ASSUME_SOCK_CLOEXEC */
            }
        }
        if (LogFile != -1 && !connected) {
            int old_errno = errno;
            if (__connect(LogFile, &SyslogAddr, sizeof (SyslogAddr))
                    == -1) {
                int saved_errno = errno;
                int fd = LogFile;
                LogFile = -1;
                (void) __close(fd);
                __set_errno(old_errno);
                if (saved_errno == EPROTOTYPE) {
                    /* retry with the other type: */
                    LogType = (LogType == SOCK_DGRAM
                            ? SOCK_STREAM : SOCK_DGRAM);
                    ++retry;
                    continue;
                }
            } else
                connected = 1;
        }
        break;
    }
}

void
openlogex(const char *ident, int logstat, int logfac) {
    /* Protect against multiple users and cancellation.  */
    pthread_cleanup_push(cancel_handler, NULL);
    pthread_mutex_lock(&syslogex_lock);

    openlog_internal(ident, logstat, logfac);

    pthread_cleanup_pop(1);
}

#ifndef NO_SIGPIPE

static void
sigpipe_handler(int signo) {
    closelog_internal();
}
#endif

static void
closelog_internal() {
    if (!connected)
        return;

    __close(LogFile);
    LogFile = -1;
    connected = 0;
}

void
closelogex() {
    /* Protect against multiple users and cancellation.  */
    pthread_cleanup_push(cancel_handler, NULL);
    pthread_mutex_lock(&syslogex_lock);

    closelog_internal();
    LogTag = NULL;
    LogType = SOCK_DGRAM; /* this is the default */

    /* Free the lock.  */
    pthread_cleanup_pop(1);
}

/* setlogmask -- set the log mask level */
int
setlogmaskex(int pmask)
{
    return setlogmask(pmask);
}

/* asynchronous versions */
#include "buffers.h"

#undef MODULE_FLAG
#define MODULE_FLAG FLAG_ASYNC

static pthread_t logger = 0;
static pthread_mutex_t logger_lock = PTHREAD_MUTEX_INITIALIZER;
static buffer_data buffers;

static void *syslogger(void* arg) {
    va_list ap;
    while (1) {
        /* wait for a filled buffer */
        buffer_msg* pbuffer = getReadBuffer(&buffers);
        if (likely(pbuffer)) {
            __vsyslogex_chk(pbuffer->prio, -1, 0, pbuffer->caller_tid, pbuffer->msg, ap);
            signalEmptyBufferAvailable(&buffers);
        }
    }
    logger = 0;
    pthread_exit(NULL);
}

static __inline int startLogger() {
    int error = 0;
    pthread_mutex_lock(&logger_lock);
    /* do the test again to manage the case where the caller was waiting for the mutex during the logger start */
    if (0 == logger) {
#ifdef _DEBUGFLAGS_H_
        registerLibraryDebugFlags(&debugFlags);
#endif /*_DEBUGFLAGS_H_*/

        error = buffersInit(&buffers);
        if (likely(0 == error)) {
            /* the logger is created detached to let it dying whith the process host and free irs allocated resources */
            pthread_attr_t threadAttr;
            pthread_attr_init(&threadAttr);
            pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
            error = pthread_create(&logger, &threadAttr, syslogger, (void*) & buffers);
            if (unlikely(error != 0)) {
                ERROR_MSG("pthread_create error %d (%m))", error);
                /*syslogex(INTERNALLOG,"pthread_create error %d (%m))",error);*/
            }
            pthread_attr_destroy(&threadAttr);
        }
        /* error already traced */
    }
    pthread_mutex_unlock(&logger_lock);
    return error;
}

void asyslogex(int pri, const char *fmt, ...) {
    int error = 0;

    /* start the logger if not already done */
    if (unlikely(0 == logger)) {
        error = startLogger();
    }

    if (likely(0 == error)) {
        /* get a buffer to write */
        buffer_msg* pbuffer = getTimedWriteBuffer(&buffers, BUFFER_WRITE_TIMEOUT);
        if (likely(pbuffer)) {
            /* fill it */
            va_list ap;

            pbuffer->prio = pri;
            pbuffer->caller_tid = gettid();
            va_start(ap, fmt);
            vsnprintf(pbuffer->msg, sizeof (pbuffer->msg), fmt, ap);
            va_end(ap);

            /* signal buffer ready */
            signalFilledBufferAvailable(&buffers);
        } /* (likely(pbuffer))*/
    } /* (likely(0 == error))*/
}

void avsyslogex(int pri, const char *fmt, va_list ap) {
    int error = 0;

    /* start the logger if not already done */
    if (unlikely(0 == logger)) {
        error = startLogger();
    }

    if (likely(0 == error)) {
        /* get a buffer to write */
        buffer_msg* pbuffer = getTimedWriteBuffer(&buffers, BUFFER_WRITE_TIMEOUT);
        if (likely(pbuffer)) {
            /* fill it */

            pbuffer->prio = pri;
            pbuffer->caller_tid = gettid();
            vsnprintf(pbuffer->msg, sizeof (pbuffer->msg), fmt, ap);

            /* signal buffer ready */
            signalFilledBufferAvailable(&buffers);
        }
    }
}

#define CLOSE_PIPE(x) if (x != -1) { close(x); x = -1; }
#define READ   0
#define WRITE  1
#define MAXLINE 1024
#define  NO_SELECT

static inline int execChild(const char *file, char *argv[], int stdoutWritePipe, int stderrWritePipe) {
    int error = EXIT_SUCCESS;

    if (dup2(stdoutWritePipe, STDOUT_FILENO) != -1) { /* close stdout and redirect it to the pipe */
        if (dup2(stderrWritePipe, STDERR_FILENO) != -1) { /* close stderr and redirect it to the pipe */
            //DEBUG_VAR(file,"%s");
            //DEBUG_VAR(argv[0],"%s");
            //usleep(500);
            if (execvp(file, argv)) {
                error = errno;
                ERROR_MSG("execvp error %d (%m)", error);
            }
        } else {
            int error = errno;
            ERROR_MSG("dup2 stderr error %d (%m)", error);
        }
    } else {
        int error = errno;
        ERROR_MSG("dup2 stdout error %d (%m)", error);
    }
    CLOSE_PIPE(stdoutWritePipe);
    CLOSE_PIPE(stderrWritePipe);
    return error;
}

#ifdef NO_SELECT

typedef struct outputToSyslogParam_ {
    pid_t pid;
    int pipe;
    int level;
} outputToSyslogParam;

void* sendChildOutputToSyslogThread(void *p) {
    int error = EXIT_SUCCESS;
    sigset_t blockedSignalsMask;
    outputToSyslogParam *params = (outputToSyslogParam*) p;
    char buffer[MAXLINE];
    ssize_t n;

    sigemptyset(&blockedSignalsMask);
    sigaddset(&blockedSignalsMask, SIGCHLD);
    error = pthread_sigmask(SIG_BLOCK, &blockedSignalsMask, NULL);
    if (error != 0) {
        WARNING_MSG("pthread_sigmask error %d (%m)", error);
    }

    while ((n = read(params->pipe, buffer, sizeof (buffer))) > 0) {
        buffer[n] = '\0';
        DEBUG_VAR(buffer, "%s");
        __syslogex_pid_chk(params->pid, params->level, -1, buffer);
    }

    if (n != 0) {
        const int error = errno;
        ERROR_MSG("read from %d error %d (%m)", params->pipe, error);
    }
    CLOSE_PIPE(params->pipe);
    return NULL;
}

static inline int sendChildrenOutputsToSyslog(const pid_t child, int stdoutReadPipe, int stderrReadPipe, int eventsReadPipe, int stdOutLogLevel, int stdErrLogLevel) {
    int error = EXIT_SUCCESS;
    outputToSyslogParam stdoutParams, stderrParams;
    pthread_t stdoutThread, stderrThread;

    stdoutParams.pid = child;
    stdoutParams.pipe = stdoutReadPipe;
    stdoutParams.level = stdOutLogLevel;

    stderrParams.pid = child;
    stderrParams.pipe = stderrReadPipe;
    stderrParams.level = stdErrLogLevel;

    error = pthread_create(&stdoutThread, NULL, sendChildOutputToSyslogThread, &stdoutParams);
    if (0 == error) {
        error = pthread_create(&stderrThread, NULL, sendChildOutputToSyslogThread, &stderrParams);
        if (0 == error) {
            int childStatus = -1;
            const pid_t pid = waitpid(child, &childStatus, 0);
            if (pid != -1) {
                void *pthreadReturn = NULL;
                DEBUG_MSG("child ended");
                if (WIFEXITED(childStatus)) {
                    const int exitStatus = WEXITSTATUS(childStatus);
                    INFO_MSG("process %d ended with status %d", child, exitStatus);
                } else if WIFSIGNALED(childStatus) {
                    const int signalNumber = WTERMSIG(childStatus);
                    if (WCOREDUMP(childStatus)) {
                        NOTICE_MSG("child process %d terminated by signal %d (core dumped)", child, signalNumber);
                    } else {
                        NOTICE_MSG("child process %d terminated by signal %d", child, signalNumber);
                    }
                }
                error = pthread_join(stdoutThread, &pthreadReturn);
                if (error != 0) {
                    ERROR_MSG("pthread_join stdoutThread error %d (%m)", error);
                }
                error = pthread_join(stderrThread, &pthreadReturn);
                if (error != 0) {
                    ERROR_MSG("pthread_join stderrThread error %d (%m)", error);
                }
            } else {
                error = errno;
                ERROR_MSG("waitpid %d error %d (%m)", child, error);
            }
        } else {
            ERROR_MSG("pthread_create stderr error %d (%m)", error);
        }
    } else {
        ERROR_MSG("pthread_create stdout error %d (%m)", error);
    }
    return error;
}

#else /* NO_SELECT */

static int writeEventPipe = -1;

static void sigChildHandler(int x) {
    DEBUG_MSG("*** signal %d received ***", x);
    if ((SIGCHLD == x) && (writeEventPipe != -1)) {
        int childStatus = -1;
        const pid_t childId = waitpid(-1, &childStatus, WNOHANG);
        if (childId > 0) {
            if (WIFEXITED(childStatus)) {
                const int exitStatus = WEXITSTATUS(childStatus);
                INFO_MSG("process %d ended with status %d", childId, exitStatus);
            } else if WIFSIGNALED(childStatus) {
                const int signalNumber = WTERMSIG(childStatus);
                if (WCOREDUMP(childStatus)) {
                    NOTICE_MSG("child process %d terminated by signal %d (core dumped)", childId, signalNumber);
                } else {
                    NOTICE_MSG("child process %d terminated by signal %d", childId, signalNumber);
                }
            }
            const ssize_t n = write(writeEventPipe, &childStatus, sizeof (childStatus));
            if (n == -1) {
                const int error = errno;
                ERROR_MSG("write to writeEventPipe error %d (%m)", error);
            }
        } else if (-1 == childId) {
            const int error = errno;
            ERROR_MSG("waitpid error %d (%m)", error);
        }
    }
}

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

static inline int sendChildrenOutputsToSyslog(const pid_t child, int stdoutReadPipe, int stderrReadPipe, int eventsReadPipe, int stdOutLogLevel, int stdErrLogLevel) {
    int error = EXIT_SUCCESS;
    fd_set readfds, exceptsfds;
    sigset_t blockedSignalsMask, previousSignalsMask;
    struct sigaction sigAction;
    struct timeval timeout;
    int stop = 0;
    char buffer[MAXLINE];
    int childStatus = EXIT_SUCCESS;
    struct pollfd fds;
    int nfds = -1;

    //DEBUG_MARK;
    FD_ZERO(&readfds);
    FD_SET(stdoutReadPipe, &readfds);
    FD_SET(stderrReadPipe, &readfds);
    nfds = MAX(stdoutReadPipe, stdoutReadPipe);
    FD_SET(eventsReadPipe, &readfds);
    nfds = MAX(nfds, eventsReadPipe);
    nfds++;

    FD_ZERO(&exceptsfds);
    FD_SET(stdoutReadPipe, &exceptsfds);
    FD_SET(stderrReadPipe, &exceptsfds);
    FD_SET(eventsReadPipe, &exceptsfds);

    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    /*sigemptyset(&blockedSignalsMask);
    sigaddset(&blockedSignalsMask,SIGCHLD);
    sigprocmask(SIG_BLOCK,&blockedSignalsMask,&previousSignalsMask);*/

    //DEBUG_VAR(stdoutReadPipe,"%d");
    do {
        const int retval = select(nfds, &readfds, NULL, /*&readfds*//*&exceptsfds*/NULL, &timeout);
        //DEBUG_VAR(retval,"%d");
        if (retval > 0) {

            if (FD_ISSET(stdoutReadPipe, &readfds)) {
                const ssize_t n = read(stdoutReadPipe, buffer, sizeof (buffer));
                //DEBUG_VAR(n,"%d");
                if (n != -1) {
                    buffer[n] = '\0';
                    //DEBUG_VAR(buffer,"%s");
                    __syslogex_pid_chk(child, stdErrLogLevel, -1, buffer);
                } else {
                    error = errno;
                    ERROR_MSG("read stdoutReadPipe error %d (%m)", error);
                }
            }

            if (FD_ISSET(stderrReadPipe, &readfds)) {
                const ssize_t n = read(stderrReadPipe, buffer, sizeof (buffer));
                DEBUG_VAR(n, "%d");
                if (n != -1) {
                    buffer[n] = '\0';
                    //DEBUG_VAR(buffer,"%s");
                    __syslogex_pid_chk(child, stdErrLogLevel, -1, buffer);
                } else {
                    error = errno;
                    ERROR_MSG("read stderrReadPipe error %d (%m)", error);
                }
            }

            if (FD_ISSET(eventsReadPipe, &readfds)) {
                const ssize_t n = read(eventsReadPipe, buffer, sizeof (buffer));
                //DEBUG_VAR(n,"%d");
                if (n != -1) {
                } else {
                    error = errno;
                    ERROR_MSG("read stderrReadPipe error %d (%m)", error);
                }
            }

            /*if ((FD_ISSET(stdoutReadPipe, &exceptsfds)) || (FD_ISSET(stderrReadPipe, &exceptsfds)) || (FD_ISSET(eventsReadPipe, &exceptsfds))) {
                DEBUG_MSG("**** exceptsfds ****");
            }*/

        } else if (0 == retval) {
            NOTICE_MSG("pselect retcode = %d", retval);
            stop = 1;
        } else {
            error = errno;
            NOTICE_MSG("pselect error retcode = %d, errno = %d (%m)", retval, error);
            stop = 1;
        }
    } while (!stop);
    DEBUG_MARK;
    if (waitpid(-1, &childStatus, WNOHANG | WUNTRACED) != -1) {
        NOTICE_MSG("child exited with status %d", childStatus);
    } else {
        error = errno;
        ERROR_MSG("waitpid error %d (%m)", error);
    }
    return error;
}

#endif /* NO_SELECT */

int syslogproc(const char *cmd, char *argv[], const int option, const int facility, int stdOutLogLevel, int stdErrLogLevel) {
    int error = EXIT_SUCCESS;
    int stdoutPipe[2];
    int stderrPipe[2];
    int eventsPipe[2];

    //DEBUG_VAR(facility,"%d");
    //DEBUG_VAR(stdOutLogLevel,"%d");
    //DEBUG_VAR(stdErrLogLevel,"%d");

    error = pipe(stdoutPipe);
    if (0 == error) {
        //DEBUG_VAR(stdoutPipe[READ],"%d");
        //DEBUG_VAR(stdoutPipe[WRITE],"%d");
        error = pipe(stderrPipe);
        if (0 == error) {
            error = pipe(eventsPipe);
            if (0 == error) {
                //writeEventPipe = eventsPipe[WRITE];
                if (1/*signal(SIGCHLD,sigChildHandler) != SIG_ERR*/) {
                    const pid_t child = fork();
                    switch (child) {

                        case 0: /* child */
                            /* close unused read ends */
                            CLOSE_PIPE(stdoutPipe[READ]);
                            CLOSE_PIPE(stderrPipe[READ]);
                            /* close unused self pipe events ends */
                            CLOSE_PIPE(eventsPipe[READ]);
                            CLOSE_PIPE(eventsPipe[WRITE]);
                            error = execChild(cmd, argv, stdoutPipe[WRITE], stderrPipe[WRITE]);
                            DEBUG_VAR(error, "%d");
                            break;

                        case -1: /* error */
                            error = errno;
                            ERROR_MSG("fork error %d (%m)", error);
                            break;

                        default: /* parent */
                            /* close unused write ends */
                            DEBUG_VAR(child, "%u");
                            CLOSE_PIPE(stdoutPipe[WRITE]);
                            CLOSE_PIPE(stderrPipe[WRITE]);
                            openlogex(NULL, option, facility);
                            error = sendChildrenOutputsToSyslog(child, stdoutPipe[READ], stderrPipe[READ], eventsPipe[READ], stdOutLogLevel, stdErrLogLevel);
                            DEBUG_VAR(error, "%d");
                            closelogex();
                            break;
                    } /* switch(child) */
                } else {
                    ERROR_MSG("error setting signal SIGCHLD handler");
                }
                CLOSE_PIPE(eventsPipe[READ]);
                CLOSE_PIPE(eventsPipe[WRITE]);
            } else {
                error = errno;
                ERROR_MSG("pipe eventsPipe error %d (%m)", error);
            }
            CLOSE_PIPE(stderrPipe[READ]);
            CLOSE_PIPE(stderrPipe[WRITE]);
        } else { /*  error = pipe(stderrPipe); */
            error = errno;
            ERROR_MSG("pipe stderrPipe error %d (%m)", error);
        }

        CLOSE_PIPE(stdoutPipe[READ]);
        CLOSE_PIPE(stdoutPipe[WRITE]);
    } else { /*  error = pipe(stdoutPipe); */
        error = errno;
        ERROR_MSG("pipe stdoutPipe error %d (%m)", error);
    }

    return error;
}
#undef READ
#undef WRITE

#include <dbgflags/ModuleVersionInfo.h>
MODULE_NAME(syslogex);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(1.1);
MODULE_FILE_VERSION(1.5);
MODULE_DESCRIPTION(syslog extended);
MODULE_COPYRIGHT(LGPL);

