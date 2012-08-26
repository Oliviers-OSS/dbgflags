#include "config.h"
#include "dynamicLogger.h"
#include "utils.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <limits.h>
#include <ctype.h>
#include <dbgflags/syslogex.h>
#include <dbgflags-1/loggers.h>
#include <dbgflags/debug_macros.h>
#include <dbgflags/goodies.h>

#include "configurationFile.h"

#define MALLOC_GRANULARITY 10

#ifndef XMLCALL
#define XMLCALL
#endif /* XMLCALL */

static dynamicLoggerData currentLogger = {
    NULL /* configuration */
    , openlogex /* open */
    , syslogex /* logger */
    , vsyslogex /* vlogger */
    , setlogmaskex /*setLogMask*/
    , closelogex /* close */
#ifdef _GNU_SOURCE
    , PTHREAD_RWLOCK_INITIALIZER /*lock*/
#else /*  _GNU_SOURCE */
    , PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP/* lock*/
#endif /* _GNU_SOURCE */
};

static LoggersDirectory directory[] = {
    DECLARE_LOGGER(openlogex, syslogex, vsyslogex, setlogmaskex, closelogex)
    DECLARE_LOGGER(openlog, syslog, vsyslog, setlogmask, closelog)
    DECLARE_LOGGER(openConsoleLogger, consoleLogger, vconsoleLogger, setlogmask, NULL)
    DECLARE_LOGGER(openLogFile, fileLogger, vfileLogger, setlogmask, closeLogFile)
    {NULL, NULL, NULL, NULL, NULL, NULL}
};

#ifdef _GNU_SOURCE

static inline int getRead() {
    const int error = pthread_rwlock_rdlock(&currentLogger.lock);
    DEBUG_VAR(error, "%d");
    return error;
}

static inline int releaseRead() {
    const int error = pthread_rwlock_unlock(&currentLogger.lock);
    DEBUG_VAR(error, "%d");
    return error;
}

static inline int getWrite() {
    const int error = pthread_rwlock_wrlock(&currentLogger.lock);
    DEBUG_VAR(error, "%d");
    return error;
}

static inline int releaseWrite() {
    const int error = pthread_rwlock_unlock(&currentLogger.lock);
    DEBUG_VAR(error, "%d");
    return error;
}

#else /*  _GNU_SOURCE */

static inline int getRead() {
    const int error = pthread_mutex_lock(&currentLogger.lock);
    DEBUG_VAR(error, "%d");
    return error;
}

static inline int releaseRead() {
    const int error = pthread_mutex_unlock(&currentLogger.lock);
    DEBUG_VAR(error, "%d");
    return error;
}

static inline int getWrite() {
    const int error = pthread_mutex_lock(&currentLogger.lock);
    DEBUG_VAR(error, "%d");
    return error;
}

static inline int releaseWrite() {
    const int error = pthread_mutex_unlock(&currentLogger.lock);
    DEBUG_VAR(error, "%d");
    return error;
}
#endif /* _GNU_SOURCE */

static const struct option longopts[] = {
    {"tracer", required_argument, NULL, 't'},
    {"facility", required_argument, NULL, 'f'},
    {"log-level", required_argument, NULL, 'l'},
    {"log-up-to-level", required_argument, NULL, 'L'},
    {"log-directory", required_argument, NULL, 'd'},
    {"log-max-size", required_argument, NULL, 'm'},
    {"log-max-duration", required_argument, NULL, 'D'},
    { "log-flipflop", no_argument, NULL, 'F'},
    { "log-histo", no_argument, NULL, 'H'},
    { "log-configuration-file", required_argument, NULL, 'c'},
    { "initial-flags", required_argument, NULL, 'i'},
    /* TIS compatibility parameters */
    { "LogLevel", required_argument, NULL, 'x'},
    { "LogPath", required_argument, NULL, 'y'},
    { NULL, 0, NULL, 0}
};

