#ifndef _LOGGERS_H_
#define _LOGGERS_H_

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <time.h>
#include <linux/limits.h>
#include <dbgflags/version.h>
#include <dbgflags/syslogex.h>

#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define CHECK_PARAMS(f,p)		__attribute__ ((format (printf, f, p)))
#ifndef PROFILING_HOOKS_ENABLED
#define NO_PROFILING __attribute__ ((no_instrument_function))
#else 
#define NO_PROFILING
#endif /*PROFILING_HOOKS_ENABLED*/
#if (GCC_VERSION > 40000) /* GCC 4.0.0 */
#define CHECK_NON_NULL_PTR(n)	__attribute__ ((nonnull(n)))
#else
#define CHECK_NON_NULL_PTR(n)
#endif /* (GCC_VERSION > 40000) */
#if (GCC_VERSION > 40600) /* GCC 4.6.0 */
#define DEPRECATED(x)    __attribute__((deprecated(x)))
#else
#define DEPRECATED(x)    __attribute__((deprecated))
#endif /* (GCC_VERSION > 40600) */
#else 
#define CHECK_PARAMS(f,p)
#define CHECK_NON_NULL_PTR(n)
#define DEPRECATED(x)
#endif /* __GNUC__ */

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * logger to console
 */
void openConsoleLogger(const char *ident, int logstat, int logfac) NO_PROFILING;
void setConsoleLoggerOpt(const char *ident, int logstat, int logfac) DEPRECATED("please use openConsoleLogger instead") NO_PROFILING;
void consoleLogger(int priority, const char *format, ...) CHECK_PARAMS(2,3) CHECK_NON_NULL_PTR(2) NO_PROFILING;
void vconsoleLogger(int priority, const char *format, va_list optional_arguments) CHECK_NON_NULL_PTR(2) NO_PROFILING;

/*
 * logger to files
 */
#define LOG_FILE_ROTATE     0x200
#define LOG_FILE_HISTO      0x400
#define LOG_FILE_DURATION  0x800
void openLogFile(const char *ident, int logstat, int logfac) NO_PROFILING;
void fileLogger(int priority, const char *format, ...) CHECK_PARAMS(2,3) CHECK_NON_NULL_PTR(2) NO_PROFILING;
void vfileLogger(int priority, const char *format, va_list optional_arguments) CHECK_NON_NULL_PTR(2) NO_PROFILING;
int setFileLoggerMaxSize(const size_t maxSizeInBytes) NO_PROFILING;
int setFileLoggerMaxDuration(const time_t maxDurationInSeconds) NO_PROFILING;
int setFileLoggerDirectory(const char *directory) CHECK_NON_NULL_PTR(1) NO_PROFILING;
void closeLogFile(void) NO_PROFILING;

/*
 * logger to files using AIO POSIX API
 */
void aioOpenLogFile(const char *ident, int logstat, int logfac) NO_PROFILING;
void aioFileLogger(int priority, const char *format, ...) CHECK_PARAMS(2,3) CHECK_NON_NULL_PTR(2) NO_PROFILING;
void aiovFileLogger(int priority, const char *format, va_list optional_arguments) CHECK_NON_NULL_PTR(2) NO_PROFILING;
int aioSetFileLoggerMaxSize(unsigned int maxSizeInBytes) NO_PROFILING;
int aioSetFileLoggerDirectory(const char *directory) CHECK_NON_NULL_PTR(1) NO_PROFILING;
void aioCloseLogFile(void) NO_PROFILING;

/*
 * logger set at runtime
 */
typedef struct dynamicLoggerConfiguration_ {
    size_t size; /* size of the used dynamicLoggerConfiguration structure (=sizeof(dynamicLoggerConfiguration)) */
    const char *ident;
    int logstat;
    int logfac;
    char directory[PATH_MAX];
    size_t maxSizeInBytes;
    time_t maxTimeInSeconds;
    char logger[32];
} dynamicLoggerConfiguration;

int openLogger(const dynamicLoggerConfiguration *configuration) CHECK_NON_NULL_PTR(1);
int openLoggerFromCmdLine(dynamicLoggerConfiguration *configuration,unsigned int *flags) CHECK_NON_NULL_PTR(1);
int setLoggerConfiguration(const dynamicLoggerConfiguration *configuration) CHECK_NON_NULL_PTR(1) NO_PROFILING;
void dynamicLogger(int priority, const char *format, ...) CHECK_PARAMS(2,3) CHECK_NON_NULL_PTR(2) NO_PROFILING;
void vdynamicLogger(int priority, const char *format, va_list optional_arguments) CHECK_NON_NULL_PTR(2) NO_PROFILING;
void closeLogger(void) NO_PROFILING;
const char* openLoggerFromCmdLineHelp(void) NO_PROFILING;
int dynamicLoggerParseCmdLineElement(dynamicLoggerConfiguration *configuration,unsigned int *flags,const int optc, const char *optarg) NO_PROFILING;
#define DYNAMIC_LOGGER_OPT_LOG_PARAMETERS "t:f:l:d:L:m:D:FHc:i:"

