/***************************************************************************
ProcDebugUDS.cpp  -  description
-------------------
begin                : Fri May 14 2010
copyright            : (C) 2010 by OC

***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "config.h"
#define __USE_GNU 1
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include <netdb.h>

static void *UDSTCPServer(void* parameter);
static void FreeAllocatedResources(void);

#include <dbgflags/dbgflags.h>
#include "debug.h"
//#define ENTER  { DEBUG_MSG("+%s",__FUNCTION__); }
//#define EXIT  { DEBUG_MSG("-%s",__FUNCTION__); }

#include "utils.h"
#include "ProcDebugFlagsUDSEntry.h"
#include "networkUtils.h"
#include "UdsServerManagement.h"
#include "LibrariesNameBuffer.h"
#include "RemoteStatus.h"
/*
* comparators
*/

unsigned int equal(const char* name, const char *pattern) {
    return (strcmp(name,pattern) == 0);
}

unsigned int begin(const char* name, const char *pattern) {
    const size_t nameLength = strlen(name);
    const size_t patternLenght = strlen(pattern);
    if (nameLength < patternLenght) {
        return 0;
    } else {
        return (strncmp(name,pattern,patternLenght) == 0);
    }
}

unsigned int end(const char* name, const char *pattern) {
    const size_t nameLength = strlen(name);
    const size_t patternLenght = strlen(pattern);
    if (nameLength < patternLenght) {
        return 0;
    } else {
        const size_t cmpBeginOffset = nameLength - patternLenght;
        const char *cmpString = name + cmpBeginOffset;
        return (strcmp(cmpString,pattern) == 0);
    }
}

unsigned int contains(const char* name, const char *pattern) {
    const char* p = strstr(name,pattern);
    return (p != NULL);
}


static __inline Comparators comparatorToComparator(const comparator fct) {
    Comparators c;

    if (contains == fct) {
        c = eContains;
    } else if (equal == fct) {
        c = eEqual;
    } else if (begin == fct) {
        c = eBegin;
    } else if (end == fct) {
        c = eEnd;
    } else {
        c = eError;
    }
}

static __inline comparator comparatorFunction(const Comparators c) {
    comparator fct = NULL;
    switch(c) {
        case eContains:
            fct = contains;
            break;
        case eEqual:
            fct = equal;
            break;
        case eBegin:
            fct = begin;
            break;
        case eEnd:
            fct = end;
            break;
            /* no default */
    } /* switch(c)*/

    return fct;
}

/*
*
*/

void FreeAllocatedResources(void) {
    EndUDSServer(&g_dbgFlags);
    pthread_mutex_lock(&g_dbgFlags.mutex);
    LibraryDebugFlagsEntryClear(g_dbgFlags.libraries);
    g_dbgFlags.libraries = NULL;
    pthread_mutex_unlock(&g_dbgFlags.mutex);
    removeUDSEntry();
}

static __inline int sendDebugFlags(int connected_socket,DbgFlagsCommand *dbgFlagsCommand) {
    int error = EXIT_SUCCESS;

    /*
    * read the parameters
    */
    error = receiveBuffer(connected_socket,&dbgFlagsCommand->param.getCmd,sizeof(ClientParametersGetCmd),NULL);
    if (EXIT_SUCCESS == error) {
        interfaceVersion version = 1.0;
        DebugFlags *dbgFlags = findDebugFlags(dbgFlagsCommand->param.getCmd.moduleName,&version);
        DEBUG_VAR(dbgFlagsCommand->param.getCmd.moduleName,"%s");
        DEBUG_VAR(dbgFlags,"0x%X");
        if (dbgFlags != NULL) {
            FulldebugFlags fullDbgFlags;

            memcpy(&fullDbgFlags.dbgFlags,dbgFlags,sizeof(DebugFlags));
            fullDbgFlags.syslogMask = setlogmask(0);

            error = sendBuffer(connected_socket,&fullDbgFlags,sizeof(FulldebugFlags),NULL);
            if (error != EXIT_SUCCESS) {
                ERROR_MSG("sendDebugFlags send error %d (%m)",error);
            }
        } else { /* module not found */
            NOTICE_MSG("module %s not found",dbgFlagsCommand->param.getCmd.moduleName);
            error = ENOENT;
        }
    } else {
        ERROR_MSG("receiveBuffer ClientParametersGetCmd error %d (%m)",error);
    }

    return error;
}

static int addName(const DebugFlags *library,void *userParam) {
    int error = EXIT_SUCCESS;
    LibrariesNameBuffer *namesBuffer = (LibrariesNameBuffer *)userParam;

    if (library != NULL) {
        if (userParam != NULL) {
            error = LibrariesNameBufferAdd(namesBuffer,library->moduleName);
        } else {
            error = EINVAL;
            ERROR_MSG("userParam parameter is NULL");
        }
    } else {
        error = EINVAL;
        ERROR_MSG("library parameter is NULL");
    }

    return error;
}

static __inline int sendLibrariesNames(int connected_socket,DbgFlagsCommand *dbgFlagsCommand) {
    int error = EXIT_SUCCESS;

    /*
    * read the parameters
    */
    /*error = receiveBuffer(connected_socket,&dbgFlagsCommand->lsLibCmd,sizeof(ClientParametersLsLibs),NULL);
    if (EXIT_SUCCESS == error) {*/
        LibraryDebugFlagsEntry *entries = g_dbgFlags.libraries;
        if (entries != NULL) {
            LibrariesNameBuffer namesBuffer;        
            error = LibrariesNameBufferInit(&namesBuffer);
            if (EXIT_SUCCESS == error) {
                error = LibraryDebugFlagsEntryForEach(entries,addName,&namesBuffer);
                if (EXIT_SUCCESS == error) {
                    LibrariesNameBufferAddChar(&namesBuffer,'\0'); /* add an extra null character to the end of the list to signal its end */
                    error = sendBuffer(connected_socket,namesBuffer.buffer,namesBuffer.usedSize,NULL);
                    if (error != EXIT_SUCCESS) {
                        ERROR_MSG("sendLibrariesNames send error %d (%m)",error);
                    }
                } else {
                    ERROR_MSG("LibraryDebugFlagsEntryForEach error %d",error);
                }
            }
            /* error already printed */
        } else { /* no libraries found */
            NOTICE_MSG("no libraries found");
            error = ENOENT;
        }
    /*} else {
        ERROR_MSG("receiveBuffer ClientParametersLsLibs error (%m)",error);
    }*/    

    return error;
}