const char* openLoggerFromCmdLineHelp(void) {
#define PARAM_COMMENT "\t"
#define PARAM_EOL "\n"
    static const char *parameters =
            "-t <logger> | --tracer=<logger>"PARAM_COMMENT"to set the logger"PARAM_EOL
            "-f <facility> | --facility=<facility>"PARAM_COMMENT"to set the facility"PARAM_EOL
            "-l <level> | --log-level=<level>"PARAM_COMMENT"to set the log level mask"PARAM_EOL
            "-L <level> | --log-up-to-level=<level>"PARAM_COMMENT"to set the log level threshold"PARAM_EOL
            "-d <directory> | --log-directory=<directory>"PARAM_COMMENT"to set the directory to store log files"PARAM_EOL
            "-m <size> [MB|Mo|kB|ko|b|o]|--log-max-size=<size>[MB|Mo|kB|ko|b|o]"PARAM_COMMENT"to limit a log files'size"PARAM_EOL
            "-D <duration> | --log-max-duration=<duration>"PARAM_COMMENT"to set the maximum duration of a log files, valid format are jj hh:mn:ss, hh:mn:ss, nn [h|mn|s]"PARAM_EOL
            "-F | --log-flipflop"PARAM_COMMENT"to log to 2 files"PARAM_EOL
            "-H | --log-histo"PARAM_COMMENT"to log to a set of files"PARAM_EOL
            "-c <file> | --log-configuration-file=<file>"PARAM_COMMENT"to set the configuration file used by the dynamic logger"PARAM_EOL
            "-i=<mask> |--initial-flags=<mask>"PARAM_COMMENT"to set the initial dbgflags mask value"PARAM_EOL;
    return parameters;
}

static inline int checkLoggerParam(const char *loggerName) {
    int error = EINVAL;
    LoggersDirectory *cursor = directory;
    while (cursor->Name != NULL) {
        if (strcasecmp(cursor->Name, loggerName) != 0) {
            cursor++;
        } else {
            error = EXIT_SUCCESS;
            break;
        }
    } /* while (cursor->Name != NULL) */
    return error;
}

static int getConfigurationFileParameters(const char *filename, dynamicLoggerConfiguration *configuration, unsigned int *flags) {
    int error = EXIT_SUCCESS;
    configuration_file_data data;
    configuration_file_data_init(&data);
    error = readConfigurationFile(filename, &data);
    if (EXIT_SUCCESS == error) {
        const char *logger_section = "logger";
        const char *fileLogger_section = "fileLogger";
        const char *value = configuration_file_get_value(&data, logger_section, "logger", "");
        if (value[0] != '\0') {
            error = checkLoggerParam(value);
            if (EXIT_SUCCESS == error) {
                strncpy(configuration->logger, value, sizeof (configuration->logger) / sizeof (configuration->logger[0]));
            } else {
                WARNING_MSG("unknown logger %s", value);
            }
        }

        value = configuration_file_get_value(&data, logger_section, "facility", "");
        if (value[0] != '\0') {
            const int facility = stringToFacility(value);
            if (facility != -1) {
                configuration->logfac = facility;
            } else {
                WARNING_MSG("invalid facility value %s read from configuration file %s", value, filename);
            }
        }

        value = configuration_file_get_value(&data, logger_section, "level", "");
        if (value[0] != '\0') {
            const int level = stringToSyslogLevel(value);
            if (level != -1) {
                setlogmask(LOG_UPTO(level));
            } else {
                WARNING_MSG("invalid level value %s read from configuration file %s", value, filename);
            }
        }

        value = configuration_file_get_value(&data, logger_section, "flags", "");
        if (value[0] != '\0') {
            *flags = strtoul(value, NULL, 0);
        }

        /*
         * if the directory parameter is set using an environment variable
         * then it will be resolved by configuration_file_get_value
         * or if not yet set, when used (cf. setFileLoggerDirectory).
         */
        value = configuration_file_get_value(&data, fileLogger_section, "directory", "");
        if (value[0] != '\0') {
            strncpy(configuration->directory, value, PATH_MAX);
        }

        value = configuration_file_get_value(&data, "file", "maxsize", "");
        if (value[0] != '\0') {
            const size_t size = parseSize(value);
            if (size != 0) {
                configuration->maxSizeInBytes = size;
            } else {
                WARNING_MSG("invalid maxsize value %s read from configuration file %s", value, filename);
            }
        }

        value = configuration_file_get_value(&data, fileLogger_section, "maxduration", "");
        if (value[0] != '\0') {
            const time_t duration = parseDuration(value);
            if (duration != 0) {
                configuration->maxTimeInSeconds = duration;
                configuration->logstat &= LOG_FILE_DURATION;
            } else {
                WARNING_MSG("invalid duration value %s read from configuration file %s", value, filename);
            }
        }

        value = configuration_file_get_value(&data, fileLogger_section, "management", "");
        if (value[0] != '\0') {
           if ((strcasecmp(value, "flip-flop") == 0) || (strcasecmp(value, "flipflop") == 0) || (strcasecmp(value, "rotate") == 0)) {
                configuration->logstat &= LOG_FILE_ROTATE;
            } else if (strcasecmp(value, "history") == 0) {
                configuration->logstat &= LOG_FILE_HISTO;
            }
        }
    } /* (EXIT_SUCCESS == error) */
    /* error already printed */

    configuration_file_data_free(&data);
    return error;
}

