#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <dbgflags/dbgflags.h>
#include <dbgflags/loggers.h>

#ifndef LOGGER
#define LOGGER dynamicLogger
#endif /* LOGGER */
//#define LOG_OPTS 0
#define DEBUG_EOL "\n"

#include <dbgflags/debug_macros.h>

void help(FILE *stream);
int dbgCommandsHandler(int argc, char *argv[], FILE *stream);

#define ZONE_INIT   FLAG_0
#define ZONE_FUNCTION   FLAG_1
#define ZONE_DBG_CMDS   FLAG_2

DebugFlags debugFlags = {
    "test",
    {
        "init"
        , "function"
        , "debug cmds"
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
    , 0x0
    , {help, dbgCommandsHandler}
};

#define MODULE_FLAG ZONE_DBG_CMDS

void help(FILE *stream) {
    fprintf(stream, "debug help commands:\n\ttest\n\thello world\n\tcmd -f<arg1> [-s] [-t<arg2>]\n");
}

static const struct option longopts[] = {
    {"first", required_argument, NULL, 'f'},
    {"second", no_argument, NULL, 's'},
    {"third", required_argument, NULL, 't'},
    {NULL, 0, NULL, 0}
};

static inline int testDbgCommandsHandler(FILE *stream) {
    fprintf(stream, "%s", __FUNCTION__);
    DEBUG_MARK;
    return EXIT_SUCCESS;
}

static inline int helloWorldDbgCommandsHandler(FILE *stream) {
    fprintf(stream, "%s", __FUNCTION__);
    DEBUG_MARK;
    return EXIT_SUCCESS;
}

typedef struct cmdArguments_ {
    const char *first;
    unsigned int second;
    const char *third;
} cmdArguments;

static inline int cmdDbgCommandsHandler(cmdArguments *argument, FILE *stream) {
    fprintf(stream, "%s with parameters: {%s,%d,%s}", __FUNCTION__, argument->first,argument->second,argument->third);
    DEBUG_MARK;
    return EXIT_SUCCESS;
}

int dbgCommandsHandler(int argc, char *argv[], FILE *stream) {
    int status = EXIT_SUCCESS;
    if (argc > 1) {
        if (strcmp(argv[0], "test") == 0) {
            testDbgCommandsHandler(stream);
        } else if ((strcmp(argv[0], "hello") == 0) && (strcmp(argv[1], "world") == 0)) {
            helloWorldDbgCommandsHandler(stream);
        } else if (strcmp(argv[0], "cmd") == 0) {
            int optc;
            cmdArguments arguments;
            memset(&arguments,0,sizeof(arguments));
            while ((optc = getopt_long(argc, argv, "f:st:", longopts, NULL)) != -1) {
                switch (optc) {
                    case 'f':
                        arguments.first = optarg;                        
                        break;
                    case 's':
                        arguments.second = 1;                        
                        break;
                    case 't':
                        if (optarg != NULL) {
                            arguments.third = optarg;
                        }                        
                        break;
                    default:
                        status = EINVAL;
                        ERROR_MSG("invalid cmd command parameter");
                }
            } /* while ((optc = getopt_long (argc, argv, "f:st:", longopts, NULL)) != -1) */

            if (EXIT_SUCCESS == status) {
                status = cmdDbgCommandsHandler(&arguments,stream);
            }
        } else { /* unknow/invalid cmd */
            status = EINVAL;
            ERROR_MSG("unknow or invalid cmd received");
        }
    } else { /*! argc > 1 */
        ERROR_MSG("bad debug cmd received");
        status = EINVAL;
    }

    DEBUG_VAR(status, "%d");
    return status;
}

#undef MODULE_FLAG 
#define MODULE_FLAG ZONE_FUNCTION

unsigned int function(const unsigned int a, const unsigned int b, const unsigned int d) {
    unsigned int i;
    unsigned int s = 0;
    for (i = 0; i < b; i++) {
        sleep(d);
        s += a;
        DEBUG_VAR(s, "%d");
    }
    return s;
}

#undef MODULE_FLAG 
#define MODULE_FLAG ZONE_INIT

int main(int argc, char *argv[]) {
    int error = EXIT_SUCCESS;
    unsigned int i;
    unsigned int n = 10000;
    volatile unsigned int r = 0;
    dynamicLoggerConfiguration configuration = {
        sizeof(configuration),"dynamicTest",LOG_CONS|LOG_PERROR|LOG_PID|LOG_TID|LOG_RDTSC,LOG_LOCAL0,"",1024*1024,0,"syslogex"};
    //struct rusage used;

    registerDebugFlags(&debugFlags);
    debugFlags.mask = 0xFFFFFFFF;
    //error = openLogger(&configuration);
    //error = openLoggerFromCmdLine(&configuration,&debugFlags.mask);
    error = openLoggerFromConfigurationFile("/home/oc/projects/dbgflags/tests/main-dynamic/dynamicLogger.conf",&configuration,&debugFlags.mask);
    if (argc > 1) {
        n = atoi(argv[1]);
    }

    for (i = 0; i < n; i++) {
        r = function(4, n, 1);
        DEBUG_VAR(r, "%d");
    }

    /*if (getrusage(RUSAGE_SELF,&used) == 0) {

    } else {
       ERROR_MSG("getrusage error");
    }*/

    //unregisterDebugFlags(&debugFlags);
    closeLogger();
    return error;
}