static __inline int receiveNewFlagsValues(int connected_socket,DbgFlagsCommand *dbgFlagsCommand) {
    /*
    * read the parameters
    */
    int error =  receiveBuffer(connected_socket,&dbgFlagsCommand->param.setCmd,sizeof(ClientParametersSetCmd),NULL);
    if (EXIT_SUCCESS == error) {
        interfaceVersion version = 1.0;
        DebugFlags *moduleDbgFlag = findDebugFlags(dbgFlagsCommand->param.setCmd.moduleName,&version);
        DEBUG_VAR(dbgFlagsCommand->param.setCmd.moduleName,"%s");
        DEBUG_VAR(moduleDbgFlag,"0x%X");
        if (moduleDbgFlag != NULL) {
            DEBUG_VAR(dbgFlagsCommand->param.setCmd.setFlags,"0x%X");
            if (dbgFlagsCommand->param.setCmd.setFlags & newSyslogMaskSet) {
                setlogmaskex(dbgFlagsCommand->param.setCmd.newSyslogMask);
                DEBUG_VAR(dbgFlagsCommand->param.setCmd.newSyslogMask,"%d");
            }
            if (dbgFlagsCommand->param.setCmd.setFlags & newMaskValueSet) {
                moduleDbgFlag->mask = dbgFlagsCommand->param.setCmd.newMaskValue;
                DEBUG_VAR(moduleDbgFlag->mask,"0x%X");
            }
            /* send back success status */
            error = sendErrorCode(connected_socket,error);
            if (error != EXIT_SUCCESS) {
                ERROR_MSG("sendErrorCode code error %d",error);
            }
        } else {
            NOTICE_MSG("module %s not found",dbgFlagsCommand->param.setCmd.moduleName);
            error = ENOENT;
        }        
    } else {
        ERROR_MSG("receiveBuffer ClientParametersSetCmd error %d (%m)",error);
    }    
    return error;
}

static __inline int buildArgvParameter(char *cmd,char *argv[]) {
    int argc = 0;
    register char *currentPos = cmd;
    register char *startPtr = cmd;
    while (*currentPos != '\0') {
        if (' ' == *currentPos) {
            *currentPos = '\0';
            currentPos++;
            argv[argc++] = startPtr;
            while ((' ' == *currentPos) && (*currentPos != '\0')) {
                *currentPos = '\0';
                currentPos++;
            }
            startPtr = currentPos;
        } else {
            currentPos++;
        }
    }

    if (*startPtr != '\0') {
        argv[argc++] = startPtr;
    }
    return argc;
}

static __inline int sendCustomCommandOutput(int connected_socket,DbgFlagsCommand *dbgFlagsCommand) {
    /*
    * read the custom command to run
    */
    int error =  receiveBuffer(connected_socket,&dbgFlagsCommand->param.customCmd,sizeof(ClientParametersCustomCmd),NULL);
    if (EXIT_SUCCESS == error) {
        /* first look for the right DbgFlags struct to get its custom command handler */
        interfaceVersion version = 1.0;
        DebugFlags *dbgFlags = findDebugFlags(dbgFlagsCommand->param.customCmd.moduleName,&version);
        DEBUG_VAR(dbgFlagsCommand->param.customCmd.moduleName,"%s");
        DEBUG_VAR(dbgFlags,"0x%X");
        DEBUG_VAR(version,"%f");
        if  ((dbgFlags != NULL) && (version >= 1.1)){
            /* then build the custom command's data for the handler */
            char *argv[10];
            int argc = buildArgvParameter(dbgFlagsCommand->param.customCmd.customCommand,argv);
            /* then create a stream for the handlers */
            int fd = dup(connected_socket); /* else the fclose call will close the file descriptor */
            if (fd != -1) {
                FILE *outputStream = fdopen(fd,"w");
                if (outputStream != NULL) {
                    /* then check if the command to run is the help special cmd */
                    if (strncasecmp(dbgFlagsCommand->param.customCmd.customCommand,"help",5) != 0) {
                        if (dbgFlags->customCommands.customCmdHandler  != NULL) {
                            error = (*dbgFlags->customCommands.customCmdHandler)(argc,argv,outputStream);
                        } else {
                            error = EINVAL;
                        }
                    } else {
                        if (dbgFlags->customCommands.customHelpCmd != NULL) {
                            (*dbgFlags->customCommands.customHelpCmd)(outputStream);
                        } else {
                            error = EINVAL;
                        }
                    }

                    if (fclose(outputStream) != 0) {
                        const int closeError = errno;
                        ERROR_MSG("fclose outputStream error %d (%m)",closeError);
                    }
                } else {
                    error = errno;
                    ERROR_MSG("fdopen error %d (%m)",error);
                }
            } else {
                error = errno;
                ERROR_MSG("dup error %d (%m)",error);
            }
        } else { /* module not found */
            NOTICE_MSG("module %s not found",dbgFlagsCommand->param.customCmd.moduleName);
            error = ENOENT;
        }
    } else {
        ERROR_MSG("receiveBuffer ClientParametersCustomCmd error %d (%m)",error);
    }
    return error;
}

#ifdef _DEBUG_
static __inline const char* DbgFlagsSrvCommands2String(const DbgFlagsSrvCommands cmd) {
    const char* szCmd = "";
    switch(cmd) {
        case eSet:
            szCmd = "SetDbgFlags";
            break;
        case eGet:
            szCmd = "GetDbgFlags";
            break;
        case eLsLib:
            szCmd = "ListLibraries";
            break;
            /* no default */
    }
    return szCmd;
}
#endif /* _DEBUG_ */