static inline int stringTISLogLevelToSyslogLevel(const char *tislogLevel) {
    int syslogLevelValue = -1;
    /* value is non case sensitive */
    if (strcasecmp(tislogLevel, "debug") != 0) {
        syslogLevelValue = LOG_DEBUG;
    } else if (strcasecmp(tislogLevel, "info") != 0) {
        syslogLevelValue = LOG_NOTICE;
    } else if (strcasecmp(tislogLevel, "warning") != 0) {
        syslogLevelValue = LOG_WARNING;
    } else if (strcasecmp(tislogLevel, "error") != 0) {
        syslogLevelValue = LOG_ERR;
    } else if (strcasecmp(tislogLevel, "error") != 0) {
        syslogLevelValue = LOG_CRIT;
    } else {
        DEBUG_MSG("invalid TIS Log Level %s", tislogLevel);
    }
    return syslogLevelValue;
}

int dynamicLoggerParseCmdLineElement(dynamicLoggerConfiguration *configuration, unsigned int *flags, const int optc, const char *optarg) {
    int error = EXIT_SUCCESS;
    switch (optc) {
        case 't': /* logger (tracer) */
            error = checkLoggerParam(optarg);
            if (EXIT_SUCCESS == error) {
                strncpy(configuration->logger, optarg, sizeof (configuration->logger) / sizeof (configuration->logger[0]));
            } else {
                WARNING_MSG("unknown logger %s", optarg);
            }
            break;
        case 'f':
        { /* facility */
            const int facility = stringToFacility(optarg);
            if (facility != -1) {
                configuration->logfac = facility;
            } else {
                error = EINVAL; /* error already printed */
            }
        }
            break;
        case 'l':
        { /* log-level */
            const int level = stringToSyslogLevel(optarg);
            if (level != -1) {
                setlogmask(level);
            } else {
                error = EINVAL; /* error already printed */
            }
        }
            break;
        case 'L':
        { /* log-up-to-level */
            const int level = stringToSyslogLevel(optarg);
            if (level != -1) {
                setlogmask(LOG_UPTO(level));
            } else {
                error = EINVAL; /* error already printed */
            }
        }
            break;
        case 'd':
        { /* log-directory */
            strncpy(configuration->directory, optarg, PATH_MAX);
        }
            break;
        case 'm':
        { /* log-max-size */
            const size_t size = parseSize(optarg);
            if (size != 0) {
                configuration->maxSizeInBytes = size;
            } else {
                error = EINVAL; /* error already printed */
            }
        }
            break;
        case 'D':
        { /* log-max-duration */
            const size_t duration = parseDuration(optarg);
            if (duration != 0) {
                configuration->maxTimeInSeconds = duration;
            } else {
                error = EINVAL; /* error already printed */
            }
        }
            break;
        case 'F':
        { /* log-flipflop */
            configuration->logstat |= LOG_FILE_ROTATE;
        }
            break;
        case 'H':
        { /* log-histo */
            configuration->logstat |= LOG_FILE_HISTO;
        }
            break;
        case 'c':
        { /* log-configuration-file */
            error = getConfigurationFileParameters(optarg, configuration, flags);
        }
            break;
        case 'i':
            if (flags != NULL) {
                *flags = strtoul(optarg, NULL, 0);
            } else {
                WARNING_MSG("initial flags value is set (%s) but flags parameters is NULL", optarg);
            }
            break;
            /* TIS compatibility parameters */
        case 'x': /* LogLevel */
        { /* log-up-to-level */
            const int level = stringTISLogLevelToSyslogLevel(optarg);
            if (level != -1) {
                setlogmask(LOG_UPTO(level));
            } else {
                error = EINVAL; /* error already printed */
            }
        }
            break;
        case 'y': /* LogPath */
        { /* log-directory */
            strncpy(configuration->directory, optarg, PATH_MAX);
        }
            break;
        default:
            DEBUG_MSG("parameter %c ignored", optc);
            break;
    } /* switch (optc) */
    return error;
}

