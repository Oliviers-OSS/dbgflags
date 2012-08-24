#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <config.h>
#include <errno.h>
#include <stdarg.h>

FILE *logFile = NULL;

void log(const char*format,...) {
   char Buffer[1024];
   va_list args;
   int n = 0;
   va_start(args,format);
   n = vsprintf(Buffer,format,args);
   fwrite(Buffer,1,n,logFile);
   va_end(args);
}

unsigned int function(const unsigned int a,const unsigned  int b) {
   unsigned int i;
   unsigned int s = 0;
   for(i=0;i<b;i++) {
      s += a;
      log("* Debug * ####: : " __FILE__ "(%d) %s s = %d\n",__LINE__,__FUNCTION__,s);
   }
   return s;
}

int main(int argc, char *argv[]) {
  int error = EXIT_SUCCESS;
  unsigned int i;
  unsigned int n = 10000;
  volatile unsigned int r = 0;
  const char *logFileName = "clogfile.log";
  //struct rusage used;

  logFile = fopen(logFileName,"w");
  if (logFile != NULL) {
    if (argc > 1) {
       n = atoi(argv[1]);
    }

    for(i=0;i<n;i++) {
       r = function(4,n);
       log("* Debug * ####: : " __FILE__ "(%d) %s r = %d\n",__LINE__,__FUNCTION__,r);
    }
    fclose(logFile);
  } else {
    fprintf(stderr,"fopen %s error %m",logFileName,errno);
  }
  /*if (getrusage(RUSAGE_SELF,&used) == 0) {

  } else {
     ERROR_MSG("getrusage error");
  }*/

  return error;
}
