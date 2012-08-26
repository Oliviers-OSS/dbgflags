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
   pthread_t threads[nb_threads];
   unsigned int i;
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