static inline int parseCmdLine(int argc, const char *argv[], dynamicLoggerConfiguration *configuration, unsigned int *flags) {
    int error = EXIT_SUCCESS;
    int optc;
    opterr = 0; /* to prevent the error message print for others parameters */
    while (((optc = getopt_long(argc, (char *const *) argv, DYNAMIC_LOGGER_OPT_LOG_PARAMETERS, longopts, NULL)) != -1) /*&& (EXIT_SUCCESS == error)*/) {
        error = dynamicLoggerParseCmdLineElement(configuration, flags, optc, optarg);
    } /* while (((optc = getopt_long (argc, argv, "f:l:d:L:m:fc:", longopts, NULL)) != -1) && (EXIT_SUCCESS == error)) */
    return error;
}

static inline int getCommandLine(char *cmdLine, int *argc, const char **argv[]) {
    int i = 0;
    register const char **arguments = NULL;
    int error = getcmdLine(cmdLine);

    if (EXIT_SUCCESS == error) {
        DEBUG_VAR(cmdLine, "%s");
        arguments = malloc(sizeof (char*) * MALLOC_GRANULARITY);
        if (arguments != NULL) {
            size_t allocated_size = MALLOC_GRANULARITY;
            const char *ptr = cmdLine;
            while (*ptr != '\0') {
                arguments[i] = ptr;
                DEBUG_MSG("argv[%d]=%s\n", i, arguments[i]);
                ptr += strlen(ptr) + 1;
                i++;
                if (i > allocated_size) {
                    const size_t new_allocated_size = MALLOC_GRANULARITY + allocated_size;
                    const char **new = NULL;
                    new = (const char **) realloc(arguments, sizeof (char*) * new_allocated_size);
                    if (new != NULL) {
                        allocated_size = new_allocated_size;
                        arguments = new;
                        DEBUG_MSG("new allocated size %u bytes @ 0x%X\n", allocated_size, new);
                    } else { /* !(new != NULL) */
                        error = ENOMEM;
                        ERROR_MSG("%s: failed to reallocated %u bytes to %u bytes for argv\n", __FUNCTION__, allocated_size, new_allocated_size);
                        free(arguments);
                        arguments = NULL;
                        break;
                    }
                } /* (i > allocated_size) */
            } /* while(*ptr != '\0') */
        } else { /* !(**argv != NULL) */
            error = ENOMEM;
            ERROR_MSG("%s: failed to allocated %d bytes for argv\n", __FUNCTION__, (sizeof (char*) * MALLOC_GRANULARITY));
        }
    } //error already printed

    *argc = i;
    *argv = arguments;

    return error;
}

int setLoggerConfiguration(const dynamicLoggerConfiguration *configuration) {
    int error = EINVAL;
    if (configuration->size >= 4152) {
        error = getRead();
        if (EXIT_SUCCESS == error) {
            if (fileLogger == currentLogger.logger) {
                if (configuration->maxSizeInBytes > 0) {
                    setFileLoggerMaxSize(configuration->maxSizeInBytes);
                }
                if (configuration->directory[0] != '\0') {
                    error = setFileLoggerDirectory(configuration->directory);
                }
            }
            releaseRead();
        } /* error already printed */

    } else { /* !(configuration->size >= 4152) */
        ERROR_MSG("invalid configuration parameter (bad size %d)", configuration->size);
        error = EINVAL;
    }

    return error;
}

int openLogger(const dynamicLoggerConfiguration *configuration) {
    int error = EINVAL;
    LoggersDirectory *cursor = directory;

    if (configuration->size >= 4152) {
        while (cursor->Name != NULL) {
            if (strcasecmp(cursor->Name, configuration->logger) != 0) {
                cursor++;
            } else { /* (strcasecmp(cursor->Name,configuration->logger) == 0) */
                error = getWrite();
                if (EXIT_SUCCESS == error) {
                    currentLogger.configuration = configuration;
                    currentLogger.open = cursor->Open;
                    currentLogger.logger = cursor->logger;
                    currentLogger.vlogger = cursor->vlogger;
                    currentLogger.setMask = cursor->setMask;
                    currentLogger.close = cursor->Close;
                    releaseWrite();

                    if (currentLogger.open != NULL) {
                        error = getRead();
                        if (EXIT_SUCCESS == error) {
                            (*currentLogger.open) (configuration->ident, configuration->logstat, configuration->logfac);
                            releaseRead();
                        }
                    }

                    error = setLoggerConfiguration(configuration);
                } /* (EXIT_SUCCESS == error) */
                break;
            } /* !(strcasecmp(cursor->Name,configuration->logger) == 0) */
        } /* while (cursor->Name != NULL) */
    } else { /* !(configuration->size >= 4152) */
        ERROR_MSG("invalid configuration parameter (bad size %d)", configuration->size);
    }
    return error;
}

