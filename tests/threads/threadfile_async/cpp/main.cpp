#include <dbgflags/loggers.h>
#include <dbgflags/loggers_streams>
#include <dbgflags/debug_macros.h>
#include<pthread.h>
#include <cstdlib>
#include <unistd.h>
#include <new>
#include <linux/unistd.h>

//#include <dumapp.h>

static const unsigned int nb_threads(100);
static const unsigned int nb_loop(100);
const char *program = NULL;

 emergencyLogger    logEmergency;
 alertLogger         logAlert;
 criticalLogger     logCritical;
 errorLogger         logError;
 warningLogger      logWarning;
 noticeLogger       logNotice;
 infoLogger          logInfo;
 debugLogger         logDebug;
 contextLogger       logContext;

#if 0
/*
typedef loggerStream<LOG_EMERG, fileLogger, char> emergencyAllocTracedLogger;
typedef loggerStream<LOG_ALERT, fileLogger,char> alertAllocTracedLogger;
typedef loggerStream<LOG_CRIT, fileLogger,char> criticalAllocTracedLogger;
typedef loggerStream<LOG_ERR, fileLogger,char> errorAllocTracedLogger;
typedef loggerStream<LOG_WARNING, fileLogger,char> warningAllocTracedLogger;
typedef loggerStream<LOG_NOTICE, fileLogger,char> noticeAllocTracedLogger;
typedef loggerStream<LOG_INFO, fileLogger,char> infoAllocTracedLogger;
typedef loggerStream<LOG_DEBUG, fileLogger,char> debugAllocTracedLogger;
typedef loggerStream<LOG_DEBUG, ctxLogger,char> contextAllocTracedLogger;
*/ 

 emergencyLogger    logEmergency;
 alertLogger         logAlert;
 criticalLogger     logCritical;
 errorLogger         logError;
 warningLogger      logWarning;
 noticeLogger       logNotice;
 infoLogger          logInfo;
 debugLogger         logDebug;
 contextLogger       logContext;

 class mutexMgr {
    pthread_mutex_t mutex;
    public:
       mutexMgr(pthread_mutex_t &m):mutex(m){
          int error = pthread_mutex_lock(&mutex);
       }
       ~mutexMgr(){
          int error = pthread_mutex_unlock(&mutex);
       }
 };
 
 //static pthread_once_t initialized = PTHREAD_ONCE_INIT;
 class TLSRawData {
public:
    TLSRawData *TLSInterfacePtr;
    TLSRawData():TLSInterfacePtr(this){
    }
    virtual ~TLSRawData(){
    }
    //virtual int freeData() = 0;
 };
 
 /*class TLSClassInterface {
public:

   
   TLSClassInterface(){
   }
   ~TLSClassInterface(){
   }
   virtual int freeTLSData(TLSRawData *params) = 0;
 };*/

 static void freeTLSData(void *params) {
    if (params != NULL) {
       TLSRawData *TLSData = static_cast<TLSRawData *>(params);
       if (TLSData != 0) {
          //TLSData->freeData();
          delete TLSData;
          DEBUG_VAR(TLSData,"0x%X");
          TLSData = 0;
          /*TLSClassInterface *TLSInterfacePtr = TLSData->TLSInterfacePtr;
          if (TLSInterfacePtr != 0) {
             TLSInterfacePtr->freeTLSData(TLSData);
          }*/
       }
    }
 }
 static __inline pid_t __gettid()
 {
    long res;
    __asm__ volatile ("int $0x80" \
       : "=a" (res) \
       : "0" (__NR_gettid));
    do
    {
       if ((unsigned long)(res) >= (unsigned long)(-(128 + 1)))
       {
          errno = -(res);
          res = -1;
       }
       return (pid_t) (res);
    } while (0);
 }
