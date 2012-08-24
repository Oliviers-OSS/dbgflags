#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <dbgflags/dbgflags.h>

typedef unsigned int(*comparator)(const char* name, const char*pattern);

typedef enum DbgFlagsSrvCommands_ {
    eSet,eGet,eLsLib,eCustomCmd/*,eGetValues*/
} DbgFlagsSrvCommands;

typedef enum Comparators_ {
    eContains = 0, eBegin, eEqual, eEnd, eError
} Comparators;

typedef struct FulldebugFlags_ {
    DebugFlags dbgFlags;
    int        syslogMask;
} FulldebugFlags;

typedef struct RemoteStatus_ {
    unsigned char tag;
    int status;
} RemoteStatus;

#define newSyslogMaskSet (1<<0)
#define newMaskValueSet  (1<<1)
typedef struct ClientParametersSetCmd_ {
    unsigned char setFlags;
    unsigned int newMaskValue;
    int newSyslogMask;
    char moduleName[MAX_DEBUGFLAGS_NAME_SIZE]; /* "" for the process or sub module ("library") name */
} ClientParametersSetCmd;

typedef struct ClientParametersGetCmd_ {
    char moduleName[MAX_DEBUGFLAGS_NAME_SIZE];
    //pid_t pid;
    //Comparators comparator;
    FulldebugFlags fullDbgFlags;
} ClientParametersGetCmd;

typedef struct ClientParametersLsLibs_ {
    //char moduleName[MAX_DEBUGFLAGS_NAME_SIZE];
} ClientParametersLsLibs;

/*
typedef struct ClientParametersGetValuesCmd_ {
    char moduleName[MAX_DEBUGFLAGS_NAME_SIZE];
    //pid_t pid;
    //Comparators comparator;
    FulldebugFlags fullDbgFlags;
} ClientParametersGetValuesCmd;
*/

typedef struct ClientParametersCustomCmd_ {
    char moduleName[MAX_DEBUGFLAGS_NAME_SIZE];
    char customCommand[MAX_CUSTOM_CMD_AND_PARAMS];
    FILE *customCommandAnswerOutput;
} ClientParametersCustomCmd;

typedef struct ClientParameters_ {
    const char* processName;
    const char* moduleName;
    pid_t pid;
    comparator comp;
    DbgFlagsSrvCommands command;
    union /*cmdParams*/ {
        ClientParametersSetCmd setCmd;
        ClientParametersGetCmd getCmd;
        ClientParametersCustomCmd customCmd;
    } param;
    uid_t uid; // UID of the running process to connect to
    //int cmdStatusCode;
} ClientParameters;

typedef struct DbgFlagsCommandHeader_ {
    unsigned char version; /* "protocol" version number */
    unsigned char command; /* command send */
} DbgFlagsCommandHeader;

typedef struct DbgFlagsCommand_ {
    DbgFlagsCommandHeader header;    
    union /*params*/ { /* extra parameters for the cmd if needed */
        ClientParametersSetCmd setCmd;
        ClientParametersGetCmd getCmd;
        //ClientParametersLsLibs lsLibCmd;
        ClientParametersCustomCmd customCmd;
    } param;
} DbgFlagsCommand;

#endif /* _PROTOCOL_H_ */
