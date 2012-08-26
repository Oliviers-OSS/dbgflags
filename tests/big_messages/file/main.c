#include <dbgflags/loggers.h>
#include <dbgflags/debug_macros.h>
#include<pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define nb_threads  100
#define nb_loop     1000
#define MSG_SIZE    4096

static inline void fillMsg(char *msg,size_t s) {
  char *e = msg + s;
  int i = 0xFF;
  while(msg < e) {
    if (unlikely(i > 0x7C)) { 
      i = 0x21;      
    } 
    *msg = i++;   
    msg++;
  } /* while(msg < e) */
  --msg;
  *msg = '\0';
}

#define fillBuffer(f) fillMsg(f,sizeof(f))

void *thread(void *params) {
#if __LP64__
   unsigned long int l = (unsigned long int)params;
#else //__LP64__
   unsigned int l = (unsigned int)params;
#endif //__LP64__
     
   char bigMsg[MSG_SIZE];  
   unsigned int i;
   for(i=0;i<l;i++) {
      DEBUG_VAR(i,"%d");
      fillBuffer(bigMsg);
      ERROR_MSG("big msg = %s",bigMsg);
      usleep(1000);
   }
   return NULL;
}

int main(int argc, char *argv[]) {
   int error = EXIT_SUCCESS;
   pthread_t threads[nb_threads];
   unsigned int i;
   
   /* enable all log options for testing purpose */
   if (strcmp(TO_STRING(LOGGER),"consoleLogger") == 0) {
     openConsoleLogger(argv[0],LOG_PID|LOG_TID|LOG_RDTSC|LOG_CLOCK|LOG_LEVEL,LOG_USER);
   } else if (strcmp(TO_STRING(LOGGER),"fileLogger") == 0) {
     openLogFile(argv[0],LOG_PID|LOG_TID|LOG_RDTSC|LOG_CLOCK|LOG_LEVEL,LOG_USER);
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
