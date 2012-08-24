#include "config.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <dbgflags/syslogex.h>
#include <dbgflags/dbgflags.h>
#include <dbgflags/loggers.h>
#include <dbgflags/ctxLogger.h>

#define DEBUG_LOG_HEADER "ctxLogger"
#undef FILTER
#define FILTER
/*#define LOGGER syslogex
#define LOG_OPTS  0*/
#include <dbgflags/debug_macros.h>

char *ctxLogMemoryBuffer = NULL;
char *ctxLogWritePos = NULL;
pthread_mutex_t ctxCursorLock  = PTHREAD_MUTEX_INITIALIZER;

int ctxLogInit(void) {
    int error = EXIT_SUCCESS;

    error = pthread_mutex_lock(&ctxCursorLock);
    if (EXIT_SUCCESS == error) {
        const size_t memorySize = CTX_LOGGER_MAX_LINE_SIZE * CTX_LOGGER_NB_LINE;
        int unlockError;
        if (NULL == ctxLogMemoryBuffer) {
            ctxLogMemoryBuffer = (char *)malloc(memorySize);
            if (ctxLogMemoryBuffer != NULL) {
                ctxLogWritePos = ctxLogMemoryBuffer;
                memset(ctxLogMemoryBuffer,0,memorySize);
                atexit(ctxLogUnInit);
            } else {
                error = ENOMEM;
                ERROR_MSG("failed to allocate %u bytes of memory",memorySize);
            }
        } else {
            error = EINVAL;
            ERROR_MSG("memory buffer is already allocated");
        }
        unlockError = pthread_mutex_unlock(&ctxCursorLock);
        if (unlockError != 0) {
            ERROR_MSG("pthread_mutex_unlock ctxCursorLock error %d (%s)",unlockError,strerror(unlockError));
        }
    } else {
        ERROR_MSG("pthread_mutex_lock memLogLock error %d (%s)",error,strerror(error));
    }

    return error;
}

void ctxLogUnInit(void) {
    int error = pthread_mutex_lock(&ctxCursorLock);
    if (EXIT_SUCCESS == error) {
	if (ctxLogMemoryBuffer != NULL) {
           free(ctxLogMemoryBuffer);
           ctxLogWritePos = ctxLogMemoryBuffer = NULL;
	}
        error = pthread_mutex_unlock(&ctxCursorLock);
        if (error != 0) {
            ERROR_MSG("pthread_mutex_unlock ctxCursorLock error %d (%s)",error,strerror(error));
        }
    } else {
        ERROR_MSG("pthread_mutex_lock memLogLock error %d (%s)",error,strerror(error));
    }
}

static inline void append(char *string, const char* s) {
   char *writeCursor = string;
   const char *readCursor = s;

   while(*writeCursor != '\0') {
      *writeCursor = *readCursor;
      readCursor++;
      writeCursor++;
   }
   *writeCursor = '\0';
}

void ctxLogger(int priority, const char *format, ...) {
    int n = 0;
    const int lockError = pthread_mutex_lock(&ctxCursorLock); 
    if (EXIT_SUCCESS == lockError) { 
        va_list optional_arguments;
        char *writePosition = ctxLogWritePos; 
        const size_t memorySize = CTX_LOGGER_MAX_LINE_SIZE * CTX_LOGGER_NB_LINE; 
        ctxLogWritePos += CTX_LOGGER_MAX_LINE_SIZE; 
        if (ctxLogWritePos >= memorySize + ctxLogMemoryBuffer) { 
            ctxLogWritePos = ctxLogMemoryBuffer; 
        } 
        const int unlockError = pthread_mutex_unlock(&ctxCursorLock); 
        if (unlockError != EXIT_SUCCESS) { 
            ERROR_MSG("pthread_mutex_unlock ctxCursorLock error %d (%s)",unlockError,strerror(unlockError));
        } 
        va_start(optional_arguments, format);
        n = vsnprintf(writePosition,CTX_LOGGER_MAX_LINE_SIZE,format, optional_arguments) + strlen(DEBUG_EOL); 
        append(writePosition + n,DEBUG_EOL);
        writePosition[CTX_LOGGER_MAX_LINE_SIZE-1] = '\0'; 
        va_end(optional_arguments);
    } else { 
        ERROR_MSG("pthread_mutex_lock ctxCursorLock error %d (%s)",lockError,strerror(lockError));
    }
    /*return n;*/
}

#include "ModuleVersionInfo.h"
MODULE_NAME(ctxLogger);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(1.0);
MODULE_FILE_VERSION(1.0);
MODULE_DESCRIPTION(context logger);
MODULE_COPYRIGHT(LGPL);

