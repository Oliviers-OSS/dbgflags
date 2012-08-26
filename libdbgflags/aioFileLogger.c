/*
 * log file functions using POSIX AIO API
 */

#define _POSIX_C_SOURCE 199309L
#define __USE_UNIX98
#define DONT_USE_SIGNAL
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <aio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>

#define LOGGER syslogex
#define DEBUG_LOG_HEADER "aioFileLogger"

#include <dbgflags/syslogex.h>
#include <dbgflags/dbgflags.h>
#include <dbgflags/loggers.h>
#include <dbgflags/debug_macros.h>

#include "system.h"

#define MAX_LINE                    1024
#define NB_LINES_PER_BUFFERS        5
#define BUFFER_SIZE             (NB_LINES_PER_BUFFERS * MAX_LINE)
#define NB_BUFFERS                      5

typedef struct File_ {
    int fd;
    unsigned int nb_aio_pending;
    unsigned int nb_bytes;
} File;

typedef enum BufferStatus_ {
    Free
   ,Filling
   ,Filled
} BufferStatus;

typedef struct Buffer_ {
    unsigned char data[BUFFER_SIZE];
    BufferStatus status;
    unsigned int nb_bytes;
    struct aiocb *acb;
    //pthread_mutex_t mutex;
} Buffer;

typedef struct AIOData_ {
    struct aiocb cb[NB_BUFFERS];
    struct aiocb *first_used;
    struct aiocb *last_used;
    //pthread_mutex_t mutex;
} AIOData;

typedef struct Buffers_ {
    Buffer buffers[NB_BUFFERS];
    sem_t empty;
    sem_t full;
} Buffers;

static int LogStat = 0; /* status bits, set by openlog() */
static const char *LogTag = NULL; /* string to tag the entry with */
static int LogFacility = LOG_USER; /* default facility code */
static unsigned int MaxSize = (1024 * 100);
static char fullProcessName[PATH_MAX];
static char fullFileName[PATH_MAX];
static char directory[PATH_MAX];
static char *processName = NULL;
static char *fileDirectory = NULL;
static pthread_t aio_returns;
static File files[2];

static AIOData aioData;

static inline int initialize() {
    int error = EXIT_SUCCESS;
    File *f = NULL;
    Buffer *buffer = NULL;
    struct aiocb *pcb = aioData.cb;
    /*
    * files
    */
    
    for(f=files;f<=files+1;f++) {
        f->fd = -1;
        f->nb_aio_pending = 0;
        f->nb_bytes = 0;
    }

    /*
    * AIOData
    */
    aioData.first_used = NULL;
    aioData.last_used = NULL;

    for(buffer=buffers;buffer < buffers+NB_BUFFERS; buffer++) {
        memset(buffer->data,0,BUFFER_SIZE);
        buffer->status = Free;
        buffer->nb_bytes = 0;
        buffer->acb = pcb++;
    }
    return error;
}

static Buffer *find_free_buf(void)
{
    Buffer *freeBuffer = NULL;
    Buffer *buffer = buffers;

    /* look for an already available buffer */
    for(buffer=buffers;buffer < buffers+NB_BUFFERS; buffer++) {
        if (Free == buffer->status) {
            freeBuffer = buffer;
            break;
        }
    }

    /* if not found */
    if (freeBuffer != NULL) {
        
    }
  

    
    /*if (NULL == freeBuffer) {
        freeBuffer = wait_for_free_buffer();
    }*/
    return freeBuffer;
}

static void * aio_ends_mgr(void *parameter) {
    return NULL;
}

void aioOpenLogFile(const char *ident, int logstat, int logfac) {
    if (ident != NULL) {
        LogTag = ident;
    }
    LogStat = logstat;
    if (logfac != 0 && (logfac &~LOG_FACMASK) == 0) {
        LogFacility = logfac;
    }
}