static void *UDSTCPServer(void* parameter) {
    int error = EXIT_SUCCESS;
    ProcDebugFlagsEntry *dbgFlags = (ProcDebugFlagsEntry *)parameter;
    dbgFlags->server.uds_srv_socket = socket(AF_UNIX,SOCK_STREAM,0);
    if (dbgFlags->server.uds_srv_socket != -1) {
        char socket_name[PATH_MAX];
        error =  makeSocketEntry(socket_name);
        if (EXIT_SUCCESS == error) {
            struct sockaddr_un local;
            int len = 0;
            const mode_t currentCreationMask = umask(S_IWOTH);
            local.sun_family = AF_UNIX;
            strcpy(local.sun_path,socket_name);
            len = strlen(socket_name) + sizeof(local.sun_family);
            if (bind(dbgFlags->server.uds_srv_socket,(struct sockaddr*)&local,len) != -1) {
                umask(currentCreationMask); /* restore previous mask */
                if (listen(dbgFlags->server.uds_srv_socket,5) != -1) {
                    do {
                        int connected_socket = 1;
                        socklen_t addrlen = 0;
                        struct sockaddr_un remote;
                        DEBUG_MSG("Waiting for a connexion...");
                        connected_socket = accept(dbgFlags->server.uds_srv_socket,(struct sockaddr*)&remote,&addrlen);
                        if (connected_socket != -1) {
                            DbgFlagsCommand dbgFlagsCommand;
                            ssize_t n;

                            DEBUG_MSG("connected !");
                            /* read the cmd Header */
                            n = recv(connected_socket,&dbgFlagsCommand.header,sizeof(DbgFlagsCommandHeader),0);
                            if (n != -1) {
#if defined(_DEBUG_)
                                const char *Command = DbgFlagsSrvCommands2String(dbgFlagsCommand.header.command);
                                DEBUG_VAR(dbgFlagsCommand.header.version,"%d");
                                DEBUG_VAR(Command,"%s");
#endif /* _DEBUG_*/
                                if (0 == dbgFlagsCommand.header.version) {
                                    switch(dbgFlagsCommand.header.command)
                                    {
                                    case eSet:
                                        error = receiveNewFlagsValues(connected_socket,&dbgFlagsCommand);
                                        break;
                                    case eGet:
                                        error = sendDebugFlags(connected_socket,&dbgFlagsCommand);
                                        break;
                                    case eLsLib:
                                        error = sendLibrariesNames(connected_socket,&dbgFlagsCommand);
                                        break;
                                    case eCustomCmd:
                                        error = sendCustomCommandOutput(connected_socket,&dbgFlagsCommand);
                                        break;
                                    default:
                                        error = EINVAL;
                                        NOTICE_MSG("unknown command %d",dbgFlagsCommand.header.command);

                                    } /* switch(dbgFlagsCommand.header.command)*/

                                    /* try to send back an error status in case of error */
                                    if (error != EXIT_SUCCESS) {
                                        error = sendErrorCode(connected_socket,error);
                                        if (error != EXIT_SUCCESS) {
                                            ERROR_MSG("sendErrorCode code error %d",error);
                                        }
                                    }
                                } else {
                                    error = EINVAL;
                                    NOTICE_MSG("bad cmd version number (%d)",dbgFlagsCommand.header.version);
                                }
                            } else { /* recv( == -1 */
                                /*error = n;*/
                                ERROR_MSG("recv error %d (%m)",n);
                            }
                            close(connected_socket);
                            connected_socket = -1;
                        } else { /* connected_socket == -1 */
                            error = errno;
                            ERROR_MSG("accept error %d (%m)",error);
                        }
                    } while(EXIT_SUCCESS == error); /* TODO: complete*/
                    DEBUG_VAR(error,"%d");
                    dbgFlags->server.processID = dbgFlags->server.thread = 0;
                    EndUDSServer(dbgFlags);
                    return NULL;
                } else { /* listen(uds_srv_socket,5) == -1 */
                    error = errno;
                    ERROR_MSG("listen error %d (%m)",error);
                }
            } else {
                error = errno;
                umask(currentCreationMask);
                ERROR_MSG("bind error %d (%m)",error);
            }
            close(dbgFlags->server.uds_srv_socket);
            dbgFlags->server.uds_srv_socket = -1;
        } /* getServerSocketName(socket_name) == EXIT_SUCCESS */
        /* error already printed */
    } else { /* (uds_srv_socket != -1) */
        error = errno;
        ERROR_MSG("socket error %d (%m)",error);
    }
    dbgFlags->server.processID = dbgFlags->server.thread = 0; /* thread ended */
    return NULL;
}


static inline int registerDebugFlagsEx(const DebugFlags *dbgFlags,const interfaceVersion version) {
    int error = pthread_mutex_lock(&g_dbgFlags.mutex);
    if (0 == error) {
        int mutexError;
        g_dbgFlags.process = (DebugFlags *)dbgFlags;
        g_dbgFlags.processInterfaceVersion = version;
        error = initUDSTCPServer(&g_dbgFlags); /* error already printed */
        INFO_MSG("process %s registered with error %d (%m)",dbgFlags->moduleName,error); /*deadlock !!! => logger call may registerLibraryDebugFlags but the mutex is not already released ! */
        mutexError = pthread_mutex_unlock(&g_dbgFlags.mutex);
        if (mutexError != 0) {
            ERROR_MSG("pthread_mutex_unlock dbgFlags.mutex error %d (%m)",mutexError);
            if (EXIT_SUCCESS == error) {
                error = mutexError;
            }
        }
        dumpProcDebugFlagsEntry(&g_dbgFlags);
    } else {
        ERROR_MSG("pthread_mutex_lock dbgFlags.mutex error %d (%m)",error);
    }
    return error;
}

