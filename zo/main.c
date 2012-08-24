#include "config.h"
#include "debug.h"
#include <dbgflags/dbgflags.h>
#include <ModuleVersionInfo.h>

#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>
#include <linux/limits.h>
#include <sys/types.h>

#ifndef TO_STRING
#define STRING(x) #x
#define TO_STRING(x) STRING(x)
#endif /* STRING */

/*
 * private includes files to get access to inline functions and definitions
 */
#include "ProcDebugFlagsUDSEntry.h"
#include "UdsManagement.h"
extern int listPIDS(const char *pattern, comparator comp,
        unsigned int displayFullName);
extern unsigned int contains(const char *name, const char *pattern);

static __inline void printVersion(void) {
    printf(TO_STRING(PROGRAM_NAME) " v" TO_STRING(PROGRAM_VERSION) "\n");
}

/* no argument from the point of view of the getOption function,
 * parameters will be processed by a special function
 */
#define required_special_argument no_argument

static const struct option longopts[] = {
    {"process", required_argument, NULL, 'p'},
    {"pid", required_argument, NULL, 'i'},
    {"list", optional_argument, NULL, 'l'},
    {"module", required_argument, NULL, 'm'},
    {"flags", required_argument, NULL, 'g'},
    {"syslog-level", required_argument, NULL, 's'},
    {"on", required_special_argument, NULL, 'o'},
    {"off", required_special_argument, NULL, 'f'},
    {"command", required_special_argument, NULL, 'c'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
};

static __inline void printHelp(const char *errorMsg) {
    /*"  -c, --command <custom cmd and its parameters        \n"*/
#define DESC TO_STRING(PROGRAM_NAME) ": set/get debugFlags'values of a program or a loaded library and manage log level\n"
#define USAGE  "Usage: " TO_STRING(PROGRAM_NAME) " [OPTIONS] \n" \
"  -p, --process=<process'name>]                       \n"\
"  -i, --pid=<process id>                              \n"\
"  -l, --list[=<module's name>]                        \n"\
"  -m, --module=<module's name>                        \n"\
"  -g, --flags=<new flags value>                       \n"\
"  -s, --syslog-level=<new syslog threshold value>     \n"\
"  -o, --on <list of flags'bits to set>                \n"\
"  -f, --off <list of flags'bits to unset>             \n"\
"  -h, --help                                          \n"\
"  -v, --version                                       \n"

    if (errorMsg != NULL) {
        fprintf(stderr, "Error: %s\n" USAGE, errorMsg);
    } else {
        fprintf(stdout, DESC USAGE);
    }

#undef USAGE
}

typedef enum Operation_ {
    op_read = 0 /* default value */
    , op_write, op_list
} Operation;

#define syslogMaskSet      (1<<0)
#define maskSet            (1<<1)
#define relativeOnMaskSet  (1<<2)
#define relativeOffMaskSet (1<<3)

typedef struct Parameters_ {
    Operation op;
    char processName[PATH_MAX];
    char moduleName[PATH_MAX];
    pid_t pid;
    unsigned char setFlags;
    int syslogMask;
    unsigned int mask;
    unsigned int relativeOnMask;
    unsigned int relativeOffMask;
} Parameters;

static __inline void initParameters(Parameters * param) {
    memset(param, 0, sizeof (Parameters));
    param->relativeOffMask = (unsigned int) - 1;
}

static __inline int parseSyslogLevel(const char *param,int *newSyslogLevel) {
    int error = EXIT_SUCCESS;
    if (isdigit(param[0])) { /* 0x... (hexa value) ) 0.. (octal value or zero) 1 2 3 4 5 6 7 8 9 (decimal value) */
        char *endptr = NULL;
        *newSyslogLevel = strtoul(param, &endptr, 0);
        if (endptr != param) {
            if (*newSyslogLevel > LOG_UPTO(LOG_DEBUG)) {
                error = EINVAL;
                ERROR_MSG("bad syslog value %d", *newSyslogLevel);
            }
            DEBUG_VAR(*newSyslogLevel, "%d");
        } else {
            error = EINVAL;
        }
    } else if (strcasecmp(param, "debug") == 0) {
        *newSyslogLevel = LOG_UPTO(LOG_DEBUG);
        DEBUG_VAR(*newSyslogLevel, "%d");
    } else if (strcasecmp(param, "info") == 0) {
        *newSyslogLevel = LOG_UPTO(LOG_INFO);
        DEBUG_VAR(*newSyslogLevel, "%d");
    } else if (strcasecmp(param, "notice") == 0) {
        *newSyslogLevel = LOG_UPTO(LOG_NOTICE);
        DEBUG_VAR(*newSyslogLevel, "%d");
    } else if (strcasecmp(param, "warning") == 0) {
        *newSyslogLevel = LOG_UPTO(LOG_WARNING);
        DEBUG_VAR(*newSyslogLevel, "%d");
    } else if (strcasecmp(param, "error") == 0) {
        *newSyslogLevel = LOG_UPTO(LOG_ERR);
        DEBUG_VAR(*newSyslogLevel, "%d");
    } else if (strcasecmp(param, "critical") == 0) {
        *newSyslogLevel = LOG_UPTO(LOG_CRIT);
        DEBUG_VAR(*newSyslogLevel, "%d");
    } else if (strcasecmp(param, "alert") == 0) {
        *newSyslogLevel = LOG_UPTO(LOG_ALERT);
        DEBUG_VAR(*newSyslogLevel, "%d");
    }/*else if (strcasecmp(param,"emergency") == 0) emergency message are not maskable
				   {
				   *newSyslogLevel = LOG_EMERG;
				   DEBUG_VAR(*newSyslogLevel,"%d");
				   } */
    else {
#ifdef DEBUG
        if (isalnum(param[0])) {
            DEBUG_VAR(param, "%s");
        } else {
            DEBUG_VAR(param[0], "%d");
        }
#endif				/* DEBUG */
        error = EINVAL;
    }
    return error;
}

static __inline int parseFlags(int argc, char *argv[],const unsigned int negative,unsigned int *relativeValue,int *currentPosition) {
    int error = EINVAL;
    unsigned int allRead = 0;
    int currentItem = *currentPosition;

    while ((0 == allRead) && (currentItem < argc)) {
        const char *currentArg = argv[currentItem];

        DEBUG_VAR(currentItem, "%d");
        DEBUG_VAR(currentArg, "%s");
        if (isdigit(currentArg[0])) {
            const unsigned int n = atoi(currentArg);
            const unsigned int mask = 1 << n;

            DEBUG_VAR(n, "%d");
            if (n <= 32) {
                if (negative) {
                    const unsigned int negativeMask = ~mask;
                    *relativeValue &= negativeMask;
                } else {
                    *relativeValue |= mask;
                }
                DEBUG_VAR(*relativeValue, "0x%X");
                error = EXIT_SUCCESS;
                currentItem++;
            } else {
                error = EINVAL;
                allRead = 1;
            }
        }/* (isdigit(currentArg[0])) */
        else {
            allRead = 1;
        }
    } /* while (0 == allRead) */

    *currentPosition = currentItem;
    DEBUG_VAR(currentItem, "%d");

    return error;
}

static __inline int getOptionShort(int argc, char *argv[], const struct option *longopts, int *currentPosition, const char **optArg) {
    int currentOption = -1;
    const struct option *longoptsCursor = longopts;
    int pos = *currentPosition;
    const char currentOptionName = argv[pos][1];

    while (longoptsCursor->name != NULL) {
        if (currentOptionName == longoptsCursor->val) {
            currentOption = currentOptionName;
            *currentPosition = pos + 1; /* move to the next position */
            if (longoptsCursor->has_arg != no_argument) {
                /* check the parameter's position (if any) */
                if ('\0' == argv[pos][2]) {
                    /* if there is a parameter, it's the next one */
                    pos++;
                    if ((pos < argc) && (argv[pos][0] != '-')) {
                        *optArg = argv[pos];
                        *currentPosition = pos + 1;
                    }
                }/* ('\0' == argv[pos][2]) */
                else {
                    *optArg = argv[pos] + 2;
                }

                if ((required_argument == longoptsCursor->has_arg) && (NULL == *optArg)) {
                    currentOption = -1; /* error */
                }
            } /* (longoptsCursor->has_arg != no_argument) */
            break;
        }/* (currentOptionName == longoptsCursor->val) */
        else {
            longoptsCursor++;
        }
    } /*  while (longoptsCursor->name != NULL) */
    return currentOption;
}

static __inline int getOptionLong(int argc, char *argv[], const struct option *longopts, int *currentPosition, const char **optArg) {
    int currentOption = -1;
    int pos = *currentPosition;
    const struct option *longoptsCursor = longopts;
    const char *currentOptionName = argv[pos] + 2;
    const char *parameter = strchr(currentOptionName, '=');
    int n = 0;

    if (parameter != NULL) {
        n = parameter - currentOptionName;
    } else {
        n = strlen(currentOptionName);
    }

    while (longoptsCursor->name != NULL) {
        if (strncmp(currentOptionName, longoptsCursor->name, n) == 0) {
            currentOption = longoptsCursor->val;
            *currentPosition = pos + 1;

            if (longoptsCursor->has_arg != no_argument) {
                if (parameter != NULL) {
                    *optArg = parameter + 1;
                } else if (*currentPosition < argc) {
                    if (argv[*currentPosition][0] != '-') {
                        *optArg = argv[*currentPosition];
                        *currentPosition = pos + 2;
                    } else if (required_argument ==
                            longoptsCursor->has_arg) {
                        currentOption = -1; /* error */
                    }
                } else if (required_argument ==
                        longoptsCursor->has_arg) {
                    currentOption = -1; /* error */
                }
            } /* (longoptsCursor->has_arg != no_argument) */
            break;
        } else { /* (strncmp(currentOptionName, longoptsCursor->name, n) != 0)*/
            longoptsCursor++;
        }
    } /* while (longoptsCursor->name != NULL) */

    return currentOption;
}

static __inline int getOption(int argc, char *argv[],const struct option *longopts,int *currentPosition, const char **optArg) {
    int currentOption = -1;
    int pos = *currentPosition;

    DEBUG_VAR(*currentPosition, "%d");
    *optArg = NULL;
    if (pos < argc) {
        if ('-' == argv[pos][0]) {
            if (argv[pos][1] != '-') {
                /*
                 * short option syntax
                 */
                currentOption = getOptionShort(argc, argv, longopts, currentPosition, optArg);
            } else { /* (argv[pos][1] != '-') */
                /*
                 * long option syntax
                 */
                currentOption = getOptionLong(argc, argv, longopts, currentPosition, optArg);
            } /* ! (argv[currentPosition][1] != '-') */
        }/* ('-' == argv[pos][0]) */
        else {
            currentOption = '?';
            fprintf(stderr, "syntax error %s is not a valid option\n",
                    argv[pos]);
        }
    } /* (pos < argc) */

    DEBUG_VAR(*currentPosition, "%d");
    DEBUG_VAR(currentOption, "%c");
    return currentOption;
}

static __inline int parseCmdLine(int argc, char *argv[], Parameters * parameters) {
    int error = EXIT_SUCCESS;
    int currentPosition = 1; /* skip the first argv value (program name) */
    int optc;
    const char *optArg = NULL;

    while (((optc =
            getOption(argc, argv, longopts, &currentPosition,
            &optArg)) != -1) && (EXIT_SUCCESS == error)) {
        char errorMsg[100];

        switch (optc) {
            case 'p':
                strcpy(parameters->processName, optArg);
                DEBUG_VAR(parameters->processName, "%s");
                //currentPosition++;
                break;
            case 'l': /* list possible target */
                if (optArg != NULL) {
                    strcpy(parameters->moduleName, optArg);
                    DEBUG_VAR(parameters->moduleName, "%s");
                    //currentPosition++;
                }
                parameters->op = op_list;
                break;
            case 'm': /* define the module name of the target */
                strcpy(parameters->moduleName, optArg);
                DEBUG_VAR(parameters->moduleName, "%s");
                //currentPosition++;
                break;
            case 'i': /* define the process id of the target */
                parameters->pid = strtoul(optArg, NULL, 0);
                DEBUG_VAR(parameters->pid, "%d");
                //currentPosition++;
                break;
            case 'o': /* set following dbgflags on */
                parameters->setFlags |= relativeOnMaskSet;
                error =
                        parseFlags(argc, argv, 0, &parameters->relativeOnMask,
                        &currentPosition);
                if (EXIT_SUCCESS == error) {
                    DEBUG_VAR(parameters->relativeOnMask, "0x%X");
                    DEBUG_VAR(parameters->setFlags, "0x%X");
                    parameters->op = op_write;
                } else {
                    sprintf(errorMsg, "invalid dbgflags parameter (%s)",
                            optArg);
                    error = EINVAL;
                    printHelp(errorMsg);
                }
                break;
            case 'f': /* set following dbgflags off */
                parameters->setFlags |= relativeOffMaskSet;
                error =
                        parseFlags(argc, argv, 1, &parameters->relativeOffMask,
                        &currentPosition);
                if (EXIT_SUCCESS == error) {
                    DEBUG_VAR(parameters->relativeOffMask, "0x%X");
                    DEBUG_VAR(parameters->setFlags, "0x%X");
                    parameters->op = op_write;
                } else {
                    sprintf(errorMsg, "invalid dbgflags parameter (%s)",
                            optArg);
                    error = EINVAL;
                    printHelp(errorMsg);
                }
                break;
            case 'g': /* set new dbgflags value */
            {
                char *endptr = NULL;
                parameters->mask = strtoul(optArg, &endptr, 0);
                //currentPosition++;
                if (endptr != optArg) {
                    DEBUG_VAR(parameters->mask, "%d");
                    parameters->setFlags |= maskSet;
                    DEBUG_VAR(parameters->setFlags, "0x%X");
                    parameters->op = op_write;
                } else {
                    sprintf(errorMsg, "invalid dbgflags value (%s)",
                            optArg);
                    error = EINVAL;
                    printHelp(errorMsg);
                }
            }
                break;
            case 's': /* set new syslog log level */
                error = parseSyslogLevel(optArg, &parameters->syslogMask);
                if (EXIT_SUCCESS == error) {
                    parameters->setFlags |= syslogMaskSet;
                    DEBUG_VAR(parameters->setFlags, "0x%X");
                    parameters->op = op_write;
                } else {
                    sprintf(errorMsg, "invalid sysloglevel value (%s)",
                            optArg);
                    printHelp(errorMsg);
                }
                break;
            case 'c': /* custom command */
                break;
            case 'h':
                printHelp(NULL);
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                printVersion();
                exit(EXIT_SUCCESS);
                break;
            case '?':
                /* getopt_long has already printed an error message.  */
                error = EINVAL;
                printHelp("");
                break;
            default:
                error = EINVAL;
                break;
        } /* switch (optc) */
        DEBUG_VAR(currentPosition, "%d");
    } /* while ((optc = getopt_long (argc, argv, " */

    return error;
}

static __inline void initClientParameters(ClientParameters * param) {
    memset(param, 0, sizeof (ClientParameters));
}

static __inline int readDbgFlags(Parameters * parameters) {
    int error = EXIT_SUCCESS;
    ClientParameters clientParameters;

    initClientParameters(&clientParameters);
    clientParameters.processName = parameters->processName;
    clientParameters.moduleName = parameters->moduleName;
    clientParameters.pid = parameters->pid;
    clientParameters.comp = contains;
    clientParameters.command = eGet;
    error = UDSTCPClient(&clientParameters);
    if (EXIT_SUCCESS == error) {
        displayFulldebugFlags(stdout,
                &clientParameters.param.getCmd.fullDbgFlags);
        if ('\0' == clientParameters.moduleName[0]) {
            clientParameters.command = eLsLib;
            error = UDSTCPClient(&clientParameters);
        } else {
            DEBUG_VAR(clientParameters.moduleName, "%s");
        }
    }

    return error;
}

static __inline int setDbgFlagsValues(Parameters * parameters) {
    int error = EXIT_SUCCESS;
    ClientParameters clientParameters;

    initClientParameters(&clientParameters);
    clientParameters.processName = parameters->processName;
    clientParameters.moduleName = parameters->moduleName;
    clientParameters.pid = parameters->pid;
    clientParameters.comp = contains;

    if ((parameters->setFlags & relativeOnMaskSet)
            || (parameters->setFlags & relativeOffMaskSet)
            && !(parameters->setFlags & maskSet)) {
        clientParameters.command = eGet;
        error = UDSTCPClient(&clientParameters);
    }

    if (EXIT_SUCCESS == error) {
        clientParameters.command = eSet;
        if (parameters->setFlags & syslogMaskSet) {
            clientParameters.param.setCmd.setFlags |= newSyslogMaskSet;
            clientParameters.param.setCmd.newSyslogMask =
                    parameters->syslogMask;
        }

        if ((parameters->setFlags & relativeOnMaskSet)
                || (parameters->setFlags & relativeOffMaskSet)) {
            clientParameters.param.setCmd.setFlags |= newMaskValueSet;
            clientParameters.param.setCmd.newMaskValue =
                    clientParameters.param.getCmd.fullDbgFlags.dbgFlags.mask;
            DEBUG_VAR(clientParameters.param.getCmd.fullDbgFlags.dbgFlags.
                    mask, "0x%X");
            if (parameters->setFlags & relativeOnMaskSet) {
                DEBUG_VAR(parameters->relativeOnMask, "0x%X");
                clientParameters.param.setCmd.newMaskValue |=
                        parameters->relativeOnMask;
                DEBUG_VAR(clientParameters.param.setCmd.newMaskValue,
                        "0x%X");
            }

            if (parameters->setFlags & relativeOffMaskSet) {
                //const unsigned int negativeRelativeOffMask = ~parameters->relativeOffMask;
                DEBUG_VAR(parameters->relativeOffMask, "0x%X");
                //DEBUG_VAR(negativeRelativeOffMask,"0x%X");
                //clientParameters.param.setCmd.newMaskValue &= negativeRelativeOffMask;
                clientParameters.param.setCmd.newMaskValue &=
                        parameters->relativeOffMask;
                DEBUG_VAR(clientParameters.param.setCmd.newMaskValue,
                        "0x%X");
            }
        }

        if (parameters->setFlags & maskSet) {
            clientParameters.param.setCmd.setFlags |= newMaskValueSet;
            clientParameters.param.setCmd.newMaskValue = parameters->mask;
            DEBUG_VAR(clientParameters.param.setCmd.newMaskValue, "0x%X");
        }
        error = UDSTCPClient(&clientParameters);
    }
    return error;
}

int main(int argc, char *argv[]) {
    int error = EXIT_SUCCESS;
    Parameters parameters;

    initParameters(&parameters);
    error = parseCmdLine(argc, argv, &parameters);
    if (EXIT_SUCCESS == error) {
        switch (parameters.op) {
            case op_list:
                error = listPIDS(parameters.processName, contains, 1);
                break;
            case op_write:
                error = setDbgFlagsValues(&parameters);
                if (EXIT_SUCCESS == error) {
                    error = readDbgFlags(&parameters);
                }
                break;
            case op_read:
                error = readDbgFlags(&parameters);
                break;
        } /* switch(parameters.op) */
    }
    /* (EXIT_SUCCESS == error) */
    return error;
}

MODULE_NAME(PROGRAM_NAME);
PACKAGE_NAME_AUTOTOOLS;
MODULE_AUTHOR_AUTOTOOLS;
MODULE_VERSION(PROGRAM_VERSION);
MODULE_FILE_VERSION(1.1);
MODULE_DESCRIPTION(get / set dbgflags and log threshold value);
MODULE_COPYRIGHT(GPL);