int openLoggerFromCmdLine(dynamicLoggerConfiguration *configuration, unsigned int *flags) {
    int error = EXIT_SUCCESS;
    if (configuration->size >= 4152) {
        const size_t memorySize = 16 * 1024;
        char *cmdLine = malloc(memorySize);
        if (cmdLine != NULL) {
            int argc;
            const char **argv = NULL;

            error = getCommandLine(cmdLine, &argc, &argv);
            if (EXIT_SUCCESS == error) {
                error = parseCmdLine(argc, argv, configuration, flags);
                /*if (EXIT_SUCCESS == error) no fatal error*/
                {
                    error = openLogger(configuration);
                } /*else {
                exit(-error);
         }*/
            }

            if (argv != NULL) {
                free(argv);
                argv = NULL;
            }

            free(cmdLine);
            cmdLine = NULL;
        } else {
            error = ENOMEM;
            ERROR_MSG("failed to allocate %d bytes for the command line", memorySize);
        }
    } else { /* !(configuration->size >= 4152) */
        ERROR_MSG("invalid configuration parameter (bad size %d)", configuration->size);
    }
    return error;
}

#if DBGFLAGS_INTERFACE_VERSION <= 11

int openLoggerFromConfigurationFile(const char *filename, dynamicLoggerConfiguration *configuration, unsigned int *flags) {
    int error = getConfigurationFileParameters(filename, configuration, flags);
    /*if (EXIT_SUCCESS == error) no fatal error*/
    {
        error = openLogger(configuration);
    } /*else {
     exit(-error);
   }*/
    return error;
}
#endif

void dynamicLogger(int priority, const char *format, ...) {
    const int error = getRead();
    if (EXIT_SUCCESS == error) {
        if (currentLogger.vlogger != NULL) {
            va_list optional_arguments;
            va_start(optional_arguments, format);
            (*currentLogger.vlogger)(priority, format, optional_arguments);
            va_end(optional_arguments);
        }
        releaseRead();
    } /* error already printed */
}

void vdynamicLogger(int priority, const char *format, va_list optional_arguments) {
    const int error = getRead();
    if (EXIT_SUCCESS == error) {
        if (currentLogger.vlogger != NULL) {
            (*currentLogger.vlogger)(priority, format, optional_arguments);
        }
        releaseRead();
    } /* error already printed */
}

void closeLogger(void) {
    const int error = getRead();
    if (EXIT_SUCCESS == error) {
        if (currentLogger.close != NULL) {
            (*currentLogger.close)();
        }
        releaseRead();
    } /* error already printed */
}

#if DBGFLAGS_INTERFACE_VERSION >= 12
#if HAVE_LIBEXPAT 
#include <expat.h>

#define MAX_NODE_SIZE      1024
#define BUFFER_SIZE        8192
#define MAX_XML_DEPTH	     10
#define MAX_ATTRIBUT_LENGTH 128
#define SEPARATOR            '/'
#define DEFAULT_ROOT         "traces"

struct XMLData {
    char fullNodeName[MAX_NODE_SIZE];
    char *nodes[MAX_XML_DEPTH];
    unsigned int Depth;
    dynamicLoggerConfiguration *configuration;
    unsigned int *flags;
    const char *filename;
    const char *rootNodePath;
    size_t rootNodePathLength;
    char separator;
} *xmlData = NULL;

static inline char *append(char *start, const char *node) {
    while (*node != '\0') {
        *start = *node;
        *start++;
        *node++;
    }
    *start = '\0';
    return start;
}