static inline int registerLibraryDebugFlagsEx(const DebugFlags *dbgFlags,const interfaceVersion version) {
    int error = EXIT_SUCCESS;
    if (dbgFlags != NULL) {
        LibraryDebugFlagsEntry *libDbgFlags = LibraryDebugFlagsEntryFind(g_dbgFlags.libraries,dbgFlags->moduleName);
        error = pthread_mutex_lock(&g_dbgFlags.mutex);
        if (EXIT_SUCCESS == error) {
            int mutexError;
            error = initUDSTCPServer(&g_dbgFlags);
            if (NULL == libDbgFlags) { /* new entry */
                const int insertError = LibraryDebugFlagsEntryAdd(&g_dbgFlags.libraries,(DebugFlags *)dbgFlags,version);
                INFO_MSG("library %s registered with error %d (%m)",dbgFlags->moduleName,insertError);
                DEBUG_VAR(g_dbgFlags.libraries,"0x%X");
                if (EXIT_SUCCESS == error) {
                    error = insertError;
                }
                dumpProcDebugFlagsEntry(&g_dbgFlags);
            } else { /* entry already exist */
                libDbgFlags->library = (DebugFlags *)dbgFlags;
                WARNING_MSG("overwriting library dbgFlags (duplicate name (%s)?)",dbgFlags->moduleName);
            }
            mutexError = pthread_mutex_unlock(&g_dbgFlags.mutex);
            if (mutexError != 0) {
                ERROR_MSG("pthread_mutex_unlock dbgFlags.mutex error %d (%m)",mutexError);
                if (EXIT_SUCCESS == error) {
                    error = mutexError;
                }
            }
        } else { /* pthread_mutex_lock != EXIT_SUCCESS */
            ERROR_MSG("pthread_mutex_lock dbgFlags.mutex error %d (%m)",error);
        }
    } else {
        error = EINVAL;
        ERROR_MSG("registerLibraryDebugFlags called with a NULL ptr");
    }
    return error;
}

/*
 * int registerDebugFlags(const DebugFlags *dbgFlags);
 */
int registerDebugFlags_1_0(const DebugFlags *dbgFlags) {
    return registerDebugFlagsEx(dbgFlags,1.0);
}
/* function only available in the DSO to keep compatibility with the previous release */
#if defined(__pic__) || defined(__PIC__) || defined(PIC)
asm(".symver registerDebugFlags_1_0,registerDebugFlags@VERS_1.0");
#endif  /* defined(__pic__) || defined(__PIC__) || defined(PIC) */

int registerDebugFlags_1_1(const DebugFlags *dbgFlags) {
    return registerDebugFlagsEx(dbgFlags,1.1);
}
/* new function available in the DSO and the static library */
#if defined(__pic__) || defined(__PIC__) || defined(PIC)
asm(".symver registerDebugFlags_1_1,registerDebugFlags@@VERS_1.1");
#else /* defined(__pic__) || defined(__PIC__) || defined(PIC) */
extern registerDebugFlags(const DebugFlags *dbgFlags) __attribute((alias("registerDebugFlags_1_1")));
#endif  /* defined(__pic__) || defined(__PIC__) || defined(PIC) */

/*
 * int registerLibraryDebugFlags(const DebugFlags *dbgFlags);
 */
int registerLibraryDebugFlags_1_0(const DebugFlags *dbgFlags) {
    return registerLibraryDebugFlagsEx(dbgFlags,1.0);
}
/* function only available in the DSO to keep compatibility with the previous release */
#if defined(__pic__) || defined(__PIC__) || defined(PIC)
asm(".symver registerLibraryDebugFlags_1_0,registerLibraryDebugFlags@VERS_1.0");
#endif  /* defined(__pic__) || defined(__PIC__) || defined(PIC) */


int registerLibraryDebugFlags_1_1(const DebugFlags *dbgFlags) {
    return registerLibraryDebugFlagsEx(dbgFlags,1.1);
}
/* new function available in the DSO and the static library */
#if defined(__pic__) || defined(__PIC__) || defined(PIC)
asm(".symver registerLibraryDebugFlags_1_1,registerLibraryDebugFlags@@VERS_1.1");
#else /* defined(__pic__) || defined(__PIC__) || defined(PIC) */
extern registerLibraryDebugFlags(const DebugFlags *dbgFlags) __attribute((alias("registerLibraryDebugFlags_1_1")));
#endif  /* defined(__pic__) || defined(__PIC__) || defined(PIC) */


int unregisterDebugFlags(const DebugFlags *dbgFlags) {
    int error = EXIT_SUCCESS;
    if (dbgFlags != NULL) {
        error = pthread_mutex_lock(&g_dbgFlags.mutex);
        if (EXIT_SUCCESS == error) {
            int mutexError;
            if (dbgFlags == g_dbgFlags.process) {
                g_dbgFlags.process = NULL;
            } else {
                error = LibraryDebugFlagsEntryRemoveByName(&g_dbgFlags.libraries,dbgFlags->moduleName);
                INFO_MSG("%s unregistered with error %d (%m)",dbgFlags->moduleName,error);
            }
            dumpProcDebugFlagsEntry(&g_dbgFlags);
            mutexError = pthread_mutex_unlock(&g_dbgFlags.mutex);
            if (mutexError != 0) {
                ERROR_MSG("pthread_mutex_unlock dbgFlags.mutex error %d (%m)",mutexError);
                if (EXIT_SUCCESS == error) {
                    error = mutexError;
                }
            }
        } else { /* pthread_mutex_lock */
            ERROR_MSG("pthread_mutex_lock dbgFlags.mutex error %d (%m)",error);
        }
    } else {
        error = EINVAL;
        ERROR_MSG("unregisterDebugFlags called with a NULL ptr");
    }
    return error;
}

/*
* clients functions
*/
//#define MAX_ANSWER_SIZE (sizeof(FulldebugFlags) + sizeof(int)) /* answer format is status then data depending of the cmd sent */