void aiovFileLogger(int priority, const char *format, va_list optional_arguments) {
    int n = 0;
    int error = EXIT_SUCCESS;
    int internalError = EXIT_SUCCESS;
    const int LogMask = setlogmask(0);

    /* Check priority against setlogmask values. */
    if ((LOG_MASK(LOG_PRI(priority)) & LogMask) != 0) {
        va_list optional_arguments;
        char logMsg[2048];
        char logFormat[1024];
        char *cursor = logFormat;
        struct tm now_tm;
        time_t now;

        (void) time(&now);
        cursor += strftime(cursor, sizeof (logFormat), "%h %e %T ", localtime_r(&now, &now_tm));

        if (LogTag) {
            cursor += sprintf(cursor, "%s: ", LogTag);
        }

        if (LogStat & LOG_PID) {
            if (LogStat & LOG_TID) {
                const pid_t tid = gettid();
                n = sprintf(cursor, "[%d:%d]", (int) getpid(), (int) tid);
            } else {
                n = sprintf(cursor, "[%d]", (int) getpid());
            }
            cursor += n;
        }

        if (LogStat & LOG_RDTSC) {
            const unsigned long long int t = rdtsc();
            cursor += sprintf(cursor, "(%llu)", t);
        } /* (LogStat & LOG_CLOCK) */

        if (LogStat & LOG_LEVEL) {
            switch (LOG_PRI(priority)) {
                case LOG_EMERG:
                    n = sprintf(cursor, "* Emergency * %s", format);
                    break;
                case LOG_ALERT:
                    n = sprintf(cursor, "* Alert * %s", format);
                    break;
                case LOG_CRIT:
                    n = sprintf(cursor, "* Critical * %s", format);
                    break;
                case LOG_ERR:
                    n = sprintf(cursor, "* Error * %s", format);
                    break;
                case LOG_WARNING:
                    n = sprintf(cursor, "* Warning * %s", format);
                    break;
                case LOG_NOTICE:
                    n = sprintf(cursor, "* Notice * %s", format);
                    break;
                case LOG_INFO:
                    n = sprintf(cursor, "* Info * %s", format);
                    break;
                case LOG_DEBUG:
                    n = sprintf(cursor, "* Debug * %s", format);
                    break;
                default:
                    n = sprintf(cursor, "* <%d> * %s", priority, format);
            } /* switch(priority) */
        } /* (LogStat & LOG_LEVEL) */

        n = vsprintf(logMsg, logFormat, optional_arguments);
#if 0
        error = pthread_mutex_lock(&fileLock);
        if (likely(EXIT_SUCCESS == error)) {

            if (unlikely(-1 == logFile)) {
                error = createFile();
                /* error already logged */
            }

            if (likely(EXIT_SUCCESS == error)) {
                ssize_t written = write(logFile, logMsg, n);
                if (written > 0) {
                    if (unlikely(written != n)) {
                        ERROR_MSG("only %d byte(s) of %d has been written to %s", written, n, fullFileName);
                    }
                    fileSize += written;

                    if ((LOG_FILE_ROTATE & LogStat) || (LOG_FILE_HISTO & LogStat)) {
                        if (fileSize >= MaxSize) {
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
            }

            internalError = pthread_mutex_unlock(&fileLock);
            if (internalError != EXIT_SUCCESS) {
                ERROR_MSG("pthread_mutex_lock fileLock error %d (%s)", internalError, strerror(internalError));
                if (EXIT_SUCCESS == error) {
                    error = internalError;
                }
            }
#endif
        } else {
            ERROR_MSG("pthread_mutex_lock fileLock error %d (%s)", error, strerror(error));
        }
    }

    /*return n;*/
}

void aioFileLogger(int priority, const char *format, ...) {
    va_list optional_arguments;
    va_start(optional_arguments, format);
    aiovFileLogger(priority, format,  optional_arguments);
    va_end(optional_arguments);
}

int aioSetFileLoggerMaxSize(unsigned int maxSizeInBytes) {
    if (maxSizeInBytes != 0) {
        MaxSize = maxSizeInBytes;
    }
    return MaxSize;
}

static inline int checkNsetLoggerDirectory(const char *logfileDirectory) {
    int error = EXIT_SUCCESS;
    struct stat dirStat;
    if (lstat(logfileDirectory,&dirStat) == 0) {
        if (S_ISDIR(dirStat.st_mode)) {
            strcpy(directory, logfileDirectory);
            DEBUG_VAR(directory, "%s");
            fileDirectory = directory;
        } else {
            error = EINVAL;
            ERROR_MSG("%s os not a directory (st_mode = 0x%X)",dirStat.st_mode);
        }
    } else {
        error = errno;
        ERROR_MSG("lstat %s error %d (%m)",logfileDirectory,error);
    }
    return error;
}

int setFileLoggerDirectory(const char *logfileDirectory) {
    int error = EINVAL;

    if (logfileDirectory != NULL) {
        if (logfileDirectory[0] != '$') {
            error = checkDirectory(logfileDirectory);
        } else {
            char *envVar = getenv(logfileDirectory + 1);
            if (envVar != NULL) {
                error = checkDirectory(logfileDirectory);
            } else {
                error = ENOENT;
                ERROR_MSG("environment variable %s is not found", logfileDirectory);
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

void aioCloseLogFile(void) {
    
}








#if 0
#ifdef _DEBUGFLAGS_H_
static DebugFlags debugFlags = {
    "fileLogger",
    {
        "aioFile"
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
        , ""
        , ""
    }
    , 0xFFFFFFFF
};
#define FLAG_AIOFILE  FLAG_1

#else /* _DEBUGFLAGS_H_ */
#define FILTER
#define FLAG_FILE
#endif /* _DEBUGFLAGS_H_ */

#define MODULE_FLAG FLAG_AIOFILE

#include <dbgflags/debug_macros.h>


#ifndef AIO_LISTIO_MAX
#define AIO_LISTIO_MAX _SC_AIO_LISTIO_MAX
#endif

#define SIG_AIO_BUFFER_WRITE    (SIGRTMAX-10)
#define SIG_LISTIO_DONE         (SIGRTMAX-9)

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif /* MIN */

#define BUFFER_FREE 1
#define BUFFER_FILLING  2
#define BUFFER_WRITTING 3

#define NB_BUFFERS 10
#define BUFFER_SIZE (1024)
#define NB_FD  2

typedef struct {
    int fd;
    unsigned int size;
    unsigned int nb_running_aiocb;
} file_t;

typedef struct {
    int state; /* Free,filling or writting (i.e. enqueued for write) */
    int used_size; 
    struct aiocb acb;
    char buffer[BUFFER_SIZE]; /* Data Buffer */
    file_t *file;
} buffer_t;

static void init_buffer(buffer_t *buffer, file_t file) {
    
}

struct aiocb *acb_list[AIO_LISTIO_MAX];

/* buffers log output */
static buffer_t buffersList[NB_BUFFERS];
static buffer_t *currentBuffer = NULL;
static file_t files[NB_FD];
static file_t *currentFile = NULL;
static int current_log_fd = 0;
static off_t seek_ptr = 0;
static pthread_mutex_t alogger_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t fileLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP; /* operation on file in progress */
static int LogStat = 0; /* status bits, set by openlog() */
static const char *LogTag = NULL; /* string to tag the entry with */
static int LogFacility = LOG_USER; /* default facility code */
static int LogMask = 0xff; /* mask of priorities to be logged */
static unsigned int MaxSize = (1024 * 100);
static char fullProcessName[PATH_MAX];
static char fullFileName[PATH_MAX];
static char directory[PATH_MAX];
static char *processName = NULL;
static char *fileDirectory = NULL;
//static unsigned int fileSize = 0;
//static unsigned int writtenFileSize = 0;

static buffer_t *find_free_buf(void);
static void flush_filled_buf(buffer_t *);
static void flush_several_bufs(buffer_t *bufs_to_flush, int nbufs);
static void aio_done(int, siginfo_t *, void *);
static void aio_closelog(void);
static int log_data(char*, int);
static void flush_data(void);
void initlog(const char*);

#include "filesUtils.h"
#include "setFullFileName.h"

static buffer_t *wait_for_free_buffer(void) {
    int i;
    const struct aiocb * p[NB_BUFFERS];
    buffer_t *freeBuffer = NULL;

    for (i = 0; i < NB_BUFFERS; i++) {
        p[i] = &buffersList[i].acb;
    }

    do {
        int error = aio_suspend(p, NB_BUFFERS, NULL);
        if (0 == error) {
            /* look for the ended aio write */
            for (i = 0; i < NB_BUFFERS; i++) {
                if (BUFFER_WRITTING == buffersList[i].state) {
                    error = aio_error(&buffersList[i].acb);
                    if (error != EINPROGRESS) {
                        error = aio_return(&buffersList[i].acb);
                        if (error < 0) {
                            ERROR_MSG("aio write ended with error %d (%s)", error, strerror(error));
                        }
                        buffersList[i].state = BUFFER_FREE;
                        freeBuffer = &buffersList[i];
                    }
                } /* (BUFFER_WRITTING == buflist[i].state) */
            } /* for(i=0;i<NBUFFERS;i++) */
        } else if (-1 == error) {
            INFO_MSG("aio_suspend interrupted by a signal");
        } else {
            ERROR_MSG("aio_suspend error %d (%s)", error, strerror(error));
        }
    } while (NULL == freeBuffer);
    return freeBuffer;
}

static buffer_t *find_free_buf(void) {
    int i;
    buffer_t *freeBuffer = NULL;

    /* look for an already available buffer */
    for (i = 0; ((i < NB_BUFFERS) && (NULL == freeBuffer)); i++) {
        if (BUFFER_FREE == buffersList[i].state) {
            freeBuffer = &buffersList[i];
        }
    }

    /* if not found */
    if (NULL == freeBuffer) {
        freeBuffer = wait_for_free_buffer();
    }

    DEBUG_VAR(freeBuffer, "%X");
    return freeBuffer;
}

static void flush_filled_buf(buffer_t *full_guy,file_t *file) {
    /* Set up AIOCB */
    full_guy->acb.aio_fildes = log_fd;
    full_guy->acb.aio_offset = seek_ptr;
    seek_ptr += BUFFER_SIZE;
    full_guy->acb.aio_buf = full_guy->buffer;
    full_guy->acb.aio_nbytes = BUFFER_SIZE;
    full_guy->acb.aio_reqprio = 0;
    full_guy->acb.aio_sigevent.sigev_notify = SIGEV_NONE;
    full_guy->acb.aio_sigevent.sigev_signo = 0;
    full_guy->acb.aio_sigevent.sigev_value.sival_ptr = NULL;

    /* Mark buffer as being written out */
    full_guy->state = BUFFER_WRITTING;

    /* Fire off the asynchronous I/O */
    if (aio_write(&full_guy->acb) < 0) {
        const int error = aio_error(&full_guy->acb);
        ERROR_MSG("aio_write error %d (%s)", error, strerror(error));
    }
}

static void flush_data(void) {
    const struct aiocb * p[1];

    /* Set up AIOCB */
    currentBuffer->acb.aio_fildes = currentFile->fd;
    __sync_fetch_and_add(&currentFile->nb_running_aiocb,1); /* GCC's atomic add */
    currentBuffer->acb.aio_offset = seek_ptr;
    seek_ptr += currentBuffer->used_size;
    currentBuffer->acb.aio_buf = currentBuffer->buffer;
    currentBuffer->acb.aio_nbytes = currentBuffer->used_size;
    currentBuffer->acb.aio_reqprio = 0;
    currentBuffer->acb.aio_sigevent.sigev_notify = SIGEV_NONE;
    currentBuffer->acb.aio_sigevent.sigev_signo = 0;
    currentBuffer->acb.aio_sigevent.sigev_value.sival_ptr = NULL;

    /* Mark buffer as being written out */
    currentBuffer->state = BUFFER_WRITTING;
    DEBUG_MSG("flushing remaining data...");
    /* Fire off the asynchronous I/O */
    if (aio_write(&currentBuffer->acb) < 0) {
        const int error = aio_error(&currentBuffer->acb);
        ERROR_MSG("flush aio_write error %d (%s)", error, strerror(error));
    }

    p[0] = &currentBuffer->acb;
    do {
        int error = aio_suspend(p, 1, NULL);
        if (0 == error) {
            error = aio_error(p[0]);
            if (error != EINPROGRESS) {
                if ((error = aio_return(p[0])) != BUFFER_SIZE) {
                    ERROR_MSG("aio write ended with error %d (%s)", error, strerror(error));
                }
                currentBuffer->state = BUFFER_FREE;
            }
        } else if (error != -1) {
            ERROR_MSG("flush_data aio_suspend error %d (%s)", error, strerror(error));
        }
    } while (BUFFER_WRITTING == currentBuffer->state);

    /*
     * this is the last buffer: close the file
     */
    aio_closelog();
}

static void flush_several_bufs(buffer_t *bufs_to_flush, int nbufs) {
    struct sigevent when_all_done;

    /* set up AIOCBs */
    int num_in_listio = 0;
    for(num_in_listio = 0; num_in_listio < nbufs;num_in_listio++) {
        acb_list[num_in_listio] = &bufs_to_flush[num_in_listio].acb;
        acb_list[num_in_listio]->aio_lio_opcode = LIO_WRITE;
        acb_list[num_in_listio]->aio_fildes = log_fd;
        //...
    }
    when_all_done.sigev_notify = SIGEV_SIGNAL;
    when_all_done.sigev_signo = SIG_LISTIO_DONE;
    when_all_done.sigev_value.sival_ptr = (void*)acb_list;

    /* Fire off the asynchronous I/Os! */
    
    if (lio_listio(LIO_NOWAIT,acb_list,num_in_listio,&when_all_done) < 0) {
        ERROR_MSG("lio_listio");
    }
}

/* log some data */
static inline int log_data(char *logdata, int nbytes) {
    int nlogged = 0;
    int num_to_copy = 0;

    int error = pthread_mutex_lock(&alogger_lock);
    if (0 == error) {
        while (nlogged < nbytes) {
            num_to_copy = MIN(BUFFER_SIZE - currentBuffer->used_size, nbytes - nlogged);
            memcpy(&currentBuffer->buffer[currentBuffer->used_size], logdata + nlogged, num_to_copy);
            currentBuffer->used_size += num_to_copy;
            nlogged += num_to_copy;
            DEBUG_MSG("curbuf->fillpt = %d", currentBuffer->used_size);
            if (BUFFER_SIZE == currentBuffer->used_size) {
                /* buffer full, flush and get a new one */
                DEBUG_MSG("buffer 0x%X full, flush and get a new one", currentBuffer);
                flush_filled_buf(currentBuffer);
                DEBUG_MSG("buffer flushed");
                currentBuffer = find_free_buf();
                DEBUG_MSG("new buffer 0x%X", currentBuffer);
            }
        }
        error = pthread_mutex_unlock(&alogger_lock);
        if (error != 0) {
            ERROR_MSG("pthread_mutex_unlock alogger_lock error %d", error);
        }
    } else {
        ERROR_MSG("pthread_mutex_lock alogger_lock error %d", error);
    }
    return error;
}

void aOpenLogFile(const char *ident, int logstat, int logfac) {
    int i;
    struct sigaction sa;

    for (i = 0; i < NB_BUFFERS; i++) {
        buffersList[i].state = BUFFER_FREE;
    }
    
    for (i = 0; i < NB_FD; i++) {
        files[i].fd = -1;
        files[i].nb_running_aiocb = 0;
        files[i].size = 0;
    }

    currentBuffer = find_free_buf();
    currentFile = NULL;

    if (ident != NULL) {
        LogTag = ident;
    }

    LogStat = logstat;
    
    if (logfac != 0 && (logfac &~LOG_FACMASK) == 0) {
        LogFacility = logfac;
    }   
}

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

        if (log_fd[current_log_fd] != -1) {
            WARNING_MSG("fd was NOT NULL");
            if (close(log_fd[current_log_fd]) != 0) {
                internalError = errno;
                ERROR_MSG("close %d error %d (%m)", log_fd[current_log_fd], internalError);
            }
            log_fd[current_log_fd] = -1;
        }

        if (unlikely(NULL == processName)) {
            setProcessName();
        }
        setFullFileName();

        log_fd[current_log_fd] = open(fullFileName, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP);
        if (likely(log_fd[current_log_fd] != -1)) {
            fileSize = 0;
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

void aFileLogger(int priority, const char *format, ...) {
    int n = 0;
    int error = EXIT_SUCCESS;
    int internalError = EXIT_SUCCESS;
    const int LogMask = setlogmask(0);

    /* Check priority against setlogmask values. */
    if ((LOG_MASK(LOG_PRI(priority)) & LogMask) != 0) {
        va_list optional_arguments;
        char logMsg[2048];
        char logFormat[1024];
        char *cursor = logFormat;
        struct tm now_tm;
        time_t now;

        (void) time(&now);
        cursor += strftime(cursor, sizeof (logFormat), "%h %e %T ", localtime_r(&now, &now_tm));

        if (LogTag) {
            cursor += sprintf(cursor, "%s: ", LogTag);
        }

        if (LogStat & LOG_PID) {
            if (LogStat & LOG_TID) {
                const pid_t tid = gettid();
                n = sprintf(cursor, "[%d:%d]", (int) getpid(), (int) tid);
            } else {
                n = sprintf(cursor, "[%d]", (int) getpid());
            }
            cursor += n;
        }

        if (LogStat & LOG_RDTSC) {
            const unsigned long long int t = rdtsc();
            cursor += sprintf(cursor, "(%llu)", t);
        } /* (LogStat & LOG_CLOCK) */

        va_start(optional_arguments, format);
        switch (LOG_PRI(priority)) {
            case LOG_EMERG:
                n = sprintf(cursor, "* Emergency * %s", format);
                break;
            case LOG_ALERT:
                n = sprintf(cursor, "* Alert * %s", format);
                break;
            case LOG_CRIT:
                n = sprintf(cursor, "* Critical * %s", format);
                break;
            case LOG_ERR:
                n = sprintf(cursor, "* Error * %s", format);
                break;
            case LOG_WARNING:
                n = sprintf(cursor, "* Warning * %s", format);
                break;
            case LOG_NOTICE:
                n = sprintf(cursor, "* Notice * %s", format);
                break;
            case LOG_INFO:
                n = sprintf(cursor, "* Info * %s", format);
                break;
            case LOG_DEBUG:
                n = sprintf(cursor, "* Debug * %s", format);
                break;
            default:
                n = sprintf(cursor, "* <%d> * %s", priority, format);
        } /* switch(priority) */

        n = vsprintf(logMsg, logFormat, optional_arguments);

        error = pthread_mutex_lock(&fileLock);
        if (likely(EXIT_SUCCESS == error)) {

            if (unlikely(-1 == log_fd[current_log_fd])) {
                error = createFile();
                /* error already logged */
            }

            if (likely(EXIT_SUCCESS == error)) {
                ssize_t written = write(logFile, logMsg, n);
                if (written > 0) {
                    if (unlikely(written != n)) {
                        ERROR_MSG("only %d byte(s) of %d has been written to %s", written, n, fullFileName);
                    }
                    fileSize += written;

                    if ((LOG_FILE_ROTATE & LogStat) || (LOG_FILE_HISTO & LogStat)) {
                        if (fileSize >= MaxSize) {
                            close(logFile);
                            logFile = -1;
                        }
                    }
                } else if (0 == written) {
                    ERROR_MSG("nothing has been written in %s", fullFileName);
                } else {
                    error = errno;
                    ERROR_MSG("write to %s error %d (%m)", fullFileName, error);
                }
            }

            internalError = pthread_mutex_unlock(&fileLock);
            if (internalError != EXIT_SUCCESS) {
                ERROR_MSG("pthread_mutex_lock fileLock error %d (%s)", internalError, strerror(internalError));
                if (EXIT_SUCCESS == error) {
                    error = internalError;
                }
            }
        } else {
            ERROR_MSG("pthread_mutex_lock fileLock error %d (%s)", error, strerror(error));
        }
        va_end(optional_arguments);
    }
}

int aSetFileLoggerMaxSize(unsigned int maxSizeInBytes) {
    if (maxSizeInBytes != 0) {
        MaxSize = maxSizeInBytes;
    }
    return MaxSize;
}

int aSetFileLoggerDirectory(const char *logfileDirectory) {
    int error = EINVAL;

    if (logfileDirectory != NULL) {
        if (logfileDirectory[0] != '$') {
            strcpy(directory, logfileDirectory);
            DEBUG_VAR(directory, "%s");
            fileDirectory = directory;
        } else {
            char *envVar = getenv(logfileDirectory + 1);
            if (envVar != NULL) {
                strcpy(directory, envVar);
                DEBUG_VAR(directory, "%s");
                fileDirectory = directory;
            } else {
                ERROR_MSG("environment variable %s is not found", logfileDirectory);
                error = ENOENT;
            }
        }
    }

    return error;
}

void aCloseLogFile(void) {
    const int error = close(log_fd);
    if (0 == error) {
        log_fd = 0;
    } else {
        perror("close");
    }
}

#endif


#include <dbgflags/ModuleVersionInfo.h>
MODULE_NAME(AIOFileLogger);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(1.0);
MODULE_FILE_VERSION(0.1);
MODULE_DESCRIPTION(logger to files with the same signature than syslog using POSIX AIO API);
MODULE_COPYRIGHT(LGPL);