#define gettid __gettid
 
 template <class UnSafeThreadClass> class TLSInstanceOf /*: public TLSClassInterface*/ {
    pthread_key_t TLSKey;
public:
   struct TLSData : public TLSRawData {
      UnSafeThreadClass *objectPtr;
      TLSData(UnSafeThreadClass *obj):objectPtr(obj){
      }
      virtual ~TLSData(){
         if (objectPtr != 0) {
            delete objectPtr;
            DEBUG_VAR(objectPtr,"0x%X");
            objectPtr = 0;
         }
      }
      /*virtual int freeData() {
         int error = EXIT_SUCCESS;
         delete objectPtr;
         DEBUG_VAR(objectPtr,"0x%X");
         objectPtr = 0;
         return error;
      }*/
   };
private:
   UnSafeThreadClass * allocateNewTLSInstance() {
       TLSData *data = 0;
#ifndef DUMA_CPP_OPERATORS_DECLARED
       UnSafeThreadClass *ptr = new(std::nothrow) UnSafeThreadClass();
       if (ptr) {
          DEBUG_VAR(ptr,"0x%X");
          data = new(std::nothrow) TLSData(ptr);
          DEBUG_VAR(data,"0x%X");
       } else {
          ERROR_MSG("failed to allocate %d bytes for a new instance of class UnSafeThreadClass for thread %u",sizeof(UnSafeThreadClass),gettid());
       }
#else
       UnSafeThreadClass *ptr = new UnSafeThreadClass();
       data = new TLSData(ptr);
#endif
       if (data != NULL) {
          const int error = pthread_setspecific(TLSKey,static_cast<const void*>(data));
       } else {
          ERROR_MSG("failed to allocate %d bytes for a new instance of TLSData for thread ",sizeof(TLSData));
       }
       return ptr;
    }
    
    void initialize(void) {
       int error = pthread_key_create(&TLSKey,::freeTLSData);
       if (0 == error) {
          allocateNewTLSInstance();
       }
    }
    
public:
   
   TLSInstanceOf():TLSKey(0){
       //pthread_once(&initialized,TLSInstanceOf::initialize);
      initialize();
   }
   
   ~TLSInstanceOf() {
      if (TLSKey != 0) {
         UnSafeThreadClass *ptr = static_cast<UnSafeThreadClass *>(pthread_getspecific(TLSKey));
         delete ptr;
         pthread_key_delete(TLSKey);
      }
   }
   
   UnSafeThreadClass &getInstance() {
      TLSData *data = static_cast<TLSData *>(pthread_getspecific(TLSKey));
      UnSafeThreadClass *tlsPtr = 0;
      if (unlikely(0 == data)) {
         pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;
         mutexMgr mutex(fastmutex);
         if (unlikely(0 == tlsPtr)) {
            tlsPtr = allocateNewTLSInstance();
         }
      } else {
         tlsPtr = data->objectPtr;
      }
      DEBUG_VAR(tlsPtr,"0x%X");
      return *tlsPtr;
   }

   /*virtual int freeTLSData(TLSRawData *params) {
      int error = EXIT_SUCCESS;
      TLSData *ptr = dynamic_cast<TLSData *>(params);
      if (ptr != 0) {
         free(ptr->objectPtr);
         ptr->objectPtr = 0;
         free(ptr);
      } else {
         error = EINVAL;
      }
   }*/
};

TLSInstanceOf<debugLogger>  streamDbg;
TLSInstanceOf<errorLogger>   streamError;
TLSInstanceOf<noticeLogger>  streamNotice;
//#define logDebug  streamDbg.getInstance()
//#define logError  streamError.getInstance()
//#define logNotice  streamNotice.getInstance()

//#include <dbgflags/debug_macros.h>

template <class T> inline void DebugVar_(const T &v,const char *name, const char *file, const unsigned int line, const char *function) {
   streamDbg.getInstance() << CPP_SIMPLE_DEBUG_LOG_HEADER << file << "(" << line << ") " << function << ": " << name << " = " << v << std::endl;
}
#undef DEBUG_CPP_VAR
#define DEBUG_CPP_VAR(x)  FILTER DebugVar_(x,#x,__FILE__,__LINE__,__FUNCTION__)
#endif

                                  void h(void *params) {
                               traceFunctionCall<noticeLogger,char,traceFunctionCallStdStringPtrPolicy<char> > f(logNotice,__PRETTY_FUNCTION__);
                               //d(params);
                                  }
                                  
                                  void g(void *params) {
                               traceFunctionCall<noticeLogger,char,traceFunctionCallStdStringPtrPolicy<char> > f(logNotice,__PRETTY_FUNCTION__);
                               h(params);
                                  }
                                                                    
                                  void e(void *params) {
                               traceFunctionCall<noticeLogger,char,traceFunctionCallStdStringPtrPolicy<char> > f(logNotice,__PRETTY_FUNCTION__);
                               g(params);
                                  }
                                  
                                  void d(void *params) {
                               traceFunctionCall<noticeLogger,char,traceFunctionCallStdStringPtrPolicy<char> > f(logNotice,__PRETTY_FUNCTION__);
                               e(params);
                                  }

