#ifndef _SYSLOG_EX_BUFFERS_H
#define _SYSLOG_EX_BUFFERS_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif /* _XOPEN_SOURCE */

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

//#include "syslogex_debug.h"

/*
 * parameters
 */

#ifndef MAX_MSG_SIZE
#define MAX_MSG_SIZE 1024
#endif /* MAX_MSG_SIZE */

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 2 /*128*/
#endif /* BUFFER_SIZE */

#undef MODULE_FLAG
#define MODULE_FLAG FLAG_BUFFER

typedef struct buffer_msg_
{
  pthread_t caller_tid;
  int       prio;
  int       flag;
  char      msg[MAX_MSG_SIZE];
} buffer_msg;

typedef struct buffer_cursor_
{
  unsigned int    position;
  pthread_mutex_t mutex;
  sem_t           semaphore;
} buffer_cursor;

static __inline int cursorInit(buffer_cursor *pcursor,const unsigned int semInitValue)
{
  int error = 0;
  pcursor->position = 0;
  pthread_mutex_init(&pcursor->mutex,NULL);
  if (sem_init(&pcursor->semaphore,0,semInitValue) != 0)
  {
    error = errno;
    /*syslogex(INTERNALLOG,"sem_init error %d (%m)",error);*/
    ERROR_MSG("sem_init error %d (%m)",error);
  }
  return error;
}

static __inline unsigned int cursorInc(buffer_cursor *pcursor)
{
  const int currentPosition = pcursor->position;
  pthread_mutex_lock(&pcursor->mutex);
  pcursor->position++;
  if (pcursor->position >= BUFFER_SIZE)
  {
    pcursor->position = 0;
  }
  pthread_mutex_unlock(&pcursor->mutex);
  return currentPosition;
}

static __inline int cursorTimedWait(buffer_cursor *pcursor,const struct timespec *abs_timeout)
{
  return sem_timedwait(&pcursor->semaphore,abs_timeout);
}

static __inline int cursorWait(buffer_cursor *pcursor)
{
  return sem_wait(&pcursor->semaphore);
}

static __inline int cursorIncSemaphore(buffer_cursor *pcursor)
{
  return sem_post(&pcursor->semaphore);
}

typedef struct buffer_data_
{
  buffer_cursor readPos;
  buffer_cursor writePos;
  buffer_msg    msg[BUFFER_SIZE];
} buffer_data;

static __inline int buffersInit(buffer_data *pbuffer)
{
  int error = 0;
  error = cursorInit(&pbuffer->readPos,0);
  if (0 == error)
  {
    error = cursorInit(&pbuffer->writePos,BUFFER_SIZE);
    if (0 == error)
    {
      memset(&pbuffer->msg,0,sizeof(buffer_msg)*BUFFER_SIZE);
    }
    else
    {
      /*syslogex(INTERNALLOG,"cursor_init writePos error %d",error);*/
      ERROR_MSG("cursor_init writePos error %d",error);
    }
  }
  else
  {
    /*syslogex(INTERNALLOG,"cursor_init readPos error %d",error);*/
    ERROR_MSG("cursor_init readPos error %d",error);
  }
  return error;
}

static __inline buffer_msg* getReadBuffer(buffer_data *asyncdata)
{
  buffer_msg *pbuffer = NULL;
  int error = cursorWait(&asyncdata->readPos);
  if (0 == error)
  {
    /* get the current position and move the cursor to the next one */
    const int currentPosition = cursorInc(&asyncdata->readPos);
    pbuffer = asyncdata->msg + currentPosition;
  }
  else
  {
    error = errno;
    /*syslogex(INTERNALLOG,"sem_wait for read error %d (%m)",errno);*/
    ERROR_MSG("sem_wait for read error %d (%m)",error);
  }
  return pbuffer;
}

