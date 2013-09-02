#define identity  "logproc"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <dbgflags/syslogex.h>
#include <dbgflags/loggers.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>

#ifndef TO_STRING
#define STRING(x) #x
#define TO_STRING(x) STRING(x)
#endif /* STRING */

#ifndef PACKAGE_NAME
#define PACKAGE_NAME identity
#endif /* PACKAGE_NAME */

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "1.0"
#endif /* PACKAGE_VERSION */

#define LOGGER consoleLogger
#define LOG_OPTS  0 /*LOG_CONS|LOG_PERROR|LOG_PID*/
#include <dbgflags/debug_macros.h>

#include <dbgflags/ModuleVersionInfo.h>

#ifndef COMMAND_LINE_SIZE
#define COMMAND_LINE_SIZE 2048
#endif /* COMMAND_LINE_SIZE */

typedef struct cmdLineParameters_ {
  int facility;
  int stdOutLogLevel;
  int stdErrLogLevel;
  int options;
  char cmdLine[COMMAND_LINE_SIZE];
} cmdLineParameters;

static __inline void printVersion(void) {
    printf(TO_STRING(PROGRAM_NAME) " v" TO_STRING(PROGRAM_VERSION) "\n");
}

static const struct option longopts[] = {
    {"facility",required_argument, NULL,'f'},
    {"stdout",required_argument, NULL,'o'},
    {"stderr",required_argument, NULL,'e'},
    { "help"      ,no_argument      , NULL, 'h' },
    { "version"   ,no_argument      , NULL, 'v' },
    { NULL, 0, NULL, 0 }
};

static __inline void printHelp(const char *errorMsg) {
#define USAGE  "Usage: " TO_STRING(PROGRAM_NAME) " [OPTIONS] <program> [<program's arguments>]\n" \
    "  -f, --facility                     \n"\
    "  -o, --stdout=<level>               \n"\
    "  -e, --stderr=<level>               \n"\
    "  -h, --help                        \n"\
    "  -v, --version                     \n"\
    "  Valid facility names are: auth, authpriv (for security information of a sensitive nature)" \
    ", cron, daemon, ftp, kern, lpr, mail, news, security (deprecated synonym for auth), syslog, "\
    "user, uucp, and local0 to local7, inclusive.\n"\
    "  Valid level names are): alert, crit, debug, emerg, err, error (deprecated synonym for err),"\
    " info, notice, panic (deprecated synonym for emerg), warning, warn (deprecated synonym for warning)."\
    "  For the priority order and intended purposes of these levels, see syslog(3).\n"

    if (errorMsg != NULL) {
        fprintf(stderr,"Error: %s" USAGE,errorMsg);
    } else {
        fprintf(stdout,USAGE);
    }

#undef USAGE
}

static inline int facilityLevel(const char *facilityString) {
    int facility = -1;
    if (isdigit(facilityString[0])) {
      char *endptr = NULL;
      const int n = strtoul(facilityString,&endptr,0);
      if (endptr != facilityString) {
        if (   ((n >= 0) && (n < 12)) 
            || ((n > 15) && (n < 24))) {
            facility = (n << 3);
        }
      }
    } else if (strcasecmp("user",facilityString) == 0) {
       facility = LOG_USER;
    } else if (strncasecmp("local",facilityString,5) == 0) {
       const int n = *(facilityString + 5) - '0';
       if ((n >=0) && (n < 8)) {
          facility = LOG_LOCAL0 + n;
      }
    } else if (strcasecmp("kern",facilityString) == 0) {
       facility = LOG_KERN;
    } else if (strcasecmp("mail",facilityString) == 0) {
       facility = LOG_MAIL;
    } else if (strcasecmp("daemon",facilityString) == 0) {
       facility = LOG_DAEMON;
    } else if (strcasecmp("auth",facilityString) == 0) {
       facility = LOG_AUTH;
    } else if (strcasecmp("syslog",facilityString) == 0) {
       facility = LOG_SYSLOG;
    } else if (strcasecmp("lpr",facilityString) == 0) {
       facility = LOG_LPR;
    } else if (strcasecmp("news",facilityString) == 0) {
       facility = LOG_NEWS;
    } else if (strcasecmp("uucp",facilityString) == 0) {
       facility = LOG_UUCP;
    } else if (strcasecmp("cron",facilityString) == 0) {
       facility = LOG_CRON;
    } else if (strcasecmp("authpriv",facilityString) == 0) {
       facility = LOG_AUTHPRIV;
    } else if (strcasecmp("ftp",facilityString) == 0) {
       facility = LOG_FTP;
    } else if (strcasecmp("security",facilityString) == 0) {
       facility = LOG_AUTH;
    }
    return facility;
}

