#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <iostream>
#include <dbgflags/syslogex.h>

#define CPP_SIMPLE_DEBUG_LOG_HEADER     DEBUG_LOG_HEADER ":"
#define CPP_DEBUG_LOG_HEADER_POS        " [ " << __FUNCTION__ << " (" << __FILE__ << ":" << __LINE__ << ")]:"

using namespace std;

unsigned int function(const unsigned int a,const unsigned  int b) {
   unsigned int i;
   unsigned int s = 0;
   for(i=0;i<b;i++) {
      s += a;
      syslogex(LOG_DEBUG,"* Debug * ####: : " __FILE__ "(%d) %s s = %d\n",__LINE__,__FUNCTION__,s);
   }
   return s;
}

int main(int argc, char *argv[]) {
  int error = EXIT_SUCCESS;
  unsigned int i;
  unsigned int n = 10000;
  volatile unsigned int r = 0;
  //struct rusage used;

  if (argc > 1) {
     n = atoi(argv[1]);
  }

  for(i=0;i<n;i++) {
     r = function(4,n);
     syslogex(LOG_DEBUG,"* Debug * ####: : " __FILE__ "(%d) %s r = %d\n",__LINE__,__FUNCTION__,r);
  }

  /*if (getrusage(RUSAGE_SELF,&used) == 0) {

  } else {
     ERROR_MSG("getrusage error");
  }*/

  return error;
}
