/* 
 * File:   ctxLogger.h
 * Author: oc
 *
 * Created on September 10, 2010, 1:05 AM
 */

#ifndef _CONTEXT_LOGGER_H_
#define	_CONTEXT_LOGGER_H_

#ifdef	__cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>

#ifndef CTX_LOGGER_MAX_LINE_SIZE
#define CTX_LOGGER_MAX_LINE_SIZE   1024
#endif /* CTX_LOGGER_MAX_LINE_SIZE */

#ifndef CTX_LOGGER_NB_LINE
#define CTX_LOGGER_NB_LINE   10
#endif /* CTX_LOGGER_NB_LINE */

extern char *ctxLogMemoryBuffer;
extern char *ctxLogWritePos;
extern pthread_mutex_t ctxCursorLock;

int ctxLogInit(void);
void ctxLogger(int priority, const char *format, ...); /* not "inline" function to be able to have a fct ptr */
void vctxLogger(int priority, const char *format,va_list optional_arguments);
void ctxLogUnInit(void);

#define ctxLog(priority,format, ...) { \
    const int lockError = pthread_mutex_lock(&ctxCursorLock); \
    if (EXIT_SUCCESS == lockError) { \
        char *writePosition = ctxLogWritePos; \
        const size_t memorySize = CTX_LOGGER_MAX_LINE_SIZE * CTX_LOGGER_NB_LINE; \
        ctxLogWritePos += CTX_LOGGER_MAX_LINE_SIZE; \
        if (ctxLogWritePos >= memorySize + ctxLogMemoryBuffer) { \
            ctxLogWritePos = ctxLogMemoryBuffer; \
        } \
        const int unlockError = pthread_mutex_unlock(&ctxCursorLock); \
        if (unlockError != EXIT_SUCCESS) { \
            ERROR_MSG("pthread_mutex_unlock ctxCursorLock error %d (%s)",unlockError,strerror(unlockError)); \
        } \
        snprintf(writePosition,CTX_LOGGER_MAX_LINE_SIZE,format DEBUG_EOL, ##__VA_ARGS__); \
        writePosition[CTX_LOGGER_MAX_LINE_SIZE-1] = '\0'; \
    } else { \
        ERROR_MSG("pthread_mutex_lock ctxCursorLock error %d (%s)",lockError,strerror(lockError)); \
    } \
} \

static inline void dumpCtxLog(void) { /* inline to be able to use the user program LOGGER macro*/
    const int lockError = pthread_mutex_lock(&ctxCursorLock);
    if (EXIT_SUCCESS == lockError) {
        int unlockError = EXIT_SUCCESS;

        const size_t memorySize = CTX_LOGGER_MAX_LINE_SIZE * CTX_LOGGER_NB_LINE;
        const char *limit = memorySize + ctxLogMemoryBuffer;
        const char *cursor = ctxLogWritePos; /* older trace */

        while(cursor < limit) {
            if (cursor[0] != '\0') {
                LOGGER(LOG_DEBUG,cursor);
            }
            cursor += CTX_LOGGER_MAX_LINE_SIZE;
        }

        cursor = ctxLogMemoryBuffer;
        while(cursor < ctxLogWritePos) {
            if (cursor[0] != '\0') {
                LOGGER(LOG_DEBUG,cursor);
            }
            cursor += CTX_LOGGER_MAX_LINE_SIZE;
        }

        unlockError = pthread_mutex_unlock(&ctxCursorLock);
        if (unlockError != EXIT_SUCCESS) {
            LOGGER(LOG_ERR,"pthread_mutex_unlock ctxCursorLock error (%d)",unlockError);
        }
    } else {
        LOGGER(LOG_ERR,"pthread_mutex_lock ctxCursorLock error %d (%s)",lockError,strerror(lockError));
    }
}

#ifdef	__cplusplus
}
#endif

#endif	/* _CONTEXT_LOGGER_H_ */