static inline int logLevel(const char *levelString) {
   int level = -1;

   if (isdigit(levelString[0])) /* 0x... (hexa value) ) 0.. (octal value or zero) 1 2 3 4 5 6 7 8 9 (decimal value)*/ {
        char *endptr = NULL;
        const int l = strtoul(levelString, &endptr, 0);
        if (endptr != levelString) {
           if ((l >= LOG_EMERG) && (l <= LOG_DEBUG)) {
              level = l;
           }
        }
    } else if (strcasecmp(levelString, "debug") == 0) {
        level = LOG_DEBUG;
    } else if (strcasecmp(levelString, "info") == 0) {
        level = LOG_INFO;
    } else if (strcasecmp(levelString, "notice") == 0) {
        level = LOG_NOTICE;
    } else if (strcasecmp(levelString, "warning") == 0) {
        level = LOG_WARNING;
    } else if (strcasecmp(levelString, "error") == 0) {
        level = LOG_ERR;
    } else if (strcasecmp(levelString, "critical") == 0) {
        level = LOG_CRIT;
    } else if (strcasecmp(levelString, "alert") == 0) {
        level = LOG_ALERT;
    }  else if (strcasecmp(levelString, "emerg") == 0) {
        level = LOG_EMERG;
    }
    return level;
}

static inline int setExternalCmdLine(int argc, char *argv[],cmdLineParameters *parameters) {
  int error = EXIT_SUCCESS;
  int param;
  char *c = parameters->cmdLine;
  char pos = 0;
  for(param = optind; param < argc; param++) {
     pos = sprintf(c,"%s ",argv[param]);
     c += pos;
  }
  error = ('\0' == parameters->cmdLine[0]) ? EINVAL : EXIT_SUCCESS;
  return error;
}

static inline int parseCmdLine(int argc, char *argv[],cmdLineParameters *parameters) {
    int error = EXIT_SUCCESS;
    char errorMsg[100];
    int optc;

    while (((optc = getopt_long (argc, argv, "f:o:e:hv", longopts, NULL)) != -1) && (EXIT_SUCCESS == error)) {       
       int param;
       switch (optc)
       {
       case 'f':
         param = facilityLevel(optarg);
         if (param != -1) {
            parameters->facility = param;
         } else {
           sprintf(errorMsg,"invalid facility parameter (%s)",optarg);
           printHelp(errorMsg);
           error = EINVAL;
         }
         break;
       case 'o':
         param = logLevel(optarg);
         if (param != -1) {
            parameters->stdOutLogLevel = param;
         } else {
           sprintf(errorMsg,"invalid stdout log level parameter (%s)",optarg);
           printHelp(errorMsg);
           error = EINVAL;
         }
         break;
       case 'e':
         param = logLevel(optarg);
         if (param != -1) {
            parameters->stdErrLogLevel = param;
         } else {
           sprintf(errorMsg,"invalid stdout log level parameter (%s)",optarg);
           printHelp(errorMsg);
           error = EINVAL;
         }
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
    	 DEBUG_VAR(optind,"%d");
         /* getopt_long already printed an error message.  */
         sprintf(errorMsg,"invalid parameter (%s) %d",optarg,optind);
         error = EINVAL;
         printHelp("");
         break;
       default: 
         sprintf(errorMsg,"invalid parameter (%s)",optarg);
         printHelp(errorMsg);
         error = EINVAL;
         break;
       } /* switch (optc) */
    } /* while (((optc = getopt_long (argc, argv, "f:o:e:hv", longopts, NULL)) != -1) && (EXIT_SUCCESS == error)) */
    DEBUG_VAR(optind,"%d");

    /*if (EXIT_SUCCESS == error) {
       error = setExternalCmdLine(argc,argv,parameters);
       if (error != EXIT_SUCCESS) {
          sprintf(errorMsg,"invalid external program parameter (%s)",optarg);
          printHelp(errorMsg);
       }
    }*/
    return error;
}

int main(int argc, char *argv[]) {
  int error = EXIT_SUCCESS;
  cmdLineParameters params;

  openlogex(argv[0],LOG_CONS|LOG_PERROR|LOG_PID,LOG_USER);
  /* set default values */
  params.facility = LOG_USER;
  params.stdOutLogLevel = LOG_INFO;
  params.stdErrLogLevel = LOG_ERR;
  params.options = LOG_PID;
#ifdef _DEBUG_
  params.options |= LOG_CONS|LOG_PERROR;
#endif
  params.cmdLine[0] = '\0';

  error = parseCmdLine(argc,argv,&params);
  if (EXIT_SUCCESS == error) {
     if (optind < argc) {
        syslogproc(argv[optind],argv+optind,params.options,params.facility,params.stdOutLogLevel,params.stdErrLogLevel);
     } else {
       printHelp(NULL);
     }
  }
  closelogex();
  return error;
}

MODULE_NAME(PROGRAM_NAME);
PACKAGE_NAME_AUTOTOOLS;
MODULE_AUTHOR_AUTOTOOLS;
MODULE_VERSION(PROGRAM_VERSION);
MODULE_FILE_VERSION(1.2);
MODULE_DESCRIPTION(send process outputs to syslog);
MODULE_COPYRIGHT(GPL);