static __inline int getPID(const char *name, pid_t *PIDS,unsigned int *nbPIDS) {
    int error = EXIT_SUCCESS;
    char userRootDirName[PATH_MAX];
    const uid_t userID = getuid();
    DIR *userRootDir = NULL;

    sprintf(userRootDirName,"/tmp/dbgFlags_%d",userID);
    userRootDir = opendir(userRootDirName);
    *nbPIDS = 0;
    if (userRootDir != NULL) {
        const pid_t *PIDSLimit = PIDS + *nbPIDS;
        pid_t *cursor = PIDS;
        struct dirent *directoryEntry = NULL;
        const size_t nameLenght = strlen(name);
        do
        {
            directoryEntry = readdir(userRootDir);
            if (directoryEntry != NULL) {
#if defined(_DIRENT_HAVE_D_TYPE) && defined(DT_SOCk)
                if (DT_SOCK == directoryEntry->d_type) {
#endif /* defined(_DIRENT_HAVE_D_TYPE) && defined(DT_SOCk) */
                    const char *matchingDirEntName = strstr(directoryEntry->d_name,name);
                    if (directoryEntry->d_name == matchingDirEntName) {
                        /* directoryEntry->d_name begin with the same letters than name => match */
                        if (cursor < PIDSLimit) {
                            pid_t pid = 0;
                            matchingDirEntName += nameLenght + 1; /* socketName = modulename + '_' + PID */
                            pid = atoi(matchingDirEntName);
                            if (pid != 0) {
                                *cursor = pid;
                                cursor++;
                                *nbPIDS++;
                                DEBUG_VAR(pid,"%d");
                            } else {
                                DEBUG_MSG("false positive: %s against %s",directoryEntry->d_name,name);
                            }
                        } else {
                            error = ENOMEM;
                            ERROR_MSG("PIDS array size limit reached: not enought space to store entry %s",directoryEntry->d_name);
                        }
                    } else {
                        DEBUG_MSG("%s not match against %s",directoryEntry->d_name,name);
                    }
#if defined(_DIRENT_HAVE_D_TYPE) && defined(DT_SOCk)
                } else {
                    DEBUG_MSG("%s is not a socket (%d)",directoryEntry->d_name,directoryEntry->d_type);
                }
#endif /* defined(_DIRENT_HAVE_D_TYPE) && defined(DT_SOCk) */
            }
        } while(directoryEntry != NULL);

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

/*
*
*/
int listPIDSEx(const uid_t userID,const char *pattern, comparator comp, unsigned int displayFullName) {
    int error = EXIT_SUCCESS;
    char userRootDirName[PATH_MAX];
    DIR *userRootDir = NULL;

    sprintf(userRootDirName,"/tmp/dbgFlags_%d",userID);
    userRootDir = opendir(userRootDirName);
    if (userRootDir != NULL) {
        struct dirent *directoryEntry = NULL;
        const size_t nameLenght = strlen(pattern);
        do
        {
            directoryEntry = readdir(userRootDir);
            if (directoryEntry != NULL) {
#if defined(_DIRENT_HAVE_D_TYPE)
                if (DT_SOCK == directoryEntry->d_type) {
#endif /* defined(_DIRENT_HAVE_D_TYPE) && defined(DT_SOCk) */
                    const unsigned int match = (*comp)(directoryEntry->d_name,pattern);
                    if (match != 0) {
                        /* directoryEntry->d_name match with the pattern parameter*/
                        const char *matchingDirEntName = strrchr(directoryEntry->d_name,'_'); /* socketName = modulename + '_' + PID */
                        DEBUG_VAR(directoryEntry->d_name,"%s");
                        if (matchingDirEntName != NULL) {
                            pid_t pid = 0;
                            matchingDirEntName++;
                            pid = atoi(matchingDirEntName);
                            if (pid != 0) {
                                char processName[PATH_MAX];
                                int getProcessNameError = 0;
                                DEBUG_VAR(pid,"%d");
                                if (displayFullName) {
                                    getProcessNameError = getFullProcessName(pid,processName);
                                } else {
                                    getProcessNameError = getProcessName(pid,processName);
                                }
                                if (EXIT_SUCCESS == getProcessNameError) {
                                    printf("%s [%d]\n",processName,pid);
                                } else {
                                    WARNING_MSG("process name not found for pid %d",pid);
                                }
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
#if defined(_DIRENT_HAVE_D_TYPE)
                } else {
                    DEBUG_MSG("%s is not a socket (d_type = %d)",directoryEntry->d_name,directoryEntry->d_type);
                }
#endif /* defined(_DIRENT_HAVE_D_TYPE) */
            } /* (directoryEntry != NULL) */
        } while(directoryEntry != NULL);

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

int listPIDS(const char *pattern,comparator comp,unsigned int displayFullName) {
    const uid_t userID = getuid();
    return listPIDSEx(userID,pattern,comp,displayFullName);
}

#define displayLevel(stream,level,mask,string) if (level == (mask & level)) fprintf(stream,string " ")
static __inline void displayLogMask(FILE *stream,const int syslogMask) {
    fprintf(stream,"log level set:");
    displayLevel(stream,LOG_EMERG,syslogMask,"Emergency");
    displayLevel(stream,LOG_ALERT,syslogMask,"Alert");
    displayLevel(stream,LOG_CRIT,syslogMask,"Critical");
    displayLevel(stream,LOG_ERR,syslogMask,"Error");
    displayLevel(stream,LOG_WARNING,syslogMask,"Warning");
    displayLevel(stream,LOG_NOTICE,syslogMask,"Notice");
    displayLevel(stream,LOG_INFO,syslogMask,"Info");
    displayLevel(stream,LOG_DEBUG,syslogMask,"Debug");
    fprintf(stream,"\n");
}

static const int lineLengthMax = 60;
int displayFulldebugFlags(FILE *stream,FulldebugFlags *fullDbgFlag) {
    int error = EXIT_SUCCESS;        
    char lineBuffer[128];    
    char *cursor = lineBuffer;
    int lineBufferLength = 0;
    unsigned int i = 0;
    const unsigned int mask = fullDbgFlag->dbgFlags.mask;

    DEBUG_VAR(fullDbgFlag->dbgFlags.moduleName,"%s");
    DEBUG_VAR(fullDbgFlag->dbgFlags.mask,"0x%X");
    DEBUG_VAR(fullDbgFlag->syslogMask,"%d");

    /* header */
    fprintf(stream,"Registered Name:%s\t Mask:0x%X\t Log Mask:0x%X\n"
        ,fullDbgFlag->dbgFlags.moduleName
        ,mask
        ,LOG_PRI(fullDbgFlag->syslogMask));

    /* program's flags'names */
    for(i=0;i<FLAGS_SIZE;i++) {        
        const char *flagName = fullDbgFlag->dbgFlags.flagsNames[i];
        if (flagName[0] != '\0') {
            const unsigned int flagsValue = mask & (1 << i);
            const int n = sprintf(cursor,"%d:%c%s\t"
                ,i
                ,(flagsValue?'+':'-')
                ,flagName);
            DEBUG_VAR(flagName,"%s");
            DEBUG_VAR(flagsValue,"%d");
            lineBufferLength += n;
            if (lineBufferLength < lineLengthMax) {
                cursor += n;
            } else {
                fprintf(stream,"%s\n",lineBuffer);
                cursor = lineBuffer;
                lineBufferLength = 0;
            }
        } /* (flagName[0] != '\0') */    
    } /* for(i=0;i<FLAGS_SIZE;i++)*/

    if (lineBufferLength != 0) {
        /* display last line */
        fprintf(stream,"%s\n",lineBuffer);
    }

    displayLogMask(stream,fullDbgFlag->syslogMask);
    
    return error;
}

static __inline size_t endOfLibrariesNames(const char* buffer,const size_t usedSize) {
    const char *cursor = buffer + usedSize-1;
    size_t end;
    while(*cursor != '\0') {
        cursor--;
    }
    end = cursor - buffer;
    return end;
}

static __inline int displayLibrariesNames(FILE *stream,const char *buffer,const size_t usedSize) {
    int error = EXIT_SUCCESS;
    char lineBuffer[128];    
    char *lineCursor = lineBuffer;
    const char *bufferCursor = buffer;
    const char *limitBuffer = buffer+usedSize;
    int lineBufferLength = 0;

    while(bufferCursor < limitBuffer) {
        if (bufferCursor[0] != '\0') {
            const int n = sprintf(lineCursor,"%s/\t",bufferCursor);
            lineBufferLength += n;
            bufferCursor += n - 2 + 1; 
            if (lineBufferLength < lineLengthMax) {
                lineCursor += n;
            } else {
                fprintf(stream,"%s\n",lineBuffer);
                lineCursor = lineBuffer;
                lineBufferLength = 0;
            }
        } else {
            bufferCursor++;
        }
    } /* while(bufferCursor < limitBuffer) { */

    if (lineBufferLength != 0) {
        /* display last line */
        fprintf(stream,"%s\n",lineBuffer);
    }

    return error;
}

static __inline int readLibrariesNames(int connected_socket,DbgFlagsCommand *cmd) {
    int error = EXIT_SUCCESS;
    size_t cmd_size = 0;

    /* BUG ? buffer MUST BE SEND in two part else the second is received zeroed (?!!!!) */
    //cmd_size = sizeof(DbgFlagsCommandHeader) + sizeof(ClientParametersLsLibs);
    //error = sendBuffer(uds_client_socket,&cmd,cmd_size,NULL);
    /* send header */
    cmd_size = sizeof(DbgFlagsCommandHeader);
    error = sendBuffer(connected_socket,&cmd->header,cmd_size,NULL);
    if (EXIT_SUCCESS == error) {
        /* then body */
        /*cmd_size = sizeof(ClientParametersLsLibs);
        error = sendBuffer(connected_socket,&cmd->lsLibCmd,cmd_size,NULL);
        if (EXIT_SUCCESS == error) {*/
            /* wait for the answer */            
            const size_t bufferAllocatedSize = 2048;
            char *buffer = (char *)malloc(bufferAllocatedSize);
            if (buffer != NULL) {
                char *writeCursor = buffer;
                size_t availableSize = bufferAllocatedSize;
                do
                {
                    size_t bytesReceived = 0;
                    error = receiveBuffer(connected_socket,writeCursor,availableSize,&bytesReceived);
                    switch(error)
                    {
                    case EXIT_SUCCESS: {
                            /* the buffer is full: may be more data is available */
                            const size_t usableDataSize = endOfLibrariesNames(writeCursor,bytesReceived);
                            const size_t remainingDataSize = bytesReceived - usableDataSize;

                            /* display the usable data received */
                            error = displayLibrariesNames(stdout,buffer,usableDataSize);

                            /* prepare buffer for the next read */
                            if (remainingDataSize != 0) {
                                memmove(buffer,buffer+usableDataSize,remainingDataSize);
                                writeCursor = buffer + remainingDataSize;
                                availableSize = bufferAllocatedSize - remainingDataSize;
                            } else {
                                writeCursor = buffer;
                                availableSize = bufferAllocatedSize;
                            }
                        } /* case EXIT_SUCCESS*/
                        break;
                    case EPIPE:
                        if (!isRemoteStatus(buffer,bytesReceived)) {
                            displayLibrariesNames(stdout,buffer,bytesReceived);
                        } else {
                            error = getRemoteStatus(buffer);
                            ERROR_MSG("remote error %d (%s)",error,strerror(error));
                        }
                        break;
                    default:
                        ERROR_MSG("receiveBuffer LibrariesNames error %d (%s)",error,strerror(error));
                    } /* switch(error) */
                } while(EXIT_SUCCESS == error);

                if (EPIPE == error) {
                    error = EXIT_SUCCESS;
                }

                free(buffer);
                buffer = NULL;
            } else {
                error = ENOMEM;
                ERROR_MSG("failed to allocate %d bytes for LibrariesNames' buffer",bufferAllocatedSize);
            }
        /*} else {
            error = errno;
            ERROR_MSG("sendBuffer ClientParametersLsLibs error %d (%m)",error);
        }*/

    } else {
        error = errno;
        ERROR_MSG("sendBuffer DbgFlagsCommandHeader error %d (%m)",error);
    }

    return error;
}

#if 0
static __inline int readDebugFlag(int connected_socket,DbgFlagsCommand *cmd) {
    int error = EXIT_SUCCESS;
    size_t cmd_size = 0;

    /* BUG ? buffer MUST BE SEND in two part else the second is received zeroed (?!!!!) */
    //cmd_size = sizeof(DbgFlagsCommandHeader) + sizeof(ClientParametersGetCmd);
    //error = sendBuffer(uds_client_socket,&cmd,cmd_size,NULL);
    /* send header */
    cmd_size = sizeof(DbgFlagsCommandHeader);
    error = sendBuffer(connected_socket,&cmd->header,cmd_size,NULL);
    if (EXIT_SUCCESS == error) {
        /* then body */
        cmd_size = sizeof(ClientParametersGetCmd);
        error = sendBuffer(connected_socket,&cmd->param.getCmd,cmd_size,NULL);
        if (EXIT_SUCCESS == error) {
            /* wait for the answer */
            FulldebugFlags fullDbgFlag;
            size_t bytesReceived = 0;
            error = receiveBuffer(connected_socket,&fullDbgFlag,sizeof(FulldebugFlags),&bytesReceived);
            if (EXIT_SUCCESS == error) {
                error = displayFulldebugFlags(stdout,&fullDbgFlag);                
            } else {
                const unsigned char* buffer = (unsigned char*)&fullDbgFlag;
                if ((EPIPE == error) && (isRemoteStatus(buffer,bytesReceived))) {                    
                    error = getRemoteStatus(buffer);
                    DEBUG_MSG("remote error %d (%m)",error);
                } else {
                    ERROR_MSG("receiveBuffer FulldebugFlags error %d (%m)",error);
                }                                               
            }
        } else { /* sendBuffer ClientParametersGetCmd */            
            error = errno;
            ERROR_MSG("sendBuffer ClientParametersGetCmd error %d (%m)",error);
        }
    } else {
        error = errno;
        ERROR_MSG("sendBuffer DbgFlagsCommandHeader error %d (%m)",error);
    }

    return error;
}
#endif

static __inline int readDebugFlag(int connected_socket,DbgFlagsCommand *cmd,FulldebugFlags *pfullDbgFlag) {
    int error = EXIT_SUCCESS;
    size_t cmd_size = 0;

    /* BUG ? buffer MUST BE SEND in two part else the second is received zeroed (?!!!!) */
    //cmd_size = sizeof(DbgFlagsCommandHeader) + sizeof(ClientParametersGetCmd);
    //error = sendBuffer(uds_client_socket,&cmd,cmd_size,NULL);
    /* send header */
    cmd_size = sizeof(DbgFlagsCommandHeader);
    error = sendBuffer(connected_socket,&cmd->header,cmd_size,NULL);
    if (EXIT_SUCCESS == error) {
        /* then body */
        cmd_size = sizeof(ClientParametersGetCmd);
        error = sendBuffer(connected_socket,&cmd->param.getCmd,cmd_size,NULL);
        if (EXIT_SUCCESS == error) {
            /* wait for the answer */            
            size_t bytesReceived = 0;
            error = receiveBuffer(connected_socket,pfullDbgFlag,sizeof(FulldebugFlags),&bytesReceived);
            //if (error != EXIT_SUCCESS) {
            if (EXIT_SUCCESS == error) {
                const unsigned char* buffer = (unsigned char*)pfullDbgFlag;
                if ((EPIPE == error) && (isRemoteStatus(buffer,bytesReceived))) {                    
                    error = getRemoteStatus(buffer);
                    DEBUG_MSG("remote error %d (%s)",error,strerror(error));
                    memset(pfullDbgFlag,0,sizeof(FulldebugFlags));
                }
            } else {
                ERROR_MSG("receiveBuffer FulldebugFlags error %d (%m)",error);
            }                                                           
        } else { /* sendBuffer ClientParametersGetCmd */            
            error = errno;
            ERROR_MSG("sendBuffer ClientParametersGetCmd error %d (%m)",error);
        }
    } else {
        error = errno;
        ERROR_MSG("sendBuffer DbgFlagsCommandHeader error %d (%m)",error);
    }

    return error;
}

#define MAX_CUSTOM_CMD_ANSWER       1024
static __inline void displayCustomCommandAnswer(FILE *output, const char *buffer) {
    fprintf(output,buffer);
}

static __inline int runCustomCommand(int connected_socket,DbgFlagsCommand *cmd,FILE *output) {
    int error = EXIT_SUCCESS;
    size_t cmd_size = 0;
    /* BUG ? buffer MUST BE SEND in two part else the second is received zeroed (?!!!!) */
    //cmd_size = sizeof(DbgFlagsCommandHeader) + sizeof(ClientParametersGetCmd);
    //error = sendBuffer(uds_client_socket,&cmd,cmd_size,NULL);
    /* send header */
    cmd_size = sizeof(DbgFlagsCommandHeader);
    error = sendBuffer(connected_socket,&cmd->header,cmd_size,NULL);
    if (EXIT_SUCCESS == error) {
        /* then body */
        cmd_size = sizeof(ClientParametersCustomCmd);
        error = sendBuffer(connected_socket,&cmd->param.customCmd,cmd_size,NULL);
        if (EXIT_SUCCESS == error) {
            /* wait for the answer */
            while (EXIT_SUCCESS == error) {
                char customCmdAnswer[MAX_CUSTOM_CMD_ANSWER];
                size_t bytesReceived = 0;
                memset(customCmdAnswer,0,sizeof(customCmdAnswer));
                error = receiveBuffer(connected_socket,customCmdAnswer,sizeof(customCmdAnswer)-1,&bytesReceived);
                DEBUG_VAR(error,"%d");
                if  ((EXIT_SUCCESS == error) || (EPIPE == error)) {
                    DEBUG_DUMP_MEMORY(customCmdAnswer,sizeof(customCmdAnswer));
                    if (!isRemoteStatus(customCmdAnswer,bytesReceived)) {
                        customCmdAnswer[MAX_CUSTOM_CMD_ANSWER-1] = '\0';
                        displayCustomCommandAnswer(output,customCmdAnswer);
                    } else {
                        error = getRemoteStatus(customCmdAnswer);
                        DEBUG_MSG("remote error (%d)",error);
                    }
                } else {
                    ERROR_MSG("receiveBuffer FulldebugFlags error %d",error);
                }
            } /* while (EXIT_SUCCESS == error)*/
        } else { /* sendBuffer ClientParametersGetCmd */
            error = errno;
            ERROR_MSG("sendBuffer ClientParametersGetCmd error %d (%m)",error);
        }
    } else {
        error = errno;
        ERROR_MSG("sendBuffer DbgFlagsCommandHeader error %d (%m)",error);
    }
    return error;
}

static __inline int writeDebugFlagMasks(int connected_socket,DbgFlagsCommand *cmd) {
    int error = EXIT_SUCCESS;
    size_t cmd_size = 0;

    /* BUG ? buffer MUST BE SEND in two part else the second is received zeroed (?!!!!) */
    //cmd_size = sizeof(DbgFlagsCommandHeader) + sizeof(ClientParametersGetCmd);
    //error = sendBuffer(uds_client_socket,&cmd,cmd_size,NULL);
    /* send header */
    cmd_size = sizeof(DbgFlagsCommandHeader);
    error = sendBuffer(connected_socket,&cmd->header,cmd_size,NULL);
    if (EXIT_SUCCESS == error) {
        /* then body */
        cmd_size = sizeof(ClientParametersSetCmd);
        error = sendBuffer(connected_socket,&cmd->param.setCmd,cmd_size,NULL);
        if (EXIT_SUCCESS == error) {
            /* wait for the answer */
            RemoteStatus remoteStatus;
            size_t bytesReceived = 0;
            error = receiveBuffer(connected_socket,&remoteStatus,sizeof(RemoteStatus),&bytesReceived);
            if (EXIT_SUCCESS == error) {
                if (isRemoteStatus((const unsigned char *)&remoteStatus,bytesReceived)) {
                    error = getRemoteStatus((const unsigned char *)&remoteStatus);
                    DEBUG_MSG("remote error %d (%s)",error,strerror(error));
                } else {
                    ERROR_MSG("receiveBuffer RemoteStatus invalid");
                    error = ENOMSG;
                }
            } else {
                ERROR_MSG("receiveBuffer RemoteStatus error %d (%s)",error,strerror(error));
            }
        }
    } else {
        error = errno;
        ERROR_MSG("sendBuffer DbgFlagsCommandHeader error %d (%s)",error,strerror(error));
    }
    return error;
}

int UDSTCPClient(ClientParameters *params) {
    int error = EXIT_SUCCESS;
    int uds_client_socket = socket(AF_UNIX,SOCK_STREAM,0);

    if (uds_client_socket != -1) {
        char socket_name[PATH_MAX];                
        const uid_t userID = params->uid;

        error = getSocketNameEx(userID,params->processName,params->pid,params->comp,socket_name);
        if (EXIT_SUCCESS == error) {    
            struct sockaddr_un remote;
            int len = 0;

            DEBUG_MSG("trying to connect to server %s...",socket_name);
            remote.sun_family = AF_UNIX;
            strcpy(remote.sun_path,socket_name);
            len = strlen(socket_name) + sizeof(remote.sun_family);
            if (connect(uds_client_socket,(struct sockaddr*)&remote,len) != -1) {                
                DbgFlagsCommand cmd;                
                //size_t cmd_size = 0;
                //ssize_t n = 0;

                DEBUG_MSG("connected !");
                cmd.header.version = 0;
                cmd.header.command = (unsigned char)params->command;
                switch(cmd.header.command) {
                    case eSet:
                        cmd.param.setCmd.setFlags = params->param.setCmd.setFlags;
                        cmd.param.setCmd.newMaskValue = params->param.setCmd.newMaskValue;
                        cmd.param.setCmd.newSyslogMask = params->param.setCmd.newSyslogMask;
                        strcpy(cmd.param.setCmd.moduleName,params->moduleName);
                        error = writeDebugFlagMasks(uds_client_socket,&cmd);
                        break;
                    case eGet:                   
                        strcpy(cmd.param.getCmd.moduleName,params->moduleName);
                        error = readDebugFlag(uds_client_socket,&cmd,&params->param.getCmd.fullDbgFlags);                        
                        break;
                    case eLsLib:
                        //strcpy(cmd.lsLibCmd.moduleName,params->moduleName);
                        error = readLibrariesNames(uds_client_socket,&cmd);
                        break;
                    case eCustomCmd:
                        strcpy(cmd.param.customCmd.customCommand,params->param.customCmd.customCommand);
                        strcpy(cmd.param.customCmd.moduleName,params->param.customCmd.moduleName);
                        error = runCustomCommand(uds_client_socket,&cmd,params->param.customCmd.customCommandAnswerOutput);
                        break;
                } /* switch(cmd.header.command) */
                DEBUG_VAR(error,"%d");

                if (EPIPE == error) {
                    error = EXIT_SUCCESS;
                }
            } else {
                error = errno;
                ERROR_MSG("connect to %s error %d (%m)",socket_name,error);
            }
            close(uds_client_socket);
            uds_client_socket = -1;
        } else { /* getSocketNameEx error */
            ERROR_MSG("getSocketNameEx %s %d error %d",params->moduleName,params->pid,error);
        }
    } else {
        error = uds_client_socket;
        ERROR_MSG("socket error %d (%m)",error);
    }
    return error;
}

#include <dbgflags/ModuleVersionInfo.h>
MODULE_NAME(dbgFlagsUsingUnixDomainSocket);
MODULE_AUTHOR(Olivier Charloton);
MODULE_FILE_VERSION(1.1);
MODULE_DESCRIPTION(dbgFlags management using Unix Domain Socket);
MODULE_COPYRIGHT(LGPL);