static inline void manageStartNode(const char *node) {
    /* manage full node's name */
    char **nodes = xmlData->nodes;
    if (likely(xmlData->Depth != 0)) {
        *nodes[xmlData->Depth] = xmlData->separator;
        nodes[xmlData->Depth + 1] = append(nodes[xmlData->Depth] + 1, node);
    } else {
        nodes[1] = append(nodes[0], node);
    }

    xmlData->Depth++;
    DEBUG_VAR(xmlData->Depth, "%d");
    DEBUG_VAR(xmlData->fullNodeName, "%s");
}

static inline const char* getNodeAttribut(const char **attributs, const char *name) {
    const char *value = NULL;
    int i;
    for (i = 0; attributs[i]; i += 2) {
        if (strcmp(attributs[i], name) == 0) {
            value = attributs[i + 1];
            break;
        }
    }
    return value;
}

/* 
<traces flags="0x1234">
  <dynamicLogger logger="consoleLogger" facility="user" level="info"/>
  <fileLogger directory="$HOME" maxsize="10 Mo" maxduration="1h"  management=""/> 
</traces>
 */

static inline void setGlobalsAttributs(const char **attributs) {
    int i;
    for (i = 0; attributs[i]; i += 2) {
        const char *value = attributs[i + 1];
        if (strcasecmp(attributs[i], "flags") == 0) {
            unsigned int *flags = xmlData->flags;
            *flags = strtoul(value, NULL, 0);
            DEBUG_MSG("flags=0x%X", *flags);
        } else if (strcasecmp(attributs[i], "level") == 0) {
            const int level = stringToSyslogLevel(value);
            if (level != -1) {
                setlogmask(LOG_UPTO(level));
                DEBUG_VAR(level, "%d");
            } else {
                WARNING_MSG("invalid level value %s read from configuration file %s", value, xmlData->filename);
            }
        } else {
            WARNING_MSG("unknown traces global attribut %s (value %s) in file %s", attributs[i], value, xmlData->filename);
        }
    } /* for (i = 0; attributs[i]; i += 2) */
}

static char* convertToList(const char *string, char *list) {
    register const char *src = string;
    register char *dst = list;
    while (*src != '\0') {
        if (isalpha(*src) || (*src == '_')) {
            *dst = *src;
        } else {
            *dst = '\0';
        }
        dst++;
        src++;
    }
    *dst = '\0';
    return list;
}

static inline void setDynamicLoggerAttributs(const char **attributs) {
    dynamicLoggerConfiguration *configuration = xmlData->configuration;
    int i;
    for (i = 0; attributs[i]; i += 2) {
        const char *value = attributs[i + 1];
        if (strcasecmp(attributs[i], "logger") == 0) {
            const int error = checkLoggerParam(value);
            if (EXIT_SUCCESS == error) {
                strncpy(configuration->logger, value, sizeof (xmlData->configuration->logger) / sizeof (xmlData->configuration->logger[0]));
                DEBUG_VAR(configuration->logger, "%s");
            } else {
                WARNING_MSG("unknown logger %s read from configuration file %s", value, xmlData->filename);
            }
        } else if (strcasecmp(attributs[i], "facility") == 0) {
            const int facility = stringToFacility(value);
            if (facility != -1) {
                configuration->logfac = facility;
                DEBUG_VAR(configuration->logfac, "%d");
            } else {
                WARNING_MSG("invalid facility value %s read from configuration file %s", value, xmlData->filename);
            }
        } else if (strcasecmp(attributs[i], "options") == 0) {
            char list[MAX_NODE_SIZE];
            configuration->logstat = parseFlagsOptions(convertToList(value, list));
            DEBUG_VAR(configuration->logstat, "%d");
        } else {
            WARNING_MSG("unknown traces dynamicLogger attribut %s (value %s) in file %s", attributs[i], value, xmlData->filename);
        }
    } /* for (i = 0; attributs[i]; i += 2) */
}

