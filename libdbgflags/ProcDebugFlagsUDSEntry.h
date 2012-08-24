#ifndef _PROC_DEBUG_FLAGS_ENTRY_H_
#define _PROC_DEBUG_FLAGS_ENTRY_H_

#include <dbgflags/dbgflags.h>
#include "LibraryDebugFlagsEntry.h"
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

typedef struct ProcDebugFlagsServer_ {
   pthread_t thread;
   int uds_srv_socket;
   pid_t processID;
} ProcDebugFlagsServer;

typedef struct ProcDebugFlagsEntry_ {
    DebugFlags *process;
    LibraryDebugFlagsEntry *libraries;
    pthread_mutex_t mutex;
    ProcDebugFlagsServer server;
} ProcDebugFlagsEntry;

static ProcDebugFlagsEntry g_dbgFlags = {
        NULL, /*process*/
        NULL, /* libraries */
        /*PTHREAD_MUTEX_INITIALIZER*/PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP, /* mutex */
        {0,-1,0} /* server */
};

#if 0
static __inline void getParamProcessName(const char *moduleFullName,char *processName) {
    const char *readCursor = moduleFullName;
    char *writeCursor = processName;
    while( (*readCursor != '\0') && (*readCursor != '\\') && (*readCursor != '/')){
        *writeCursor = *readCursor;
        readCursor++;
        writeCursor++;
    }
    *writeCursor = '\0';
}

static __inline void getParamModuleName(const char *moduleFullName,char *moduleName) {
    const char *readCursor = moduleFullName;
    char *writeCursor = moduleName;
    while(*readCursor != '\0') {
        if ((*readCursor != '\\') && (*readCursor != '/')){
            *writeCursor = *readCursor;        
            writeCursor++;
        } else {
            writeCursor = moduleName;
        }
        readCursor++;
    }
    *writeCursor = '\0';
}
#endif

static __inline DebugFlags *findDebugFlags(const char *name) {
    DebugFlags *dbgFlags = NULL;

    if (g_dbgFlags.process != NULL) { /* registered process'module name ? */
        if (('\0' == name[0]) || (strcmp(g_dbgFlags.process->moduleName,name) == 0)) {
            dbgFlags = g_dbgFlags.process;
        }
    }

    if (NULL == dbgFlags) { /* no, so may be one of its library... */
        LibraryDebugFlagsEntry *libDbgFlags = LibraryDebugFlagsEntryFind(g_dbgFlags.libraries,name);
        if (libDbgFlags != NULL) {
            dbgFlags = libDbgFlags->library;
        }
    }    

    return dbgFlags;
}

static __inline void dumpDebugFlags(const DebugFlags *pEntry) {
    if (pEntry != NULL) {
        unsigned int i;     
        DEBUG_VAR(pEntry->moduleName,"%s");
        for(i=0;i<32;i++) {
            if (pEntry->flagsNames[i][0] != '\0') {
                DEBUG_VAR(pEntry->flagsNames[i],"%s");
            }
        }
        DEBUG_VAR(pEntry->mask,"0x%X");     
    } /* (pEntry != NULL) */     
}

static __inline void dumpLibraryDebugFlagsEntry(const LibraryDebugFlagsEntry *pEntry) {
    if (pEntry != NULL) {
        if (pEntry->library != NULL) {
            dumpDebugFlags(pEntry->library);
            if (pEntry->next != NULL) {
                dumpLibraryDebugFlagsEntry(pEntry->next);
            } /* (pEntry->next != NULL) */
        } /* (pEntry->library != NULL) */     
    } /* (pEntry != NULL) */
}  

static __inline  void dumpProcDebugFlagsServer(const ProcDebugFlagsServer *server) {
        DEBUG_VAR(server->thread,"%u");
        DEBUG_VAR(server->processID,"%u");
        DEBUG_VAR(server->uds_srv_socket,"%d");
}

static __inline void dumpProcDebugFlagsEntry(const ProcDebugFlagsEntry *pEntry) {
    const int syslogMask = setlogmask(0);
    if (pEntry != NULL) {
        if (pEntry->process != NULL) {
            dumpDebugFlags(pEntry->process);
        }
        if (pEntry->libraries != NULL) {
            dumpLibraryDebugFlagsEntry(pEntry->libraries);
        }
        DEBUG_VAR(pEntry->mutex,"%d");
        dumpProcDebugFlagsServer(&(pEntry->server));
    } /* (pEntry != NULL) */
    DEBUG_VAR(syslogMask,"0x%X");
}

#endif /* _PROC_DEBUG_FLAGS_ENTRY_H_ */