#if DBGFLAGS_INTERFACE_VERSION >= 12
typedef enum configurationFileType_ {
  iniFileType
 ,XMLFileType
} configurationFileType;

typedef struct configurationXMLFileParams_ {
  const char *rootNodePath;
  const char separator;
} configurationXMLFileParams;

/*
typedef union fileTypeParams_ {
  configurationXMLFileParams XMLFile;
  char RFU;
} fileTypeParams;*/

#ifndef __cplusplus
typedef struct configurationFileParams_ {
  size_t size; /* size of the used dynamicLoggerConfiguration structure (=sizeof(ConfigurationFileParams)) */
  configurationFileType fileType;
   union {
     configurationXMLFileParams XMLFile;
     char RFU[4];
  } params;
} configurationFileParams;

#else

typedef struct configurationFileParams_ {
  size_t size; /* size of the used dynamicLoggerConfiguration structure (=sizeof(ConfigurationFileParams)) */
  configurationFileType fileType;
   union {
     configurationXMLFileParams *XMLFile;
     char RFU[4];
  } params;
} configurationFileParams;
#endif /* __cplusplus */

#if 0
struct configurationXMLFileParams {
  const char *rootNodePath;
  const char separator;
  configurationXMLFileParams()
    :rootNodePath(0),separator('/'){
  }
  configurationXMLFileParams(const configurationXMLFileParams &src)
    :rootNodePath(src.rootNodePath),separator(src.separator) {
  }
  configurationXMLFileParams operator = (const struct configurationXMLFileParams &src) {
    if (this != &src) {
    }
    return *this;
  }
};

struct configurationFileParams {
  size_t size; /* size of the used dynamicLoggerConfiguration structure (=sizeof(ConfigurationFileParams)) */
  configurationFileType fileType;
   union {
     configurationXMLFileParams XMLFile;
     char RFU[4];
  } params;
  configurationFileParams() {
  }
  configurationFileParams(configurationFileParams &src) {
  }
  configurationFileParams operator = (const struct configurationFileParams &src) {
    return *this;
  }
} 
#endif /*__cplusplus*/

int openLoggerFromConfigurationFileEx(const configurationFileParams *configFileParams, const char *filename,dynamicLoggerConfiguration *configuration,unsigned int *flags) CHECK_NON_NULL_PTR(2) CHECK_NON_NULL_PTR(3) NO_PROFILING;
#define openLoggerFromConfigurationFile(filename,configuration,flags)  openLoggerFromConfigurationFileEx(NULL,filename,configuration,flags)
#else
int openLoggerFromConfigurationFile(const char *filename,dynamicLoggerConfiguration *configuration,unsigned int *flags) CHECK_NON_NULL_PTR(1) CHECK_NON_NULL_PTR(2) NO_PROFILING;
#endif /* DBGFLAGS_INTERFACE_VERSION >= 12 */

#ifdef __cplusplus
}
#if 0
struct configurationFileParams {
public:
  size_t size; /* size of the used dynamicLoggerConfiguration structure (=sizeof(ConfigurationFileParams)) */
  configurationFileType fileType;
   union {
     //configurationXMLFileParams XMLFile;
char toto;
     /*char RFU[4];*/
  } params;
// to solve error: member ‘configurationXMLFileParams configurationFileParams_::<anonymous union>::XMLFile’ with copy assignment operator not allowed in union
inline configurationFileParams operator = (const struct configurationFileParams &src) {
  if (this != &src) {
    size = src.size;
    fileType = src.fileType;
    switch(fileType) {
      case XMLFileType:
         //params.XMLFile.rootNodePath = src.params.XMLFile.rootNodePath;
         //params.XMLFile.separator = src.params.XMLFile.separator;
         break;
      default:
         break;
    } //switch(fileType)
  } // (this != &src)
  return *this;
}
};
#endif /* 0 */
#endif /* __cplusplus */

#if DBGFLAGS_INTERFACE_VERSION >= 13

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * logger to files, one file per thread (=> no mutex)
 * each one can have its own parameters (ident, size, duration, directory (?)...)
 */

void openLogThreadFile(const char *ident, int logstat, int logfac) NO_PROFILING;
void threadFileLogger(int priority, const char *format, ...) CHECK_PARAMS(2,3) CHECK_NON_NULL_PTR(2) NO_PROFILING;
void vthreadFileLogger(int priority, const char *format, va_list optional_arguments) CHECK_NON_NULL_PTR(2) NO_PROFILING;
int setThreadFileLoggerMaxSize(const size_t maxSizeInBytes) NO_PROFILING;
int setThreadFileLoggerMaxDuration(const time_t maxDurationInSeconds) NO_PROFILING;
int setThreadFileLoggerDirectory(const char *directory) CHECK_NON_NULL_PTR(1) NO_PROFILING;
void closeLogThreadFile(void) NO_PROFILING;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DBGFLAGS_INTERFACE_VERSION */

#endif /* _LOGGERS_H_ */
