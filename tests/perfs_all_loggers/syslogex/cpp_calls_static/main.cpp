#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "config.h"
#include <dbgflags/dbgflags.h>
#include <dbgflags/loggers.h>
#ifndef LOGGER
#define LOGGER consoleLogger
#endif /* LOGGER */
#define LOG_OPTS 0
#include <dbgflags/debug_macros.h>

debugLogger         logDebug;
#define TRACE_FCT_CALL traceFunctionCall<debugLogger> f(logDebug,__FUNCTION__,((debugFlags.mask &  MODULE_FLAG)!=0x0))

DebugFlags debugFlags =
{
  "c_dbgflags_0",
  {
    "Init"
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""
    ,""}
  ,0xFFFFFFFF
};

#define ZONE_INIT   FLAG_0
#define MODULE_FLAG ZONE_INIT

unsigned int function(const unsigned int a,const unsigned  int b) {
   TRACE_FCT_CALL;
   unsigned int i;
   unsigned int s = 0;
   for(i=0;i<b;i++) {
      s += a;
      DEBUG_STREAM << CPP_DEBUG_LOG_HEADER_POS << "s = " << s <<std::endl;
   }
   return s;
}

int main(int argc, char *argv[]) {
  TRACE_FCT_CALL;
  int error = EXIT_SUCCESS;
  unsigned int i;
  unsigned int n = 10000;
  volatile unsigned int r = 0;
  //struct rusage used;

  registerDebugFlags(&debugFlags);
  //debugFlags.mask = 0x0;

  if (argc > 1) {
     n = atoi(argv[1]);
  }

  for(i=0;i<n;i++) {
     r = function(4,n);
     DEBUG_STREAM << CPP_DEBUG_LOG_HEADER_POS << "r = " << r <<std::endl;
  }

  /*if (getrusage(RUSAGE_SELF,&used) == 0) {

  } else {
     ERROR_MSG("getrusage error");
  }*/
  unregisterDebugFlags(&debugFlags);
  return error;
}