static inline void setFileLoggerAttributs(const char **attributs) {
    dynamicLoggerConfiguration *configuration = xmlData->configuration;
    int i;
    for (i = 0; attributs[i]; i += 2) {
        const char *value = attributs[i + 1];
        if (strcasecmp(attributs[i], "directory") == 0) {
            strncpy(configuration->directory, value, PATH_MAX);
        } else if ((strcmp(attributs[i], "maxsize") == 0) || (strcasecmp(attributs[i], "max_size") == 0)) {
            const size_t size = parseSize(value);
            if (size != 0) {
                configuration->maxSizeInBytes = size;
            } else {
                WARNING_MSG("invalid maxsize value %s read from configuration file %s", value, xmlData->filename);
            }
        } else if ((strcasecmp(attributs[i], "maxduration") == 0) || (strcasecmp(attributs[i], "max_duration") == 0)) {
            const time_t duration = parseDuration(value);
            if (duration != 0) {
                configuration->maxTimeInSeconds = duration;
                configuration->logstat &= LOG_FILE_DURATION;
            } else {
                WARNING_MSG("invalid duration value %s read from configuration file %s", value, xmlData->filename);
            }
        } else if ((strcasecmp(attributs[i], "management") == 0) || (strcasecmp(attributs[i], "strategy") == 0)) {
            if ((strcasecmp(value, "flip-flop") == 0) || (strcasecmp(value, "rotate") == 0)) {
                configuration->logstat &= LOG_FILE_ROTATE;
            } else if (strcasecmp(value, "history") == 0) {
                configuration->logstat &= LOG_FILE_HISTO;
            } else {
                WARNING_MSG("invalid management value %s read from configuration file %s", value, xmlData->filename);
            }
        } else {
            WARNING_MSG("unknown traces fileLogger attribut %s (value %s) in file %s", attributs[i], value, xmlData->filename);
        }
    } /* for (i = 0; attributs[i]; i += 2) */
}

static void XMLCALL startNodeHandler(void *data, const char *node, const char **attributs) {
    dynamicLoggerConfiguration *configuration = xmlData->configuration;
    /* manage full node's name */
    manageStartNode(node);

    /*
     * look for parameters to manage traces
     */

    /* check for subtree */
    if (strncasecmp(xmlData->rootNodePath, xmlData->fullNodeName, xmlData->rootNodePathLength) == 0) {
        /* get parameters from the subtree */
        if (xmlData->fullNodeName[xmlData->rootNodePathLength] == 0) {
            setGlobalsAttributs(attributs);
        } else if (strcasecmp("dynamicLogger", xmlData->fullNodeName + xmlData->rootNodePathLength + 1) == 0) {
            setDynamicLoggerAttributs(attributs);
        } else if (strcasecmp("fileLogger", xmlData->fullNodeName + xmlData->rootNodePathLength + 1) == 0) {
            setFileLoggerAttributs(attributs);
        }
    }
}

static void XMLCALL endNodeHandler(void *data, const char *node) {
    xmlData->Depth--;
}

static int getXMLConfigurationFileParameters(const configurationXMLFileParams *XMLFileParams, const char *filename, dynamicLoggerConfiguration *configuration, unsigned int *flags) {
    static pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;
    int error = pthread_mutex_lock(&fastmutex);
    if (0 == error) {
        int mutexError;
        int file = open(filename, O_RDONLY);
        if (file != -1) {
            XML_Parser parser = XML_ParserCreate(NULL);
            if (parser != NULL) {
                char Buffer[BUFFER_SIZE];
                ssize_t length = 0;
                int done = 0;

                /* initialisation */
                xmlData = (struct XMLData *) malloc(sizeof (struct XMLData));
                if (xmlData) {
                    memset(xmlData, 0, sizeof (struct XMLData));
                    xmlData->nodes[0] = xmlData->fullNodeName;
                    xmlData->configuration = configuration;
                    xmlData->flags = flags;
                    xmlData->filename = filename;
                    xmlData->rootNodePath = XMLFileParams->rootNodePath;
                    if (xmlData->rootNodePath != NULL) {
                        xmlData->rootNodePathLength = strlen(xmlData->rootNodePath);
                        xmlData->separator = XMLFileParams->separator;
                    } else {
                        xmlData->rootNodePath = DEFAULT_ROOT;
                        xmlData->rootNodePathLength = strlen(DEFAULT_ROOT);
                        xmlData->separator = SEPARATOR;
                    }

                    XML_SetElementHandler(parser, startNodeHandler, endNodeHandler);

                    /* run throught the file's content*/
                    while ((length = read(file, Buffer, sizeof (Buffer))) > 0) {
                        if (XML_Parse(parser, Buffer, length, done) == XML_STATUS_ERROR) {
                            enum XML_Error XMLError = XML_GetErrorCode(parser);
                            error = -1;
                            ERROR_MSG("XML Parse error at line %d in file:\n%s", XML_GetCurrentLineNumber(parser), filename, XML_ErrorString(XMLError));
                            break;
                        }
                    } /* while( (n = read(file,Buffer,sizeof(Buffer))) > 0) */

                    if (-1 == length) {
                        error = errno;
                        ERROR_MSG("read error %d in file %s (%m)", error, filename);
                    }

                    free(xmlData);
                    xmlData = NULL;
                } else { /* (xmlData) */
                    error = ENOMEM;
                    ERROR_MSG("Couldn't allocate memory for internal XML data");
                }
                XML_ParserFree(parser);
                parser = NULL;
            } else { /* (parser != NULL) */
                error = ENOMEM;
                ERROR_MSG("Couldn't allocate memory for the XML parser");
            }
            close(file);
            file = -1;
        } else {
            error = errno;
            ERROR_MSG("error %d opening file %s (%m)", error, filename);
        }
        mutexError = pthread_mutex_unlock(&fastmutex);
        if (mutexError != 0) {
            ERROR_MSG("pthread_mutex_lock error %d", mutexError);
        }
    } else { /* if (pthread_mutex_lock(&fastmutex) == 0) */
        ERROR_MSG("pthread_mutex_lock error %d", error);
    }
    return error;
}

