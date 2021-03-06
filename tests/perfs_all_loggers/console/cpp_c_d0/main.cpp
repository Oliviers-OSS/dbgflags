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
  ,0x0
};
#define ZONE_INIT   FLAG_0
#define MODULE_FLAG ZONE_INIT

unsigned int function(const unsigned int a,const unsigned  int b) {
   unsigned int i;
   unsigned int s = 0;
   for(i=0;i<b;i++) {
      s += a;
      DEBUG_VAR(s,"%d");
   }
   return s;
}

int main(int argc, char *argv[]) {
  int error = EXIT_SUCCESS;
  unsigned int i;
  unsigned int n = 10000;
  volatile unsigned int r = 0;
  //struct rusage used;

  registerDebugFlags(&debugFlags);
  debugFlags.mask = 0;
  if (argc > 1) {
     n = atoi(argv[1]);
  }

  for(i=0;i<n;i++) {
     r = function(4,n);
     DEBUG_VAR(r,"%d");
  }

  /*if (getrusage(RUSAGE_SELF,&used) == 0) {

  } else {
     ERROR_MSG("getrusage error");
  }*/

  unregisterDebugFlags(&debugFlags);
  return error;
}
