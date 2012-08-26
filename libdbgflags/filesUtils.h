#ifndef _FILES_UTILS_H_
#define _FILES_UTILS_H_

static __inline void setDirectory(char *processNamePosition) {
    if (processNamePosition != NULL) {
        *processNamePosition = '\0';
        strcpy(directory, fullProcessName);
        *processNamePosition = '/';
    } else {
        char *homeDir = getenv("HOME");
        if (homeDir != NULL) {
            strcpy(directory, homeDir);
        } else {
            strcpy(directory, "/tmp");
        }
    }
    DEBUG_VAR(directory, "%s");
    fileDirectory = directory;
}

static __inline void setProcessName(void) {
    int error = getCurrentFullProcessName(fullProcessName);
    if (likely(EXIT_SUCCESS == error)) {
        char *start = strrchr(fullProcessName, '/');
        DEBUG_VAR(fullProcessName, "%s");
        if (unlikely(NULL == fileDirectory)) {
            setDirectory(start);
        } else {
            setDirectory(NULL);
        }

        if (start != NULL) {
            processName = start + 1;
        } else {
            processName = fullProcessName;
        }
    } else { /* EXIT_SUCCESS != getCurrentFullProcessName */
        const pid_t pid = getpid();
        ERROR_MSG("getCurrentFullProcessName error %d (%m)", error);
        setDirectory(NULL);
        sprintf(fullProcessName, "process_%u", pid);
        processName = fullProcessName;
    }
    DEBUG_VAR(processName, "%s");
}

static __inline int isADirectory(const char *directory) {
   int error = EXIT_SUCCESS;
   struct stat dirStat;
   if (lstat(directory,&dirStat) == 0) {
      if (!S_ISDIR(dirStat.st_mode)) {
         error = EINVAL;
         NOTICE_MSG("%s is not a directory (st_mode = 0x%X)",directory,dirStat.st_mode);
      }
   } else {
      error = errno;
      ERROR_MSG("lstat %s error %d (%m)",directory,error);
   }
   return error;
}

static __inline int checkDirectory(const char *directory) {
    int error = EXIT_SUCCESS;
    struct stat dirStat;
    if (stat(directory,&dirStat) == 0) {
        if (!S_ISDIR(dirStat.st_mode)) {
            error = EINVAL;
            NOTICE_MSG("%s is not a directory (st_mode = 0x%X)",directory,dirStat.st_mode);
        }
    } else {
        error = errno;
        ERROR_MSG("stat %s error %d (%m)",directory,error);
    }
    return error;
}

#endif /* _FILES_UTILS_H_ */