void c(void *params) {
 traceFunctionCall<noticeLogger,char,traceFunctionCallStdStringPtrPolicy<char> > f(logNotice,__PRETTY_FUNCTION__);
 d(params);
}

void b(void *params) {
 traceFunctionCall<noticeLogger,char,traceFunctionCallStdStringPtrPolicy<char> > f(logNotice,__PRETTY_FUNCTION__);
 c(params);
}

void a(void *params) {
   traceFunctionCall<noticeLogger,char,traceFunctionCallStdStringPtrPolicy<char> > f(logNotice,__PRETTY_FUNCTION__);
   b(params);
}

static void __inline initializeLogger(const unsigned int n)  {
    const int logstat = LOG_PID|LOG_TID|LOG_RDTSC|LOG_FILE_WITHOUT_SYNC;
    const int logfac = LOG_USER;
    const char *identity = program;

    if (strcmp(TO_STRING(LOGGER),"threadFileLogger") == 0) {
    	char threadIdentity[PATH_MAX];
    	sprintf(threadIdentity,"%s.%d",identity,n);
    	openLogThreadFile(threadIdentity,logstat,logfac);
    	setThreadFileLoggerMaxSize(4 * 1024 * 1024);
    	DEBUG_VAR(threadIdentity,"%s");
    } else if (strcmp(TO_STRING(LOGGER),"syslogex") == 0) {
        openlogex(identity, logstat, logfac);
    } else if (strcmp(TO_STRING(LOGGER),"fileLogger") == 0) {
        openLogFile(program, logstat, logfac);
        setFileLoggerMaxSize(4 * 1024 * 1024);
    } else if (strcmp(TO_STRING(LOGGER),"consoleLogger") == 0) {
        setConsoleLoggerOpt(program, logstat, logfac);
    }
}

void *thread(void *params) {
   traceFunctionCall<noticeLogger,char,traceFunctionCallStdStringPtrPolicy<char> > f(logNotice,__PRETTY_FUNCTION__);
#if __LP64__
   unsigned long int l = (unsigned long int)params;
#else //__LP64__
   unsigned int l = reinterpret_cast<unsigned long>(params);
#endif //__LP64__
   unsigned int i;
   for(i=0;i<l;i++) {
      DEBUG_CPP_VAR(i);
      a(params);
   }
   return NULL;
}

int main(int argc, char *argv[]) {
   int error = EXIT_SUCCESS;
   pthread_t threads[nb_threads];
   unsigned int i;

   program = argv[0];
   initializeLogger(0);
   DEBUG_VAR(program,"%s");

   for(i=0;i<nb_threads;i++) {
      DEBUG_CPP_VAR(i);
      error = pthread_create(&threads[i],NULL,thread,(void*)nb_loop);
      if (error != 0) {
         //streamError.getInstance() << "pthread_create " << i << " error " << error << std::endl;
         ERROR_STREAM << "pthread_create " << i << " error " << error << std::endl;
         threads[i] = 0;
      }
   }
   for(i=0;i<nb_threads;i++) {
      if (threads[i] != 0) {
         void *thread_return = NULL;
         //streamNotice.getInstance() << "waiting for " << i << " ending..." << std::endl;
         NOTICE_STREAM << "waiting for " << i << " ending..." << std::endl;
         error = pthread_join(threads[i],&thread_return);
         DEBUG_CPP_VAR(error);
      } 
   }
   return error;
}
