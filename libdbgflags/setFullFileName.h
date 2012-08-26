#ifndef _SET_FULL_FILE_NAME_H_
#define _SET_FULL_FILE_NAME_H_

/*
 * moved from filesUtils.h (used by the fileLogger and aiofileLogger)
 * because of threadFileLogger which needs its own version
 */

static __inline int setFullFileName(void) {
    int error = EXIT_SUCCESS;

    if (LOG_FILE_ROTATE & LogStat) {
        static char *oldFileName = NULL;
        static char oldFileNameBuffer[PATH_MAX];
        if (unlikely(NULL == oldFileName)) {
            sprintf(oldFileNameBuffer, "%s/%s.bak", directory, processName);
            sprintf(fullFileName, "%s/%s.log", directory, processName);
            oldFileName = oldFileNameBuffer;
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
        time_t currentTime = time(NULL);
        if (unlikely(((time_t) - 1) == currentTime)) {
            currentTime = 0;
        }
        sprintf(fullFileName, "%s/%s_%u.log", directory, processName, currentTime);
    } else {
        sprintf(fullFileName, "%s/%s.log", directory, processName);
    }
    DEBUG_VAR(fullFileName, "%s");
    return error;
}

#endif /* _SET_FULL_FILE_NAME_H_ */