#ifndef _UDS_MANAGEMENT_H_
#define _UDS_MANAGEMENT_H_

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "protocol.h"

static __inline int makeRootDir(char *userRootDirName) {
    int error = EXIT_SUCCESS;
    const uid_t userID = getuid();

    sprintf(userRootDirName,"/tmp/dbgFlags_%d",userID);
    if (mkdir(userRootDirName,S_IRWXU|S_IRGRP|S_IXGRP) == -1) {
        if (errno != EEXIST) {
            error = errno;       
            ERROR_MSG("mkdir %s error %d (%m)",userRootDirName,error);
        }
    }

    return error;
}

static __inline void getPattern(const char *moduleName, char *pattern) {
    while( (*moduleName != '\0') && (*moduleName != '/') ) {
        *pattern = *moduleName;
        moduleName++;
        pattern++;
    }
    *pattern = '\0';
    DEBUG_VAR(pattern,"%s");
}

static __inline int getSocketNameEx(const uid_t userID,const char *moduleName,pid_t pid,comparator comp,char *socketName) {
    int error = EXIT_SUCCESS;
    char userRootDirName[PATH_MAX];    
    DIR *userRootDir = NULL;
    char pattern[PATH_MAX];

    socketName[0] = '\0';
    getPattern(moduleName,pattern);
    sprintf(userRootDirName,"/tmp/dbgFlags_%d",userID);
    userRootDir = opendir(userRootDirName);
    if (userRootDir != NULL) {
        struct dirent *directoryEntry = NULL;
        const size_t nameLenght = strlen(pattern);
        unsigned int nbMatch = 0;
        do
        {
            directoryEntry = readdir(userRootDir);
            if (directoryEntry != NULL) {
                const unsigned int match = (*comp)(directoryEntry->d_name,pattern);
                if (match != 0) {
                    /* directoryEntry->d_name match with the pattern parameter*/
                    const char *matchingDirEntName = strrchr(directoryEntry->d_name,'_'); /* socketName = modulename + '_' + PID */
                    if (matchingDirEntName != NULL) {
                        pid_t modulePid = 0;
                        matchingDirEntName++;
                        modulePid = atoi(matchingDirEntName);
                        if ((0 == pid) && (modulePid != 0)) { /* no pid parameter set and a matching name has been found */
                            char processName[PATH_MAX];
                            int getProcessNameError = 0;
                            DEBUG_VAR(modulePid,"%d");
                                                        
                            getProcessNameError = getFullProcessName(modulePid,processName); /* just to check if this entry is still valid */                            
                            if (EXIT_SUCCESS == getProcessNameError) {
                                sprintf(socketName,"%s/%s",userRootDirName,directoryEntry->d_name);
                                DEBUG_VAR(socketName,"%s");
                                nbMatch++;
                                DEBUG_VAR(nbMatch,"%d");
                            } else {
                                WARNING_MSG("process name not found for pid %d",modulePid);
                            }
                        } else if (pid == modulePid) { /* pid parameter set and a matching name has been found */
                            sprintf(socketName,"%s/%s",userRootDirName,directoryEntry->d_name);
                            DEBUG_VAR(socketName,"%s");
                            nbMatch++;
                            DEBUG_VAR(nbMatch,"%d");         
                        } else { /* (pid == 0) */
                            DEBUG_MSG("false positive: %s against %s but pid not found",directoryEntry->d_name,pattern);
                            DEBUG_VAR(directoryEntry->d_type,"%d");
                            if (DT_SOCK == directoryEntry->d_type) {
                                 if (unlink(directoryEntry->d_name) != 0) {
                                    const int unlink_error = errno;
                                    INFO_MSG("unlink %s error %d (%m)",directoryEntry->d_name,unlink_error);
                                }
                            } /* (DT_SOCK == directoryEntry->d_type) */
                        }
                    } else { /* (matchingDirEntName == NULL) */
                        DEBUG_MSG("false positive: %s against %s but character _ is not found",directoryEntry->d_name,pattern);
                    }
                } else { /* (match == 0)*/
                    DEBUG_MSG("%s not mach against %s",directoryEntry->d_name,pattern);
                }
            } /* (directoryEntry != NULL) */
        } while(directoryEntry != NULL);

        if (0 == nbMatch) {
            error = ENOENT; /* not found */
        } else if (nbMatch > 1) {
            error = EINVAL; /* can't set a uniq valid socket name using theses arguments */
            ERROR_MSG("can't set a uniq valid socket name using theses arguments, %d candidats found",nbMatch);
        }

        if (closedir(userRootDir) == -1 ) {
            error = errno;
            ERROR_MSG("closedir %s error %d (%m)",userRootDirName,error);
        }
        userRootDir = NULL;
    } else {
        error = errno;
        ERROR_MSG("opendir %s error %d (%m)",userRootDirName,error);
    }

    return error;
}

static __inline int getSocketName(const char *moduleName,pid_t pid,comparator comp,char *socketName) { /* deprecated */
    const uid_t userID = getuid();
    return getSocketNameEx(userID,moduleName,pid,comp,socketName);
}

#endif /* _UDS_MANAGEMENT_H_ */
