#include <dbgflags/loggers.h>
#include <dbgflags/debug_macros.h>
#include<pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define nb_threads  100
#define nb_loop     1000

void *thread(void *params) {
#if __LP64__
   unsigned long int l = (unsigned long int)params;
#else //__LP64__
   unsigned int l = (unsigned int)params;
#endif //__LP64__
     
   unsigned int i;
   for(i=0;i<l;i++) {
      DEBUG_VAR(i,"%d");
      usleep(1000);
   }
   return NULL;
}

int main(int argc, char *argv[]) {
   int error = EXIT_SUCCESS;
   const int logstat = LOG_PID|LOG_TID|LOG_RDTSC;
   const int logfac = LOG_USER;
   const char *identity = argv[0];

   pthread_t threads[nb_threads];
   unsigned int i;     

   setConsoleLoggerOpt(identity, logstat, logfac); //tmp
    if (strcmp(TO_STRING(LOGGER),"syslogex") == 0) {
        openlogex(identity, logstat, logfac);
    } else if (strcmp(TO_STRING(LOGGER),"fileLogger") == 0) {
        openLogFile(identity, logstat, logfac);
        setFileLoggerMaxSize(4 * 1024);
    } else if (strcmp(TO_STRING(LOGGER),"consoleLogger") == 0) {
        setConsoleLoggerOpt(identity, logstat, logfac);
    } else if (strcmp(TO_STRING(LOGGER),"threadFileLogger") == 0) {
	openLogThreadFile(identity, logstat, logfac);
	setThreadFileLoggerMaxSize(4 * 1024);
    }      
    
   for(i=0;i<nb_threads;i++) {
      DEBUG_VAR(i,"%d");
      error = pthread_create(&threads[i],NULL,thread,(void*)nb_loop);
      if (error != 0) {
         ERROR_MSG("pthread_create %d error %d",i,error);
         threads[i] = 0;
      }
   }
   for(i=0;i<nb_threads;i++) {
      if (threads[i] != 0) {
         void *thread_return = NULL;
         NOTICE_MSG("waiting for %d ending...",i);
         error = pthread_join(threads[i],&thread_return);
         DEBUG_VAR(error,"%d");
      } 
   }
   return error;
}