#else /* HAVE_LIBEXPAT */
#define getXMLConfigurationFileParameters(params,filename,configuration,flags) ENOSYS /* Function not implemented */
#endif /* HAVE_LIBEXPAT */

/* 
 * C++ version 1.0 to avoid a compilation error using gcc 4.1.1-21
 */

typedef struct configurationFileParamsCpp_ {
    size_t size; /* size of the used dynamicLoggerConfiguration structure (=sizeof(ConfigurationFileParams)) */
    configurationFileType fileType;

    union {
        configurationXMLFileParams *XMLFile;
        char RFU[4];
    } params;
} configurationFileParamsCpp;

static inline int openLoggerFromConfigurationFileEx12(const configurationFileParamsCpp *configFileParams, const char *filename, dynamicLoggerConfiguration *configuration, unsigned int *flags) {
    int error = EXIT_SUCCESS;
    switch (configFileParams->fileType) {
        case iniFileType:
            error = getConfigurationFileParameters(filename, configuration, flags);
            break;
        case XMLFileType:
            error = getXMLConfigurationFileParameters(configFileParams->params.XMLFile, filename, configuration, flags);
            break;
        default:
            error = ENOSYS; /* Function not implemented */
    }
    return error;
}

/* 
 * C normal version 1.0
 */
static inline int openLoggerFromConfigurationFileEx16(const configurationFileParams *configFileParams, const char *filename, dynamicLoggerConfiguration *configuration, unsigned int *flags) {
    int error = EXIT_SUCCESS;
    switch (configFileParams->fileType) {
        case iniFileType:
            error = getConfigurationFileParameters(filename, configuration, flags);
            break;
        case XMLFileType:
            error = getXMLConfigurationFileParameters(&configFileParams->params.XMLFile, filename, configuration, flags);
            break;
        default:
            error = ENOSYS; /* Function not implemented */
    }
    return error;
}

int openLoggerFromConfigurationFileEx(const configurationFileParams *configFileParams, const char *filename, dynamicLoggerConfiguration *configuration, unsigned int *flags) {
    int error = EXIT_SUCCESS;
    if (configFileParams != NULL) {
        DEBUG_VAR(configFileParams->size, "%u");
        switch (configFileParams->size) {
            case 12:
                error = openLoggerFromConfigurationFileEx12((configurationFileParamsCpp*) configFileParams, filename, configuration, flags);
                break;
            case 16:
                error = openLoggerFromConfigurationFileEx16(configFileParams, filename, configuration, flags);
                break;
            default:
                error = ENOSYS; /* Function not implemented */
        } /* switch(configFileParams->size) */
    } else {
        error = getConfigurationFileParameters(filename, configuration, flags);
    }
    error = openLogger(configuration);
    return error;
}

#endif /* #if (DBGFLAGS_INTERFACE_VERSION >= 12) */

#include <dbgflags/ModuleVersionInfo.h>
MODULE_NAME(dynamicLogger);
MODULE_AUTHOR(Olivier Charloton);
MODULE_VERSION(1.1);
MODULE_FILE_VERSION(1.5);
MODULE_DESCRIPTION(dynamic logger to be able to set the logger at runtime);
MODULE_COPYRIGHT(LGPL);


