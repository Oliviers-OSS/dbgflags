#ifndef _UDS_SERVER_MANAGEMENT_H_
#define _UDS_SERVER_MANAGEMENT_H_

#include <pthread.h>
#include <signal.h>

#include "UdsManagement.h"
#include "ProcDebugFlagsUDSEntry.h"

static __inline int getServerSocketName(char *socketName) {
    char processName[PATH_MAX];
    int error = getCurrentProcessName(processName);

    if (EXIT_SUCCESS == error) {
        const pid_t pid = getpid();
        sprintf(socketName,"%s_%d",processName,pid);
    }
    /* error already printed */
    return error;
}

static __inline int makeSocketEntry(char *fullSocketName) {
    char userRootDirName[PATH_MAX];
    int error = makeRootDir(userRootDirName);
    if (EXIT_SUCCESS == error) {
        char socketName[PATH_MAX];
        error = getServerSocketName(socketName);
        if (EXIT_SUCCESS == error) {
            sprintf(fullSocketName,"%s/%s",userRootDirName,socketName);
            if (unlink(fullSocketName) == -1) {
                if (errno != ENOENT) {
                    error = errno;
                    ERROR_MSG("unlink %s error %d (%m)",fullSocketName,error);
                }
            }
        }
        /* error already printed */
    }
    /* error already printed */
    return error;
}

static __inline int removeUDSEntry() {
    char socketName[PATH_MAX];
    int error = getServerSocketName(socketName);
    if (EXIT_SUCCESS == error) {
        const uid_t userID = getuid();
        char fullSocketName[PATH_MAX];
        sprintf(fullSocketName,"/tmp/dbgFlags_%d/%s",userID,socketName);
        /*DEBUG_VAR(fullSocketName,"%s");*/
        if (unlink(fullSocketName) == -1) {
            error = errno;
            ERROR_MSG("unlink %s error %d (%m)",fullSocketName,error);
        }  /* (unlink(fullSocketName) == -1) */
    } /* (EXIT_SUCCESS == getServerSocketName */
    /* error already printed */
    return error;
}

static __inline void EndUDSServer(ProcDebugFlagsEntry *dbgFlags) {
    if (dbgFlags->server.uds_srv_socket != -1) {
        close(dbgFlags->server.uds_srv_socket); /* this will end the server loop in error */
        dbgFlags->server.uds_srv_socket = -1;
    }
}

#define STOP_UDSSRV SIGUSR2
static __inline int stopUDSTCPServer(ProcDebugFlagsEntry *dbgFlags) {
  int error = EINVAL;
  if (dbgFlags) {
    if (dbgFlags->server.thread != 0) {
       error = pthread_kill(dbgFlags->server.thread,STOP_UDSSRV);
    }
  }
  return error;
}

static __inline int initUDSTCPServer(ProcDebugFlagsEntry *dbgFlags) {
    int error = EXIT_SUCCESS;
    const pid_t currentPID = getpid();
    if ((currentPID != dbgFlags->server.processID) && (dbgFlags->server.processID != 0)) {
       //TODO stop the running thread if any
       stopUDSTCPServer(dbgFlags);
       dbgFlags->server.thread = 0;
    }
    if (0 == dbgFlags->server.thread) {
        error = pthread_create(&dbgFlags->server.thread,NULL,UDSTCPServer,dbgFlags);
        if (0 == error) {
            dbgFlags->server.processID = currentPID;
            if (atexit(FreeAllocatedResources) != 0) {
                error = errno;
                ERROR_MSG("atexit FreeAllocatedResources error %d (%m)",error);
            }
        } else {
            ERROR_MSG("pthread_create UDSTCPServer error %d (%s)",error,strerror(error));
        }
    } else {
      DEBUG_MSG("server is already running (PID %u / PTH 0x%X)",dbgFlags->server.processID,dbgFlags->server.thread);
    }
    return error;
}

#endif /* _UDS_SERVER_MANAGEMENT_H_ */