static __inline buffer_msg* getTimedReadBuffer(buffer_data *asyncdata,const unsigned int timeOut)
{
  buffer_msg *pbuffer = NULL;
  struct timespec abs_timeout;

  if (time(&abs_timeout.tv_sec) != -1)
  {
    int error = 0;
    abs_timeout.tv_sec += MS_TO_SEC(timeOut);
    abs_timeout.tv_nsec = MS_TO_NSEC_MINUS_SEC(timeOut);

    /* wait for a fulled slot */
    error = cursorTimedWait(&asyncdata->readPos,&abs_timeout);
    if (0 == error)
    {
      /* get the current position and move the cursor to the next one */
      const int currentPosition = cursorInc(&asyncdata->readPos);
      pbuffer = asyncdata->msg + currentPosition;
    }
    else
    {
      const int error = errno;
      syslogex(INTERNALLOG,"sem_timedwait for read error %d (%m))",error);
      ERROR_MSG("sem_timedwait for read error %d (%m))",error);
    }
  }
  else
  {
    const int error = errno;
    syslogex(INTERNALLOG,__FILE__ "(%d): time error %d (%m)",__LINE__,error);
    ERROR_MSG("time error %d (%m)",error);
  }

  return pbuffer;
}

static __inline buffer_msg* getWriteBuffer(buffer_data *asyncdata)
{
  buffer_msg *pbuffer = NULL;
  int error = cursorWait(&asyncdata->writePos);
  if (0 == error)
  {
    /* get the current position and move the cursor to the next one */
    const int currentPosition = cursorInc(&asyncdata->writePos);
    pbuffer = asyncdata->msg + currentPosition;
  }
  else
  {
    error = errno;
    /*syslogex(INTERNALLOG,"sem_wait for write error %d (%m))",errno);*/
    ERROR_MSG("sem_wait for write error %d (%m))",error);
  }
  return pbuffer;
}

static __inline buffer_msg* getTimedWriteBuffer(buffer_data *asyncdata,const unsigned int timeOut)
{
  buffer_msg *pbuffer = NULL;
  struct timespec abs_timeout;

  if (time(&abs_timeout.tv_sec) != -1)
  {
    int error = 0;
    abs_timeout.tv_sec += MS_TO_SEC(timeOut);
    abs_timeout.tv_nsec = MS_TO_NSEC_MINUS_SEC(timeOut);

    /* wait for an empty slot */
    error = cursorTimedWait(&asyncdata->writePos,&abs_timeout);
    if (0 == error)
    {
      /* get the current position and move the cursor to the next one */
      const int currentPosition = cursorInc(&asyncdata->writePos);
      pbuffer = asyncdata->msg + currentPosition;
    }
    else
    {
      error = errno;
      /*syslogex(INTERNALLOG,"sem_timedwait for write error %d (%m)",errno);*/
      ERROR_MSG("sem_timedwait for write error %d (%s)",error,strerror(error));
    }
  }
  else
  {
    const int error = errno;
    /*syslogex(INTERNALLOG,__FILE__ "(%d): time error %d (%m)",__LINE__,errno);*/
    ERROR_MSG("time error %d (%s)",error,strerror(error));
  }

  return pbuffer;
}

static __inline int signalEmptyBufferAvailable(buffer_data *asyncdata)
{
  int error = cursorIncSemaphore(&asyncdata->writePos);
  if (unlikely(error != 0))
  {
    error = errno;
    /*syslogex(INTERNALLOG,__FILE__ "sem_post write error %d (%m)",errno);*/
    ERROR_MSG("sem_post write error %d (%m)",error);
  }
  return error;
}

static __inline int signalFilledBufferAvailable(buffer_data *asyncdata)
{
  int error = cursorIncSemaphore(&asyncdata->readPos);
  if (unlikely(error != 0))
  {
      error = errno;
    /*syslogex(INTERNALLOG,__FILE__ "sem_post read error %d (%m)",errno);*/
    ERROR_MSG("sem_post read error %d (%m)",error);
  }
  return error;
}

#ifdef __cplusplus
}
#endif

#endif /* _SYSLOG_EX_BUFFERS_H */

